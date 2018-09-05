# The entries in the mitk_modules list must be
# ordered according to their dependencies.

set(MITK_MODULES
  Core
  CommandLine
  AppUtil
  RDF
  LegacyIO
  DataTypesExt
  Annotation
  LegacyGL
  AlgorithmsExt
  MapperExt
  DICOMReader
  DICOMReaderServices
  DICOMQI
  DICOMTesting
  SceneSerializationBase
  PlanarFigure
  ImageDenoising
  ImageExtraction
  SceneSerialization
  Gizmo
  GraphAlgorithms
  Multilabel
  ImageStatistics
  ContourModel
  SurfaceInterpolation
  Segmentation
  PlanarFigureSegmentation
  QtWidgets
  QtWidgetsExt
  Chart
  SegmentationUI
  MatchPointRegistration
  MatchPointRegistrationUI
  Classification
  GPGPU
  OpenIGTLink
  IGTBase
  IGT
  CameraCalibration
  OpenCL
  OpenCVVideoSupport
  QtOverlays
  ToFHardware
  ToFProcessing
  ToFUI
  PhotoacousticsHardware
  PhotoacousticsAlgorithms
  PhotoacousticsLib
  US
  USUI
  DicomUI
  Remeshing
  Python
  QtPython
  Persistence
  OpenIGTLinkUI
  IGTUI
  DicomRT
  RTUI
  IOExt
  XNAT
  TubeGraph
  BiophotonicsHardware
  DiffusionImaging
  TumorInvasionAnalysis
  BoundingShape
  RenderWindowManager
  RenderWindowManagerUI
  CEST
  BasicImageProcessing
  ModelFit
  ModelFitUI
  Pharmacokinetics
  PharmacokineticsUI
)

if(MITK_ENABLE_PIC_READER)
  list(APPEND MITK_MODULES IpPicSupportIO)
endif()
