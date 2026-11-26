/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkKinectTestHelper.h"

#include <mitkTestingMacros.h>

/**
 * Checks of the Kinect 1 device that need no camera.
 */
int mitkKinectBasicsTest(int /*argc*/, char* /*argv*/[])
{
  MITK_TEST_BEGIN("mitkKinectBasicsTest");

  auto device = mitk::KinectTestHelper::CreateKinectDevice();
  MITK_TEST_CONDITION_REQUIRED(device.IsNotNull(), "The Kinect factory is registered and creates a device");
  MITK_TEST_CONDITION_REQUIRED(std::string(device->GetNameOfClass()) == "KinectDevice", "The factory creates a KinectDevice");
  MITK_TEST_CONDITION_REQUIRED(!device->IsCameraActive(), "The device is inactive before StartCamera()");

  bool connected = false;
  try
  {
    connected = device->ConnectCamera();
  }
  catch (const mitk::Exception& e)
  {
    MITK_TEST_CONDITION_REQUIRED(mitk::KinectTestHelper::IsNoKinectFound(e), "Without a camera, connecting reports that no Kinect was found");
  }

  if (connected)
  {
    MITK_TEST_OUTPUT(<< "A Kinect is attached; the no-device check does not apply.");
    device->DisconnectCamera();
  }
  else
  {
    MITK_TEST_CONDITION_REQUIRED(!device->IsCameraActive(), "A failed connect leaves the device inactive");
    MITK_TEST_CONDITION_REQUIRED(device->DisconnectCamera(), "Disconnecting an unconnected device succeeds");
  }

  MITK_TEST_END();
}
