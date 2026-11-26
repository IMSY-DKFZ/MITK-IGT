option(MITK_USE_TOF_KINECT "Enable support for Kinect camera" OFF)

if(MITK_USE_TOF_KINECT)
  find_package(libfreenect CONFIG REQUIRED)
  # The package config only provides variables, no imported targets.
  find_library(MITK_FREENECT_LIBRARY NAMES ${FREENECT_LIBRARIES} HINTS ${FREENECT_LIBRARY_DIRS} NO_DEFAULT_PATH REQUIRED)
  set(MITK_TOF_AVAILABLE_CAMERAS ${MITK_TOF_AVAILABLE_CAMERAS},Microsoft Kinect)

  if(WIN32)
    # libfreenect installs freenect.dll to <prefix>/lib, while its package
    # config announces <prefix>/bin, so search both.
    set_property(GLOBAL APPEND PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS
      "${FREENECT_RUNTIME_LIBRARY_DIRS}" "${FREENECT_LIBRARY_DIRS}")
  endif()

  MITK_CREATE_MODULE(
    INCLUDE_DIRS
      PUBLIC ${MITK_BIN_DIR}
      PRIVATE ${FREENECT_INCLUDE_DIRS}
    DEPENDS MitkToFHardware
    ADDITIONAL_LIBS ${MITK_FREENECT_LIBRARY}
    AUTOLOAD_WITH MitkToFHardware
  )
endif()
