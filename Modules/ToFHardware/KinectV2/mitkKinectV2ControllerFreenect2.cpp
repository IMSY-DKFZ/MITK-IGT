/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkKinectV2Controller.h"

#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/logger.h>
#include <libfreenect2/registration.h>

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

#include <mitkLog.h>
#include <mitkNumericTypes.h>

#include <cmath>
#include <memory>
#include <vector>

namespace
{
  constexpr int DepthWidth = 512;
  constexpr int DepthHeight = 424;
  constexpr int ColorWidth = 1920;
  constexpr int ColorHeight = 1080;
  constexpr int FrameTimeoutMs = 100;
}

namespace mitk
{
  class KinectV2Controller::KinectV2ControllerPrivate
  {
  public:
    KinectV2ControllerPrivate();

    /** Waits for the next frame set and fills every buffer that is not null. */
    bool Acquire(float* distances, float* amplitudes, unsigned char* rgb);

    void UpdatePolyData(const libfreenect2::Frame& undistorted);

    libfreenect2::Freenect2 m_Context;
    libfreenect2::Freenect2Device* m_Device;
    std::unique_ptr<libfreenect2::SyncMultiFrameListener> m_Listener;
    std::unique_ptr<libfreenect2::Registration> m_Registration;
    libfreenect2::Frame m_Undistorted;
    libfreenect2::Frame m_Registered;
    std::vector<int> m_ColorDepthMap; ///< index into the color image for each depth pixel, -1 if invalid

    vtkSmartPointer<vtkPolyData> m_PolyData;
    double m_TriangulationThreshold; ///< squared distance threshold, 0 disables the check
    bool m_GenerateTriangularMesh;
  };

  KinectV2Controller::KinectV2ControllerPrivate::KinectV2ControllerPrivate()
    : m_Device(nullptr),
      m_Undistorted(DepthWidth, DepthHeight, 4),
      m_Registered(DepthWidth, DepthHeight, 4),
      m_ColorDepthMap(DepthWidth * DepthHeight, -1),
      m_PolyData(vtkSmartPointer<vtkPolyData>::New()),
      m_TriangulationThreshold(0.0),
      m_GenerateTriangularMesh(false)
  {
  }

  bool KinectV2Controller::KinectV2ControllerPrivate::Acquire(float* distances, float* amplitudes, unsigned char* rgb)
  {
    if (m_Device == nullptr)
      return false;

    libfreenect2::FrameMap frames;
    if (!m_Listener->waitForNewFrame(frames, FrameTimeoutMs))
      return false;

    const libfreenect2::Frame* color = frames[libfreenect2::Frame::Color];
    const libfreenect2::Frame* ir = frames[libfreenect2::Frame::Ir];
    const libfreenect2::Frame* depth = frames[libfreenect2::Frame::Depth];

    // The sensor delivers mirrored images. Flip them horizontally so the
    // result matches the Microsoft SDK backend.
    const auto* depthData = reinterpret_cast<const float*>(depth->data);
    const auto* irData = reinterpret_cast<const float*>(ir->data);
    for (int y = 0; y < DepthHeight; ++y)
    {
      for (int x = 0; x < DepthWidth; ++x)
      {
        const int source = x + y * DepthWidth;
        const int target = (DepthWidth - x - 1) + y * DepthWidth;
        if (distances != nullptr)
          distances[target] = depthData[source];
        if (amplitudes != nullptr)
          amplitudes[target] = irData[source];
      }
    }

    if (rgb != nullptr)
    {
      const bool isBgr = color->format == libfreenect2::Frame::BGRX;
      for (int y = 0; y < ColorHeight; ++y)
      {
        for (int x = 0; x < ColorWidth; ++x)
        {
          const unsigned char* pixel = color->data + (x + y * ColorWidth) * 4;
          unsigned char* target = rgb + ((ColorWidth - x - 1) + y * ColorWidth) * 3;
          target[0] = isBgr ? pixel[2] : pixel[0];
          target[1] = pixel[1];
          target[2] = isBgr ? pixel[0] : pixel[2];
        }
      }
    }

    if (distances != nullptr)
    {
      m_Registration->apply(color, depth, &m_Undistorted, &m_Registered, true, nullptr, m_ColorDepthMap.data());
      this->UpdatePolyData(m_Undistorted);
    }

    m_Listener->release(frames);
    return true;
  }

