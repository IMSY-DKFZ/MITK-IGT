/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkKinectTestHelper.h"

#include <mitkProperties.h>
#include <mitkTestingMacros.h>

#include <algorithm>

namespace
{
  constexpr float MinimumDepthMm = 400.0f;
  constexpr float MaximumDepthMm = 8000.0f;
  constexpr unsigned char RgbSentinel = 0xAB;

  void CheckRgbStream(mitk::ToFCameraDevice* device)
  {
    MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), "RGB: connect");
    device->StartCamera();

    mitk::KinectTestHelper::Frame first;
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, -1, first), "RGB: a frame arrives within 1 s");

    const auto start = std::chrono::steady_clock::now();
    mitk::KinectTestHelper::Frame frame = first;
    int received = 0;
    for (int i = 0; i < 100; ++i)
    {
      if (mitk::KinectTestHelper::WaitForFrame(device, frame.Sequence, frame))
        ++received;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    MITK_TEST_OUTPUT(<< "RGB: " << received << " of 100 frames in " << elapsed.count() << " ms");
    MITK_TEST_CONDITION_REQUIRED(received >= 90, "RGB: at least 90 of 100 frames arrive");
    MITK_TEST_CONDITION_REQUIRED(elapsed.count() < 50 * received, "RGB: the mean frame interval is below 50 ms");
    MITK_TEST_CONDITION_REQUIRED(frame.Distances != first.Distances, "RGB: the depth image changes between frames");
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::CountValidDepthInRange(frame, MinimumDepthMm, MaximumDepthMm) > 0, "RGB: depth is valid and between 400 and 8000 mm");
    MITK_TEST_CONDITION_REQUIRED(std::any_of(frame.Rgb.begin(), frame.Rgb.end(), [](unsigned char c) { return c != 0; }), "RGB: the RGB image is not black");

    device->StopCamera();
    MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), "RGB: disconnect");
  }

  void CheckIrStream(mitk::ToFCameraDevice* device)
  {
    device->SetProperty("IR", mitk::BoolProperty::New(true));
    MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), "IR: connect");
    device->StartCamera();

    mitk::KinectTestHelper::Frame frame;
    std::fill(frame.Rgb.begin(), frame.Rgb.end(), RgbSentinel);
    for (int i = 0; i < 30; ++i)
      MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, frame.Sequence, frame), "IR: frame " << i << " arrives");
    MITK_TEST_CONDITION_REQUIRED(std::any_of(frame.Amplitudes.begin(), frame.Amplitudes.end(), [](float a) { return a != 0.0f; }), "IR: the amplitude image is not empty");
    MITK_TEST_CONDITION_REQUIRED(std::all_of(frame.Rgb.begin(), frame.Rgb.end(), [](unsigned char c) { return c == RgbSentinel; }), "IR: the RGB buffer is left untouched");

    device->StopCamera();
    MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), "IR: disconnect");
    device->SetProperty("IR", mitk::BoolProperty::New(false));
  }

  void CheckReconnect(mitk::ToFCameraDevice* device)
  {
    for (int round = 0; round < 2; ++round)
    {
      MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), "Reconnect: connect, round " << round);
      device->StartCamera();
      mitk::KinectTestHelper::Frame frame;
      for (int i = 0; i < 10; ++i)
        MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, frame.Sequence, frame), "Reconnect: frame " << i << ", round " << round);
      const auto stopStart = std::chrono::steady_clock::now();
      device->StopCamera();
      MITK_TEST_CONDITION_REQUIRED(std::chrono::steady_clock::now() - stopStart < std::chrono::seconds(1), "Reconnect: StopCamera() returns within 1 s");
      MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), "Reconnect: disconnect, round " << round);
    }
  }
}

/**
 * Streams from a real Kinect 1. Reports "skipped" when no Kinect is attached.
 */
int mitkKinectHardwareTest(int /*argc*/, char* /*argv*/[])
{
  auto device = mitk::KinectTestHelper::CreateKinectDevice();
  if (device.IsNull())
  {
    std::cout << "The Kinect factory is not registered." << std::endl;
    return EXIT_FAILURE;
  }

  try
  {
    device->ConnectCamera();
    device->DisconnectCamera();
  }
  catch (const mitk::Exception& e)
  {
    if (mitk::KinectTestHelper::IsNoKinectFound(e))
    {
      std::cout << "No Kinect attached; skipping." << std::endl;
      return mitk::KinectTestHelper::SkipReturnCode;
    }
    throw;
  }

  MITK_TEST_BEGIN("mitkKinectHardwareTest");
  CheckRgbStream(device);
  CheckIrStream(device);
  CheckReconnect(device);
  MITK_TEST_END();
}
