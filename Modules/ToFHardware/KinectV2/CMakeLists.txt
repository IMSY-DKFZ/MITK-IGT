option(MITK_USE_TOF_KINECTV2 "Enable support for Kinect V2 camera" OFF)

if(MITK_USE_TOF_KINECTV2)
  if(WIN32)
    find_library(MITK_KINECTV2_LIB KinectV20.lib DOC "Kinect V2 access library.")
    find_path(MITK_KINECTV2_INCLUDE_DIR Kinect.h DOC "Include directory of Kinect V2 camera.")
    set(_kinectv2_include_dirs ${MITK_KINECTV2_INCLUDE_DIR})
    set(_kinectv2_libs ${MITK_KINECTV2_LIB})
  else()
    # The Microsoft SDK is Windows only, use libfreenect2 everywhere else.
    find_package(freenect2 REQUIRED)
    set(_kinectv2_include_dirs ${freenect2_INCLUDE_DIRS})
    set(_kinectv2_libs ${freenect2_LIBRARIES})
  endif()
  set(MITK_TOF_AVAILABLE_CAMERAS ${MITK_TOF_AVAILABLE_CAMERAS},Microsoft Kinect V2)

  MITK_CREATE_MODULE(
    INCLUDE_DIRS
      PUBLIC ${MITK_BIN_DIR}
      PRIVATE ${_kinectv2_include_dirs}
    DEPENDS MitkToFHardware
    ADDITIONAL_LIBS ${_kinectv2_libs}
    AUTOLOAD_WITH MitkToFHardware
  )
endif()