  void KinectV2Controller::KinectV2ControllerPrivate::UpdatePolyData(const libfreenect2::Frame& undistorted)
  {
    const unsigned int pointCount = DepthWidth * DepthHeight;
    const double meterFactor = 1000.0;

    auto points = vtkSmartPointer<vtkPoints>::New();
    auto vertices = vtkSmartPointer<vtkCellArray>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    auto textureCoordinates = vtkSmartPointer<vtkFloatArray>::New();
    textureCoordinates->SetNumberOfComponents(2);
    textureCoordinates->Allocate(pointCount);

    // Point IDs only grow for valid pixels, so remember the ID per pixel for triangulation.
    std::vector<vtkIdType> vertexIds(pointCount, 0);
    std::vector<bool> isPointValid(pointCount, false);

    for (int j = 0; j < DepthHeight; ++j)
    {
      for (int i = 0; i < DepthWidth; ++i)
      {
        const unsigned int pixelID = i + j * DepthWidth;

        float x, y, z;
        m_Registration->getPointXYZ(&undistorted, j, i, x, y, z);
        if (std::isnan(z) || z <= mitk::eps)
          continue;

        isPointValid[pixelID] = true;
        // libfreenect2 returns meters, MITK works in mm. x is negated to match the flipped images.
        vertexIds[pixelID] = points->InsertNextPoint(-x * meterFactor, y * meterFactor, z * meterFactor);

        if (m_GenerateTriangularMesh)
        {
          if (i >= 1 && j >= 1)
          {
            const vtkIdType xy = pixelID;
            const vtkIdType x_1y = pixelID - 1;
            const vtkIdType xy_1 = pixelID - DepthWidth;
            const vtkIdType x_1y_1 = xy_1 - 1;

            if (isPointValid[xy] && isPointValid[x_1y] && isPointValid[x_1y_1] && isPointValid[xy_1])
            {
              double pointXY[3], pointX_1Y[3], pointXY_1[3], pointX_1Y_1[3];
              points->GetPoint(vertexIds[xy], pointXY);
              points->GetPoint(vertexIds[x_1y], pointX_1Y);
              points->GetPoint(vertexIds[xy_1], pointXY_1);
              points->GetPoint(vertexIds[x_1y_1], pointX_1Y_1);

              if (mitk::Equal(m_TriangulationThreshold, 0.0) ||
                  (vtkMath::Distance2BetweenPoints(pointXY, pointX_1Y) <= m_TriangulationThreshold &&
                   vtkMath::Distance2BetweenPoints(pointXY, pointXY_1) <= m_TriangulationThreshold &&
                   vtkMath::Distance2BetweenPoints(pointX_1Y, pointX_1Y_1) <= m_TriangulationThreshold &&
                   vtkMath::Distance2BetweenPoints(pointXY_1, pointX_1Y_1) <= m_TriangulationThreshold))
              {
                polys->InsertNextCell(3);
                polys->InsertCellPoint(vertexIds[x_1y]);
                polys->InsertCellPoint(vertexIds[xy]);
                polys->InsertCellPoint(vertexIds[x_1y_1]);

                polys->InsertNextCell(3);
                polys->InsertCellPoint(vertexIds[x_1y_1]);
                polys->InsertCellPoint(vertexIds[xy]);
                polys->InsertCellPoint(vertexIds[xy_1]);
              }
              else
              {
                vertices->InsertNextCell(1);
                vertices->InsertCellPoint(vertexIds[xy]);
              }
            }
          }
        }
        else
        {
          vertices->InsertNextCell(1);
          vertices->InsertCellPoint(vertexIds[pixelID]);
        }

        const int colorIndex = m_ColorDepthMap[pixelID];
        if (colorIndex >= 0 && colorIndex < ColorWidth * ColorHeight)
        {
          const int colorX = colorIndex % ColorWidth;
          const int colorY = colorIndex / ColorWidth;
          // The RGB image is flipped as well, so mirror the u coordinate.
          const float u = 1.0f - (colorX + 0.5f) / ColorWidth;
          const float v = (colorY + 0.5f) / ColorHeight;
          textureCoordinates->InsertTuple2(vertexIds[pixelID], u, v);
        }
        else
        {
          textureCoordinates->InsertTuple2(vertexIds[pixelID], 0, 0);
        }
      }
    }

    m_PolyData = vtkSmartPointer<vtkPolyData>::New();
    m_PolyData->SetPoints(points);
    m_PolyData->SetVerts(vertices);
    m_PolyData->SetPolys(polys);
    m_PolyData->GetPointData()->SetTCoords(textureCoordinates);
  }

