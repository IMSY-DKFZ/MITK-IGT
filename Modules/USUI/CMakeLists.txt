MITK_CREATE_MODULE(
  INCLUDE_DIRS Qmitk
  DEPENDS MitkUS MitkQtWidgetsExt
)
# MITK >= 2026.08 uses AUTOUIC: generated ui_*.h live in the private autogen
# include dir. Expose it so dependent plugins still find them.
# TODO: replace by forward declarations of Ui:: classes in public headers.
if(MODULE_IS_ENABLED)
  get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(_is_multi_config)
    set(_autogen_include_dir ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_TARGET}_autogen/include_$<CONFIG>)
  else()
    set(_autogen_include_dir ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_TARGET}_autogen/include)
  endif()
  target_include_directories(${MODULE_TARGET} PUBLIC
    $<BUILD_INTERFACE:${_autogen_include_dir}>)
endif()
