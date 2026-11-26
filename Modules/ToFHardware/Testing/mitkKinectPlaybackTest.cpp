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

#include <itksys/SystemTools.hxx>

#include <algorithm>
#include <string>

namespace
{
  constexpr float MinimumDepthMm = 400.0f;
  constexpr float MaximumDepthMm = 8000.0f;
  constexpr float RawNoDepth = 2047.0f;
  constexpr unsigned char RgbSentinel = 0xAB;

  bool HasRecording()
  {
    std::string path;
    if (!itksys::SystemTools::GetEnv("FAKENECT_PATH", path) || path.empty())
      return false;
    // fakenect needs the registration data in device.json to produce depth in
    // mm; without it, it crashes in the MM and REGISTERED depth modes.
    return itksys::SystemTools::FileExists(path + "/device.json", true);
  }

  void CheckLifecycle(mitk::ToFCameraDevice* device)
  {
    MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), "Lifecycle: connect");
    device->StartCamera();
    device->StartCamera();
    mitk::KinectTestHelper::Frame frame;
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, -1, frame), "Lifecycle: a second StartCamera() keeps streaming");

    // The stream mode is fixed at connect time; switching to IR while
    // streaming RGB must keep delivering RGB.
    device->SetProperty("IR", mitk::BoolProperty::New(true));
    std::fill(frame.Rgb.begin(), frame.Rgb.end(), RgbSentinel);
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, frame.Sequence, frame), "Lifecycle: frames keep arriving after selecting IR");
    MITK_TEST_CONDITION_REQUIRED(std::any_of(frame.Rgb.begin(), frame.Rgb.end(), [](unsigned char c) { return c != RgbSentinel; }),
      "Lifecycle: RGB is still delivered until the next connect");
    device->SetProperty("IR", mitk::BoolProperty::New(false));

    MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), "Lifecycle: disconnect while streaming");
    MITK_TEST_CONDITION_REQUIRED(!device->IsCameraActive(), "Lifecycle: disconnecting stops the acquisition");
  }

  void StreamAndCheck(mitk::ToFCameraDevice* device, const char* phase)
  {
    MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), phase << ": connect");
    device->StartCamera();
    mitk::KinectTestHelper::Frame frame;
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, -1, frame, std::chrono::milliseconds(0)) && frame.Sequence >= 1,
      phase << ": an image is available as soon as StartCamera() returns");
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, -1, frame), phase << ": a frame arrives within 1 s");
    MITK_TEST_CONDITION_REQUIRED(std::none_of(frame.Distances.begin(), frame.Distances.end(), [](float d) { return d == RawNoDepth; }),
      phase << ": depth is converted to mm (no raw 11 bit 'no data' values)");
    const int valid = mitk::KinectTestHelper::CountValidDepthInRange(frame, MinimumDepthMm, MaximumDepthMm);
    MITK_TEST_CONDITION_REQUIRED(valid >= 0, phase << ": all depth values are 0 (invalid) or between 400 and 8000 mm");
    MITK_TEST_CONDITION_REQUIRED(valid > 0, phase << ": the frame has valid depth");
    MITK_TEST_CONDITION_REQUIRED(std::any_of(frame.Rgb.begin(), frame.Rgb.end(), [](unsigned char c) { return c != 0; }), phase << ": the RGB image is not black");
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(device, frame.Sequence, frame), phase << ": a second frame arrives");
    device->StopCamera();
    MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), phase << ": disconnect");
  }
}

/**
 * Plays a fakenect recording (FAKENECT_PATH) through the Kinect device. The
 * test is set up in CMakeLists.txt to load fakenect instead of libfreenect.
 *
 * fakenect reads FAKENECT_LOOP once per process, and keeps its position in
 * the recording across connections. ctest therefore runs this test twice:
 * looping, to check streaming and reconnecting, and with FAKENECT_LOOP=0,
 * to check that the end of the recording stops the device like an
 * unplugged camera.
 */
int mitkKinectPlaybackTest(int /*argc*/, char* /*argv*/[])
{
  if (!HasRecording())
  {
    std::cout << "No fakenect recording with device.json in FAKENECT_PATH; skipping." << std::endl;
    return mitk::KinectTestHelper::SkipReturnCode;
  }

  std::string loop;
  const bool playToEnd = itksys::SystemTools::GetEnv("FAKENECT_LOOP", loop) && loop == "0";

  MITK_TEST_BEGIN("mitkKinectPlaybackTest");

  if (!playToEnd)
  {
    auto device = mitk::KinectTestHelper::CreateKinectDevice();
    MITK_TEST_CONDITION_REQUIRED(device.IsNotNull(), "The Kinect factory creates a device");
    StreamAndCheck(device, "First connection");
    StreamAndCheck(device, "Reconnection");
    CheckLifecycle(device);

    {
      auto shortLived = mitk::KinectTestHelper::CreateKinectDevice();
      MITK_TEST_CONDITION_REQUIRED(shortLived->ConnectCamera(), "Connect a device that is destroyed while streaming");
      shortLived->StartCamera();
      mitk::KinectTestHelper::Frame frame;
      MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::WaitForFrame(shortLived, -1, frame), "It streams before it is destroyed");
    }
    MITK_TEST_OUTPUT(<< "Destroying a streaming device did not crash.");
  }
  else
  {
    auto device = mitk::KinectTestHelper::CreateKinectDevice();
    MITK_TEST_CONDITION_REQUIRED(device->ConnectCamera(), "Connect for playback to the end of the recording");
    device->StartCamera();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (device->IsCameraActive() && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    MITK_TEST_CONDITION_REQUIRED(!device->IsCameraActive(), "The end of the recording, like an unplugged camera, stops the device");
    const auto stopStart = std::chrono::steady_clock::now();
    device->StopCamera();
    MITK_TEST_CONDITION_REQUIRED(std::chrono::steady_clock::now() - stopStart < std::chrono::seconds(1), "StopCamera() returns within 1 s");
    device->DisconnectCamera();
  }

  MITK_TEST_END();
}
