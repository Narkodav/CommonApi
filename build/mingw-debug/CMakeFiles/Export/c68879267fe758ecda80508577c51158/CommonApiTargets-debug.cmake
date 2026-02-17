#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "CommonApi::CommonApi" for configuration "Debug"
set_property(TARGET CommonApi::CommonApi APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(CommonApi::CommonApi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libCommonApi.a"
  )

list(APPEND _cmake_import_check_targets CommonApi::CommonApi )
list(APPEND _cmake_import_check_files_for_CommonApi::CommonApi "${_IMPORT_PREFIX}/lib/libCommonApi.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
