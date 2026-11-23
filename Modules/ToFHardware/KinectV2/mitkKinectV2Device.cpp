/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include "mitkKinectV2Device.h"


#include <mitkRealTimeClock.h>
#include <mitkProperties.h>
#include <mitkSmartPointerProperty.h>
#include <mitkSurface.h>

#include <itksys/SystemTools.hxx>


namespace mitk
{
  bool KinectV2Device::m_PrintFrameRate = false;

  KinectV2Device::KinectV2Device():
    m_DistanceDataBuffer(nullptr),
    m_AmplitudeDataBuffer(nullptr),
    m_RGBDataBuffer(nullptr),
    m_DepthBufferSize(sizeof(float)*512*424),
    m_RGBBufferSize(3*1920*1080)
  {
    m_Controller = mitk::KinectV2Controller::New();
    m_PolyData = vtkSmartPointer<vtkPolyData>::New();
  }

  KinectV2Device::~KinectV2Device()
  {
  }

  std::string GetDeviceName()
  {
    return "Microsoft Kinect 2 Device ";
  }

  bool KinectV2Device::OnConnectCamera()
  {
    bool ok = false;
    if (m_Controller)
    {
      ok = m_Controller->OpenCameraConnection();
      if (ok)
      {
        this->m_CaptureWidth = m_Controller->GetDepthCaptureWidth();
        this->m_CaptureHeight = m_Controller->GetDepthCaptureHeight();
        this->m_PixelNumber = this->m_CaptureWidth * this->m_CaptureHeight;

        this->m_RGBImageWidth = m_Controller->GetRGBCaptureWidth();
        this->m_RGBImageHeight = m_Controller->GetRGBCaptureHeight();
        this->m_RGBPixelNumber = this->m_RGBImageWidth * this->m_RGBImageHeight;

        // allocate buffer
        this->m_DistanceArray = new float[this->m_PixelNumber];
        for(int i=0; i<this->m_PixelNumber; i++) {this->m_DistanceArray[i]=0.0;}
        this->m_AmplitudeArray = new float[this->m_PixelNumber];
        for(int i=0; i<this->m_PixelNumber; i++) {this->m_AmplitudeArray[i]=0.0;}

        this->m_DistanceDataBuffer = new float*[this->m_MaxBufferSize];
        for(int i=0; i<this->m_MaxBufferSize; i++)
        {
          this->m_DistanceDataBuffer[i] = new float[this->m_PixelNumber];
        }
        this->m_AmplitudeDataBuffer = new float*[this->m_MaxBufferSize];
        for(int i=0; i<this->m_MaxBufferSize; i++)
        {
          this->m_AmplitudeDataBuffer[i] = new float[this->m_PixelNumber];
        }
        this->m_RGBDataBuffer = new unsigned char*[this->m_MaxBufferSize];
        for (int i=0; i<this->m_MaxBufferSize; i++)
        {
          this->m_RGBDataBuffer[i] = new unsigned char[this->m_RGBPixelNumber*3];
        }

        m_CameraConnected = true;
      }
    }
    return ok;
  }

  bool KinectV2Device::DisconnectCamera()
  {
    bool ok = false;
    if (m_Controller)
    {
      ok =  m_Controller->CloseCameraConnection();

      // clean-up only if camera was connected
      if (m_CameraConnected)
      {
        delete [] m_DistanceArray;
        delete [] m_AmplitudeArray;

        for(int i=0; i<this->m_MaxBufferSize; i++)
        {
          delete[] this->m_DistanceDataBuffer[i];
          delete[] this->m_AmplitudeDataBuffer[i];
          delete[] this->m_RGBDataBuffer[i];
        }
        delete[] this->m_DistanceDataBuffer;
        delete[] this->m_AmplitudeDataBuffer;
        delete[] this->m_RGBDataBuffer;

        m_CameraConnected = false;
      }
    }
    return ok;
  }

  void KinectV2Device::StartCamera()
  {
    if (m_CameraConnected)
    {
      // get the first image
      this->m_Controller->UpdateCamera();
      this->m_ImageMutex.lock();
      this->m_Controller->GetAllData(this->m_DistanceDataBuffer[this->m_FreePos],this->m_AmplitudeDataBuffer[this->m_FreePos],this->m_RGBDataBuffer[this->m_FreePos]);
      this->m_FreePos = (this->m_FreePos+1) % this->m_BufferSize;
      this->m_CurrentPos = (this->m_CurrentPos+1) % this->m_BufferSize;
      this->m_ImageSequence++;
      this->m_ImageMutex.unlock();

      this->m_CameraActiveMutex.lock();
      this->m_CameraActive = true;
      this->m_CameraActiveMutex.unlock();
      this->m_Thread = std::thread(&KinectV2Device::Acquire, this);
      // wait a little to make sure that the thread is started
      itksys::SystemTools::Delay(10);
    }
    else
    {
      MITK_WARN << "Camera not connected";
    }
  }

  void KinectV2Device::UpdateCamera()
  {
    if (m_Controller)
    {
      m_Controller->UpdateCamera();
    }
  }

