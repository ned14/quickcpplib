# Adds two targets, one a static and the other a shared library for ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_sl: Static library target
#  *  ${PROJECT_NAME}_dl: Dynamic library target
#  *  ${PROJECT_NAME}_sl-Xsan: with sanitiser
#  *  ${PROJECT_NAME}_dl-Xsan: with sanitiser

if(NOT DEFINED ${PROJECT_NAME}_HEADERS)
  message(FATAL_ERROR "FATAL: QuickCppLibSetupProject has not been included yet.")
endif()
if(NOT DEFINED ${PROJECT_NAME}_SOURCES)
  message(FATAL_ERROR "FATAL: Cannot include QuickCppLibMakeLibrary without a src directory. "
                      "Perhaps you meant QuickCppLibMakeHeaderOnlyLibrary?")
endif()

add_library(${PROJECT_NAME}_sl STATIC ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
##if(PROJECT_IS_DEPENDENCY)
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES EXCLUDE_FROM_ALL ON)
##endif()
add_dependencies(_sl ${PROJECT_NAME}_sl)
list(APPEND ${PROJECT_NAME}_TARGETS ${PROJECT_NAME}_sl)
foreach(special ${SPECIAL_BUILDS})
  add_library(${PROJECT_NAME}_sl-${special} STATIC EXCLUDE_FROM_ALL ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
  target_compile_options(${PROJECT_NAME}_sl-${special} PRIVATE ${${special}_COMPILE_FLAGS})
  list(APPEND ${PROJECT_NAME}_${special}_TARGETS ${PROJECT_NAME}_sl-${special})
endforeach()
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_sl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-$<PLATFORM_ID>-$(Platform)-$<CONFIG>"
  )
elseif(CMAKE_GENERATOR MATCHES "Xcode")
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_sl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-$<PLATFORM_ID>-${CMAKE_SYSTEM_PROCESSOR}-$<CONFIG>"
  )
else()
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_sl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}"
  )
endif()
add_library(${PROJECT_NAMESPACE}${PROJECT_NAME}::sl ALIAS ${PROJECT_NAME}_sl)

add_library(${PROJECT_NAME}_dl SHARED ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
##if(PROJECT_IS_DEPENDENCY)
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES EXCLUDE_FROM_ALL ON)
##endif()
add_dependencies(_dl ${PROJECT_NAME}_dl)
list(APPEND ${PROJECT_NAME}_TARGETS ${PROJECT_NAME}_dl)
foreach(special ${SPECIAL_BUILDS})
  add_library(${PROJECT_NAME}_dl-${special} SHARED EXCLUDE_FROM_ALL ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
  if(DEFINED ${special}_LINK_FLAGS)
    _target_link_options(${PROJECT_NAME}_dl-${special} ${${special}_LINK_FLAGS})
  endif()
  target_compile_options(${PROJECT_NAME}_dl-${special} PRIVATE ${${special}_COMPILE_FLAGS})
  list(APPEND ${PROJECT_NAME}_${special}_TARGETS ${PROJECT_NAME}_dl-${special})
endforeach()
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_dl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-$<PLATFORM_ID>-$(Platform)-$<CONFIG>"
  )
elseif(CMAKE_GENERATOR MATCHES "Xcode")
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_dl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-$<PLATFORM_ID>-${CMAKE_SYSTEM_PROCESSOR}-$<CONFIG>"
  )
else()
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_dl-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}"
  )
endif()
add_library(${PROJECT_NAMESPACE}${PROJECT_NAME}::dl ALIAS ${PROJECT_NAME}_dl)

# Check headers for C++ Modules support
if(MSVC AND MSVC_VERSION VERSION_GREATER 1900)  # VS2015
  # Parse the front of each header file looking for ^import .*;
  # todo
endif()

include(QuickCppLibPrecompiledHeader)
# Now the config is ready, generate a private precompiled header for
# ${PROJECT_NAME}_INTERFACE and have the sources in ${PROJECT_NAME}_SOURCES
# use the precompiled header UNLESS there is only one source file
# 
# todo

