/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef mitkKinectTestHelper_h
#define mitkKinectTestHelper_h

#include <mitkException.h>
#include <mitkIToFDeviceFactory.h>
#include <mitkToFCameraDevice.h>

#include <usGetModuleContext.h>
#include <usModuleContext.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace mitk
{
  namespace KinectTestHelper
  {
    /** Return code that ctest reports as "skipped" (see SKIP_RETURN_CODE in CMakeLists.txt). */
    constexpr int SkipReturnCode = 77;

    /**
     * The Kinect module is an autoload module and must not be linked, so tests
     * reach it the way ToF Util does: through its registered device factory.
     */
    inline ToFCameraDevice::Pointer CreateKinectDevice()
    {
      auto* context = us::GetModuleContext();
      for (const auto& reference : context->GetServiceReferences<IToFDeviceFactory>())
      {
        auto* factory = context->GetService(reference);
        if (factory != nullptr && factory->GetFactoryName() == "Kinect Factory")
          return factory->CreateToFCameraDevice();
      }
      return nullptr;
    }

    constexpr unsigned int Width = 640;
    constexpr unsigned int Height = 480;

    /** One frame as delivered by ToFCameraDevice::GetAllImages(). */
    struct Frame
    {
      std::vector<float> Distances = std::vector<float>(Width * Height, 0.0f);
      std::vector<float> Amplitudes = std::vector<float>(Width * Height, 0.0f);
      std::vector<unsigned char> Rgb = std::vector<unsigned char>(Width * Height * 3, 0);
      int Sequence = -1;
    };

    /**
     * Waits until the device delivers an image newer than afterSequence, or
     * until the timeout expires. Returns false on timeout.
     */
    inline bool WaitForFrame(ToFCameraDevice* device, int afterSequence, Frame& frame,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
      const auto deadline = std::chrono::steady_clock::now() + timeout;
      std::vector<float> intensities(Width * Height);
      std::vector<char> sourceData(1);
      do
      {
        int captured = -1;
        device->GetAllImages(frame.Distances.data(), frame.Amplitudes.data(), intensities.data(),
          sourceData.data(), -1, captured, frame.Rgb.data());
        // Sequence 0 means that the device has no image yet.
        if (captured >= 1 && captured > afterSequence)
        {
          frame.Sequence = captured;
          return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      } while (std::chrono::steady_clock::now() < deadline);
      return false;
    }

    /** Number of valid (non-zero) depth pixels; fails the range check by returning -1. */
    inline int CountValidDepthInRange(const Frame& frame, float minimumMm, float maximumMm)
    {
      int valid = 0;
      for (const float distance : frame.Distances)
      {
        if (distance == 0.0f)
          continue;
        if (distance < minimumMm || distance > maximumMm)
          return -1;
        ++valid;
      }
      return valid;
    }

    /** True if the exception is the controller's "no camera attached" error. */
    inline bool IsNoKinectFound(const Exception& e)
    {
      return std::string(e.GetDescription()).find("No Kinect found") != std::string::npos;
    }
  }
}

#endif
