#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "jsoncpp_static" for configuration "Debug"
set_property(TARGET jsoncpp_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(jsoncpp_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/jsoncpp.lib"
  )

list(APPEND _cmake_import_check_targets jsoncpp_static )
list(APPEND _cmake_import_check_files_for_jsoncpp_static "${_IMPORT_PREFIX}/debug/lib/jsoncpp.lib" )

# Import target "jsoncpp_object" for configuration "Debug"
set_property(TARGET jsoncpp_object APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(jsoncpp_object PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_DEBUG ""
  IMPORTED_OBJECTS_DEBUG "${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_reader.cpp.obj;${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_value.cpp.obj;${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_writer.cpp.obj"
  )

list(APPEND _cmake_import_check_targets jsoncpp_object )
list(APPEND _cmake_import_check_files_for_jsoncpp_object "${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_reader.cpp.obj;${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_value.cpp.obj;${_IMPORT_PREFIX}/debug/lib/objects-Debug/jsoncpp_object/json_writer.cpp.obj" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
