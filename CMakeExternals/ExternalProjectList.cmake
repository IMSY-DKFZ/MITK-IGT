mitkFunctionAddExternalProject(NAME OpenIGTLink ON)
if(MITK_USE_OpenCV)
  mitkFunctionAddExternalProject(NAME OpenCV ON PREPEND)
endif()
