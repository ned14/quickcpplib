# Parse a set of includes and sources for ${PROJECT_NAME}
# 
# Inputs:
#  *      PROJECT_NAME: The name of the project
#  * PROJECT_NAMESPACE: Any namespace for the project
#  * Directory structure from ${CMAKE_CURRENT_SOURCE_DIR} is assumed to be:
#    * include/${PROJECT_PATH}
#    * src
#    * test
#    Files matched are *.h, *.hpp, *.ipp, *.c, *.cpp *.cxx
#    Files excluded are anything with a .quickcpplib file in its root
# 
# Outputs:
#  *                   PROJECT_DIR: PROJECT_NAMESPACE with any :: replaced with a / followed by PROJECT_NAME
#  *         PROJECT_IS_DEPENDENCY: ON if this this project is a dependency of a higher level project
#
# Cached outputs:
#  *               ${PROJECT_NAME}_PATH: ${CMAKE_CURRENT_SOURCE_DIR}
#  *          ${PROJECT_NAME}_INTERFACE: The master interface PCHable header file ${PROJECT_DIR}/${PROJECT_NAME}.hpp, plus any sources which need to be compiled into any consumers
#  *            ${PROJECT_NAME}_HEADERS: Any header files found in include
#  *            ${PROJECT_NAME}_SOURCES: Any source files found in src
#  *              ${PROJECT_NAME}_TESTS: Any source files found in test not in a special category
#  *      ${PROJECT_NAME}_COMPILE_TESTS: Any source files found in test or example which must compile
#  * ${PROJECT_NAME}_COMPILE_FAIL_TESTS: Any source files found in test which must fail to compile

if(DEFINED PROJECT_NAMESPACE)
  string(REPLACE "::" "/" PROJECT_DIR ${PROJECT_NAMESPACE})
else()
  set(PROJECT_DIR)
endif()
set(PROJECT_DIR ${PROJECT_DIR}${PROJECT_NAME})
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(PROJECT_IS_DEPENDENCY OFF)
else()
  set(PROJECT_IS_DEPENDENCY ON)
endif()

# Prune any items with a .quickcpplib in their root directory
function(prune_quickcpplib_libraries boostishlist fileslist)
  if(${boostishlist})
    # As an optimisation for deep nested trees of boostish libraries,
    # use the boostishlist to prune itself first
    set(boostishlist_)
    foreach(boostish ${${boostishlist}})
      string(LENGTH "${boostish}" len)
      math(EXPR len "${len} - 9")
      string(SUBSTRING "${boostish}" 0 ${len} boostish_)
      escape_string_into_regex(boostish_ "${boostish_}")
      list(APPEND boostishlist_ ${boostish_})
    endforeach()
    foreach(boostish ${boostishlist_})
      list_filter(boostishlist_ EXCLUDE REGEX "${boostish}.+")
    endforeach()
    # Now we have an optimal boostishlist
    set(fileslist_ ${${fileslist}})
    foreach(boostish ${boostishlist_})
      list_filter(fileslist_ EXCLUDE REGEX "${boostish}")
    endforeach()
    set(${fileslist} ${fileslist_} PARENT_SCOPE)
  endif()
endfunction()

# Check a cached scan file's directory timestamps,
# if any are stale then delete the cached scan file
function(delete_stale_cached_scan_file path)
  if(EXISTS "${path}.cache")
    include("${path}.cache")
    set(plsdelete FALSE)
    set(idx 0)
    foreach(var ${ARGN})
      foreach(dir ${${var}_DIRECTORIES})
        if(NOT idx)
          set(dirts1 "${dir}")
          set(idx 1)
        else()
          file(TIMESTAMP "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" dirts2)
          if(NOT dirts1 STREQUAL dirts2)
            set(plsdelete TRUE)
          endif()
          set(idx 0)
        endif()
      endforeach()
    endforeach()
    if(plsdelete)
      indented_message(STATUS "${path} is stale, deleting")
      file(REMOVE "${path}")
    endif()
  endif()
endfunction()

