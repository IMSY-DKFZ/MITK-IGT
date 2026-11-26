MITK_CREATE_MODULE_TESTS()
if(TARGET ${TESTDRIVER})
  mitk_use_modules(TARGET ${TESTDRIVER} PACKAGES VTK|TestingRendering)

  mitkAddCustomModuleRenderingTest(mitkPlayerLoadAndRenderDepthDataTest_KinectDepthImage #testname
    mitkPlayerLoadAndRenderDepthDataTest #testclassname
    Kinect_LiverPhantom_DistanceImage.nrrd #input image
    -V ${MITK_DATA_DIR}/ToF-Data/ReferenceScreenshots/Kinect_LiverPhantom_DistanceImage640x480REF.png #reference image
  )

  mitkAddCustomModuleRenderingTest(mitkPlayerLoadAndRenderRGBDataTest_KinectRGBImage mitkPlayerLoadAndRenderRGBDataTest Kinect_LiverPhantom_RGBImage.nrrd -V ${MITK_DATA_DIR}/ToF-Data/ReferenceScreenshots/Kinect_LiverPhantom_RGBImage640x480REF.png)
endif()

if(TARGET ${TESTDRIVER} AND MITK_USE_TOF_KINECT)
  set_tests_properties(mitkKinectHardwareTest mitkKinectPlaybackTest PROPERTIES SKIP_RETURN_CODE 77)

  set(MITK_KINECT_FAKENECT_RECORDING "" CACHE PATH
    "Recording made with fakenect-record, played by mitkKinectPlaybackTest. Empty skips the test.")
  set(MITK_KINECT_FAKENECT_LIBRARY "" CACHE FILEPATH
    "fakenect library from the libfreenect build, loaded by mitkKinectPlaybackTest instead of libfreenect.")

  if(MITK_KINECT_FAKENECT_RECORDING AND MITK_KINECT_FAKENECT_LIBRARY)
    # ENVIRONMENT already holds MITK's PATH, whose separators would swallow
    # appended entries, so everything here goes through ENVIRONMENT_MODIFICATION.
    set(_fakenect_environment "FAKENECT_PATH=set:${MITK_KINECT_FAKENECT_RECORDING}")
    if(WIN32)
      # fakenect exports the libfreenect API, so a copy named freenect.dll
      # that is found first on PATH replaces the real library.
      set(_fakenect_dir "${CMAKE_CURRENT_BINARY_DIR}/fakenect")
      configure_file("${MITK_KINECT_FAKENECT_LIBRARY}" "${_fakenect_dir}/freenect.dll" COPYONLY)
      list(APPEND _fakenect_environment "PATH=path_list_prepend:${_fakenect_dir}")
    else()
      list(APPEND _fakenect_environment "LD_PRELOAD=set:${MITK_KINECT_FAKENECT_LIBRARY}")
    endif()
    set_property(TEST mitkKinectPlaybackTest PROPERTY ENVIRONMENT_MODIFICATION ${_fakenect_environment} "FAKENECT_LOOP=set:1")

    # fakenect reads FAKENECT_LOOP once per process, so playing to the end of
    # the recording needs a test run of its own.
    add_test(NAME mitkKinectPlaybackEndOfRecordingTest COMMAND ${TESTDRIVER} mitkKinectPlaybackTest)
    get_test_property(mitkKinectPlaybackTest ENVIRONMENT _mitk_test_environment)
    set_tests_properties(mitkKinectPlaybackEndOfRecordingTest PROPERTIES
      ENVIRONMENT "${_mitk_test_environment}"
      ENVIRONMENT_MODIFICATION "${_fakenect_environment};FAKENECT_LOOP=set:0"
      SKIP_RETURN_CODE 77)
  endif()
endif()
