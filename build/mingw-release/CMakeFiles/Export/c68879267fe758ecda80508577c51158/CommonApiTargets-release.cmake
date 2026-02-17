#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "CommonApi::CommonApi" for configuration "Release"
set_property(TARGET CommonApi::CommonApi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(CommonApi::CommonApi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libCommonApi.a"
  )

list(APPEND _cmake_import_check_targets CommonApi::CommonApi )
list(APPEND _cmake_import_check_files_for_CommonApi::CommonApi "${_IMPORT_PREFIX}/lib/libCommonApi.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