  KinectV2Controller::KinectV2Controller() : d(new KinectV2ControllerPrivate)
  {
    libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Warning));
  }

  KinectV2Controller::~KinectV2Controller()
  {
    this->CloseCameraConnection();
    delete d;
  }

  bool KinectV2Controller::OpenCameraConnection()
  {
    if (d->m_Device != nullptr)
      return true;

    if (d->m_Context.enumerateDevices() == 0)
    {
      MITK_WARN << "No Kinect 2 found!";
      return false;
    }

    d->m_Device = d->m_Context.openDefaultDevice();
    if (d->m_Device == nullptr)
    {
      MITK_WARN << "Could not open Kinect 2!";
      return false;
    }

    d->m_Listener = std::make_unique<libfreenect2::SyncMultiFrameListener>(
      libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth);
    d->m_Device->setColorFrameListener(d->m_Listener.get());
    d->m_Device->setIrAndDepthFrameListener(d->m_Listener.get());

    if (!d->m_Device->start())
    {
      MITK_WARN << "Could not start Kinect 2 streams!";
      this->CloseCameraConnection();
      return false;
    }

    // Camera parameters are only valid after start().
    d->m_Registration = std::make_unique<libfreenect2::Registration>(
      d->m_Device->getIrCameraParams(), d->m_Device->getColorCameraParams());

    MITK_INFO << "Kinect 2 successfully connected (serial " << d->m_Device->getSerialNumber() << ")";
    return true;
  }

  bool KinectV2Controller::CloseCameraConnection()
  {
    if (d->m_Device != nullptr)
    {
      d->m_Device->stop();
      d->m_Device->close();
      delete d->m_Device;
      d->m_Device = nullptr;
    }
    d->m_Registration.reset();
    d->m_Listener.reset();
    return true;
  }

  bool KinectV2Controller::InitializeMultiFrameReader()
  {
    return d->m_Device != nullptr;
  }

  bool KinectV2Controller::UpdateCamera()
  {
    return d->m_Device != nullptr;
  }

  void KinectV2Controller::GetDistances(float* distances)
  {
    d->Acquire(distances, nullptr, nullptr);
  }

  void KinectV2Controller::GetAmplitudes(float* amplitudes)
  {
    d->Acquire(nullptr, amplitudes, nullptr);
  }

  void KinectV2Controller::GetIntensities(float*)
  {
    // The Kinect 2 provides no intensity image.
  }

  void KinectV2Controller::GetRgb(unsigned char* rgb)
  {
    d->Acquire(nullptr, nullptr, rgb);
  }

  bool KinectV2Controller::GetAllData(float* distances, float* amplitudes, unsigned char* rgb)
  {
    return d->Acquire(distances, amplitudes, rgb);
  }

  int KinectV2Controller::GetRGBCaptureWidth() const
  {
    return ColorWidth;
  }

  int KinectV2Controller::GetRGBCaptureHeight() const
  {
    return ColorHeight;
  }

  int KinectV2Controller::GetDepthCaptureWidth() const
  {
    return DepthWidth;
  }

  int KinectV2Controller::GetDepthCaptureHeight() const
  {
    return DepthHeight;
  }

  vtkSmartPointer<vtkPolyData> KinectV2Controller::GetVtkPolyData()
  {
    return d->m_PolyData;
  }

  void KinectV2Controller::SetGenerateTriangularMesh(bool flag)
  {
    d->m_GenerateTriangularMesh = flag;
  }

  void KinectV2Controller::SetTriangulationThreshold(double triangulationThreshold)
  {
    d->m_TriangulationThreshold = triangulationThreshold * triangulationThreshold;
  }
}
