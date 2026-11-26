option(MITK_USE_TOF_KINECTV2 "Enable support for Kinect V2 camera" OFF)

if(MITK_USE_TOF_KINECTV2)
  find_package(freenect2 REQUIRED)
  set(MITK_TOF_AVAILABLE_CAMERAS ${MITK_TOF_AVAILABLE_CAMERAS},Microsoft Kinect V2)

  if(WIN32)
    # freenect2.dll and its libusb/turbojpeg dependencies are installed to
    # <prefix>/bin, not next to the import library in <prefix>/lib.
    get_filename_component(_freenect2_bin_dir "${freenect2_DIR}/../../../bin" ABSOLUTE)
    set_property(GLOBAL APPEND PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS "${_freenect2_bin_dir}")
  endif()

  MITK_CREATE_MODULE(
    INCLUDE_DIRS
      PUBLIC ${MITK_BIN_DIR}
      PRIVATE ${freenect2_INCLUDE_DIRS}
    DEPENDS MitkToFHardware
    ADDITIONAL_LIBS ${freenect2_LIBRARIES}
    AUTOLOAD_WITH MitkToFHardware
  )
endif()