  void KinectV2Device::Acquire()
  {
    mitk::RealTimeClock::Pointer realTimeClock = mitk::RealTimeClock::New();
    double t1 = realTimeClock->GetCurrentStamp();
    const int n = 100;
    while (this->IsCameraActive())
    {
      this->UpdateCamera();

      bool generateTriangularMesh = false;
      this->GetBoolProperty("GenerateTriangularMesh", generateTriangularMesh);
      this->m_Controller->SetGenerateTriangularMesh(generateTriangularMesh);

      if (auto* threshold = dynamic_cast<mitk::FloatProperty*>(this->GetProperty("TriangulationThreshold")))
        this->m_Controller->SetTriangulationThreshold(threshold->GetValue());

      // Readers only access m_CurrentPos, so filling m_FreePos needs no lock.
      // This keeps the main thread responsive while the controller waits for frames.
      if (!this->m_Controller->GetAllData(this->m_DistanceDataBuffer[this->m_FreePos],
                                          this->m_AmplitudeDataBuffer[this->m_FreePos],
                                          this->m_RGBDataBuffer[this->m_FreePos]))
      {
        continue;
      }

      this->m_ImageMutex.lock();
      // GetAllImages() cannot pass a surface, so the one computed by the
      // controller is handed over via the "ToFSurface" property.
      this->m_PolyData = this->m_Controller->GetVtkPolyData();
      this->m_FreePos = (this->m_FreePos+1) % this->m_BufferSize;
      this->m_CurrentPos = (this->m_CurrentPos+1) % this->m_BufferSize;
      this->m_ImageSequence++;
      const int imageSequence = this->m_ImageSequence;
      this->m_ImageMutex.unlock();

      this->Modified();

      if (m_PrintFrameRate && imageSequence % n == 0)
      {
        const double t2 = realTimeClock->GetCurrentStamp() - t1;
        MITK_INFO << " Framerate (fps): " << n / (t2/1000) << " Sequence: " << imageSequence;
        t1 = realTimeClock->GetCurrentStamp();
      }
    }
  }

  void KinectV2Device::GetAmplitudes(float* amplitudeArray, int& imageSequence)
  {
    m_ImageMutex.lock();
    if (m_CameraActive)
    {
      memcpy(amplitudeArray, this->m_AmplitudeDataBuffer[this->m_CurrentPos], this->m_DepthBufferSize);
      imageSequence = this->m_ImageSequence;
    }
    else
    {
      MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
    }
    m_ImageMutex.unlock();
  }

  void KinectV2Device::GetIntensities(float*, int&)
  {
  }

  void KinectV2Device::GetDistances(float* distanceArray, int& imageSequence)
  {
    m_ImageMutex.lock();
    if (m_CameraActive)
    {
      memcpy(distanceArray, this->m_DistanceDataBuffer[this->m_CurrentPos], this->m_DepthBufferSize);
      imageSequence = this->m_ImageSequence;
    }
    else
    {
      MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
    }
    m_ImageMutex.unlock();
  }

  void KinectV2Device::GetAllImages(float* distanceArray, float* amplitudeArray, float* /*intensityArray*/, char* /*sourceDataArray*/,
    int requiredImageSequence, int& capturedImageSequence, unsigned char* rgbDataArray)
  {
    if (m_CameraActive)
    {
      // check for empty buffer
      if (this->m_ImageSequence < 0)
      {
        // buffer empty
        MITK_WARN << "Buffer empty!! ";
        capturedImageSequence = this->m_ImageSequence;
        return;
      }
      // determine position of image in buffer
      int pos = 0;
      if ((requiredImageSequence < 0) || (requiredImageSequence > this->m_ImageSequence))
      {
        capturedImageSequence = this->m_ImageSequence;
        pos = this->m_CurrentPos;
      }
      else if (requiredImageSequence <= this->m_ImageSequence - this->m_BufferSize)
      {
        capturedImageSequence = (this->m_ImageSequence - this->m_BufferSize) + 1;
        pos = (this->m_CurrentPos + 1) % this->m_BufferSize;
      }
      else // (requiredImageSequence > this->m_ImageSequence - this->m_BufferSize) && (requiredImageSequence <= this->m_ImageSequence)
      {
        capturedImageSequence = requiredImageSequence;
        pos = (this->m_CurrentPos + (10-(this->m_ImageSequence - requiredImageSequence))) % this->m_BufferSize;
      }
      // write image data to arrays
      m_ImageMutex.lock();
      memcpy(distanceArray, this->m_DistanceDataBuffer[pos], this->m_DepthBufferSize);
      memcpy(amplitudeArray, this->m_AmplitudeDataBuffer[pos], this->m_DepthBufferSize);
      memcpy(rgbDataArray, this->m_RGBDataBuffer[pos], this->m_RGBBufferSize);
      // The acquisition thread reassigns m_PolyData, so it may only be copied under the lock.
      vtkSmartPointer<vtkPolyData> polyData = this->m_PolyData;
      m_ImageMutex.unlock();

      //Since the standard method GetAllImages does not allow transfering a surface,
      //we use a property to pass the surface to the workbench.
      mitk::Surface::Pointer surface = mitk::Surface::New();
      surface->SetVtkPolyData(polyData);
      this->SetProperty("ToFSurface", mitk::SmartPointerProperty::New( surface ));

      this->Modified();
    }
    else
    {
      MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
    }
  }

  KinectV2Controller::Pointer KinectV2Device::GetController()
  {
    return this->m_Controller;
  }

  int KinectV2Device::GetRGBCaptureWidth()
  {
    return this->m_RGBImageWidth;
  }

  int KinectV2Device::GetRGBCaptureHeight()
  {
    return this->m_RGBImageHeight;
  }
}
