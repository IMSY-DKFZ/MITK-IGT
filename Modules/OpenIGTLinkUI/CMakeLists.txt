MITK_CREATE_MODULE(
  INCLUDE_DIRS Qmitk
  DEPENDS MitkOpenIGTLink MitkQtWidgetsExt MitkPersistence
  #GENERATED_CPP ${TOOL_GUI_CPPS} ${TOOL_CPPS}
)
# MITK >= 2026.08 uses AUTOUIC: generated ui_*.h live in the private autogen
# include dir. Expose it so dependent plugins still find them.
# TODO: replace by forward declarations of Ui:: classes in public headers.
if(MODULE_IS_ENABLED)
  target_include_directories(${MODULE_TARGET} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/${MODULE_TARGET}_autogen/include_$<CONFIG>>)
endif()

## create IGTUI config
#configure_file(mitkIGTUIConfig.h.in ${PROJECT_BINARY_DIR}/mitkIGTUIConfig.h @ONLY)
