set(MODULE_TESTS
  #mitkThreadedToFRawDataReconstructionTest.cpp
  mitkAbstractToFDeviceFactoryTest.cpp
  mitkToFCameraMITKPlayerDeviceTest.cpp
  mitkToFCameraMITKPlayerDeviceFactoryTest.cpp
  mitkToFImageCsvWriterTest.cpp
  mitkToFImageGrabberTest.cpp
  mitkToFImageRecorderTest.cpp
  #mitkToFImageRecorderFilterTest.cpp
  mitkToFImageWriterTest.cpp
  mitkToFNrrdImageWriterTest.cpp
  mitkToFOpenCVImageGrabberTest.cpp
)

if(MITK_USE_TOF_KINECT)
  list(APPEND MODULE_TESTS
    mitkKinectBasicsTest.cpp
    mitkKinectHardwareTest.cpp
    mitkKinectPlaybackTest.cpp
  )
endif()

set(MODULE_CUSTOM_TESTS
  mitkPlayerLoadAndRenderDepthDataTest.cpp
  mitkPlayerLoadAndRenderRGBDataTest.cpp
)
