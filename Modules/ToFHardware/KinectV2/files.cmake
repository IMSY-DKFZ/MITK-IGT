set(CPP_FILES
  mitkKinectV2Device.cpp
  mitkKinectV2DeviceFactory.cpp
  mitkKinectV2Activator.cpp
)

if(WIN32)
  list(APPEND CPP_FILES mitkKinectV2Controller.cpp)
else()
  list(APPEND CPP_FILES mitkKinectV2ControllerFreenect2.cpp)
endif()

set(RESOURCE_FILES
  CalibrationFiles/KinectV2_IR_camera.xml
  CalibrationFiles/KinectV2_RGB_camera.xml
)