delete_stale_cached_scan_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/headers.cmake" "${PROJECT_NAME}_HEADERS")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/headers.cmake")
  indented_message(STATUS "Using cached scan of project ${PROJECT_NAME} headers ...")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/interface.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/interface.cmake")
  endif()
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/headers.cmake")
else()
  indented_message(STATUS "Cached scan of project ${PROJECT_NAME} headers not found! Starting scan ...")
  if(NOT ${PROJECT_NAME}_INTERFACE_DISABLED)
    set(${PROJECT_NAME}_INTERFACE "${PROJECT_DIR}/${PROJECT_NAME}.hpp")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/${${PROJECT_NAME}_INTERFACE}")
      indented_message(FATAL_ERROR "FATAL: No master interface header file found at include/${${PROJECT_NAME}_INTERFACE}. "
                          "You need a master interface header file at that location if you are to make available "
                          "your library as a C++ Module or as a precompiled header. If your library can never "
                          "support a master interface header file, set ${PROJECT_NAME}_INTERFACE_DISABLED to ON."
      )
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}/${PROJECT_NAME}.ixx")
      set(${PROJECT_NAME}_INTERFACE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}/${PROJECT_NAME}.ixx")
    endif()
  endif()
  indented_message(STATUS "  Recursively scanning ${CMAKE_CURRENT_SOURCE_DIR}/include for header files ...")
  # cmake glob is unfortunately very slow on deep directory hierarchies, so we glob
  # recursively everything we need at once and extract out from that giant list what we need
  file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/.quickcpplib"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/*.ipp"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/*.natvis"
       )
  set(${PROJECT_NAME}_HEADERS_FILTER ${${PROJECT_NAME}_HEADERS})
  list_filter(${PROJECT_NAME}_HEADERS_FILTER INCLUDE REGEX "\\.quickcpplib$")
  prune_quickcpplib_libraries(${PROJECT_NAME}_HEADERS_FILTER ${PROJECT_NAME}_HEADERS)
  unset(${PROJECT_NAME}_HEADERS_FILTER)

  # Identify source files for the interface library to be linked into all consumers
  # MSVC has a cool feature where .natvis files tell the debugger how to display a type
  # We append this to the main interface header because we want all .natvis in all the
  # dependencies brought into anything we link
  if(MSVC)
    set(${PROJECT_NAME}_INTERFACE_SOURCES ${${PROJECT_NAME}_HEADERS})
    list_filter(${PROJECT_NAME}_INTERFACE_SOURCES INCLUDE REGEX "\\.natvis$")
    list(APPEND ${PROJECT_NAME}_INTERFACE ${${PROJECT_NAME}_INTERFACE_SOURCES})
    unset(${PROJECT_NAME}_INTERFACE_SOURCES)
  endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
  delete_stale_cached_scan_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake" "${PROJECT_NAME}_SOURCES")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake")
    indented_message(STATUS "Using cached scan of project ${PROJECT_NAME} sources ...")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake")
  else()
    indented_message(STATUS "Cached scan of project ${PROJECT_NAME} sources not found! Starting scan ...")
    file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/.quickcpplib"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
         "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ipp"
         )
    set(${PROJECT_NAME}_SOURCES_FILTER ${${PROJECT_NAME}_SOURCES})
    list_filter(${PROJECT_NAME}_SOURCES_FILTER INCLUDE REGEX "\\.quickcpplib$")
    prune_quickcpplib_libraries(${PROJECT_NAME}_SOURCES_FILTER ${PROJECT_NAME}_SOURCES)
    unset(${PROJECT_NAME}_SOURCES_FILTER)
  endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
  delete_stale_cached_scan_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake" "${PROJECT_NAME}_TESTS" "${PROJECT_NAME}_COMPILE_TESTS" "${PROJECT_NAME}_COMPILE_FAIL_TESTS")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake")
    indented_message(STATUS "Using cached scan of project ${PROJECT_NAME} tests ...")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake")
  else()
    indented_message(STATUS "Cached scan of project ${PROJECT_NAME} tests not found! Starting scan ...")
    file(GLOB_RECURSE ${PROJECT_NAME}_TESTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/.quickcpplib"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.hpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.c"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cxx"
         "${CMAKE_CURRENT_SOURCE_DIR}/test/*.ipp"
         )
    set(${PROJECT_NAME}_TESTS_FILTER ${${PROJECT_NAME}_TESTS})
    list_filter(${PROJECT_NAME}_TESTS_FILTER INCLUDE REGEX "\\.quickcpplib$")
    prune_quickcpplib_libraries(${PROJECT_NAME}_TESTS_FILTER ${PROJECT_NAME}_TESTS)
    unset(${PROJECT_NAME}_TESTS_FILTER)
    
    set(${PROJECT_NAME}_COMPILE_FAIL_TESTS ${${PROJECT_NAME}_TESTS})
    list_filter(${PROJECT_NAME}_COMPILE_FAIL_TESTS INCLUDE REGEX "/compile-fail/")
    list_filter(${PROJECT_NAME}_TESTS EXCLUDE REGEX "/compile-fail/")
    
    file(GLOB_RECURSE ${PROJECT_NAME}_COMPILE_TESTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/.quickcpplib"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.hpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.c"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.cxx"
         "${CMAKE_CURRENT_SOURCE_DIR}/example/*.ipp"
         )
    set(${PROJECT_NAME}_TESTS_FILTER ${${PROJECT_NAME}_COMPILE_TESTS})
    list_filter(${PROJECT_NAME}_TESTS_FILTER INCLUDE REGEX "\\.quickcpplib$")
    prune_quickcpplib_libraries(${PROJECT_NAME}_TESTS_FILTER ${PROJECT_NAME}_COMPILE_TESTS)
    unset(${PROJECT_NAME}_TESTS_FILTER)
    set(${PROJECT_NAME}_COMPILE_TESTS2 ${${PROJECT_NAME}_TESTS})
    list_filter(${PROJECT_NAME}_COMPILE_TESTS2 INCLUDE REGEX "/compile-success/")
    list_filter(${PROJECT_NAME}_TESTS EXCLUDE REGEX "/compile-success/")
    list(APPEND ${PROJECT_NAME}_COMPILE_TESTS ${${PROJECT_NAME}_COMPILE_TESTS2})
    unset(${PROJECT_NAME}_COMPILE_TESTS2)
  endif()
endif()
