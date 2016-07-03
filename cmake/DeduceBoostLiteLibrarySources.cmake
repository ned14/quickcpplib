# Deduces a set of includes and sources for ${PROJECT_NAME}
# 
# Inputs:
#  *      PROJECT_NAME: The name of the project
#  * PROJECT_NAMESPACE: Any namespace for the project
#  * Directory structure from ${CMAKE_CURRENT_SOURCE_DIR} is assumed to be:
#    * include/${PROJECT_PATH}
#    * src
#    * test/tests
#    Files matched are *.h, *.hpp, *.ipp, *.c, *.cpp
# 
# Outputs:
#  *                   PROJECT_DIR: PROJECT_NAMESPACE with any :: replaced with a / followed by PROJECT_NAME
#  *          ${PROJECT_NAME}_PATH: ${CMAKE_CURRENT_SOURCE_DIR}
#  *       ${PROJECT_NAME}_HEADERS: Any header files found in include/${PROJECT_PATH}
#  *       ${PROJECT_NAME}_SOURCES: Any source files found in src
#  *         ${PROJECT_NAME}_TESTS: Any source files found in test
#  *   ${PROJECT_NAME}_HEADERS_MD5: The MD5 of the results of 'find . -type f -printf "%t\t%s\t%p\n"' (POSIX) or 'dir /a-d /s' (Windows) for include
#  *   ${PROJECT_NAME}_SOURCES_MD5: The MD5 of the results of 'find . -type f -printf "%t\t%s\t%p\n"' (POSIX) or 'dir /a-d /s' (Windows) for src
#  *     ${PROJECT_NAME}_TESTS_MD5: The MD5 of the results of 'find . -type f -printf "%t\t%s\t%p\n"' (POSIX) or 'dir /a-d /s' (Windows) for test

string(REPLACE "::" "/" PROJECT_DIR ${PROJECT_NAMESPACE})
set(PROJECT_DIR ${PROJECT_DIR}${PROJECT_NAME})
set(${PROJECT_NAME}_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# Only go to the expense of recalculating this stuff if needed
if(${PROJECT_NAME}_HEADERS)
  message(STATUS "Using earlier scan of ${CMAKE_CURRENT_SOURCE_DIR} ...")
else()
  message(STATUS "Recursively scanning ${CMAKE_CURRENT_SOURCE_DIR} for header and source files ...")
  file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}/*.h"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}/*.hpp"
       "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}/*.ipp"
       )
  file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ipp
       )
  file(GLOB_RECURSE ${PROJECT_NAME}_TESTS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
       ${CMAKE_CURRENT_SOURCE_DIR}/test/tests/*.h
       ${CMAKE_CURRENT_SOURCE_DIR}/test/tests/*.hpp
       ${CMAKE_CURRENT_SOURCE_DIR}/test/tests/*.c
       ${CMAKE_CURRENT_SOURCE_DIR}/test/tests/*.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/test/tests/*.ipp
       )
  #message(STATUS "  ${PROJECT_NAME}_HEADERS = ${${PROJECT_NAME}_HEADERS}")
  #message(STATUS "  ${PROJECT_NAME}_SOURCES = ${${PROJECT_NAME}_SOURCES}")
  #message(STATUS "  ${PROJECT_NAME}_TESTS = ${${PROJECT_NAME}_TESTS}")

  # Call source_group() on all items with a common path so project files maintain the file hierarchy
  function(preserve_structure)
    foreach(item ${ARGV})
      set(oldbasepath ${basepath})
      get_filename_component(basepath ${item} DIRECTORY)
      if(NOT basepath STREQUAL oldbasepath)
        string(REPLACE "/" "\\" _basepath ${basepath})
        #message(STATUS "source_group(${_basepath} ${basepath}/.*)")
        source_group("${_basepath}" "${basepath}/.*")
      endif()
    endforeach()
  endfunction()
  preserve_structure(${${PROJECT_NAME}_HEADERS})
  preserve_structure(${${PROJECT_NAME}_SOURCES})
  preserve_structure(${${PROJECT_NAME}_TESTS})

  if(WIN32)
    function(md5_source_tree path outvar)
      execute_process(COMMAND CMD /c dir /b /a-d /s
          OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}\\boostlite_cmake_tempfile.txt"
          WORKING_DIRECTORY "${path}")
      file(MD5 "${CMAKE_CURRENT_BINARY_DIR}/boostlite_cmake_tempfile.txt" MD5)
      set(${outvar} ${MD5} PARENT_SCOPE)
    endfunction()
  else()
    function(md5_source_tree path outvar)
      execute_process(COMMAND find . -type f -printf "%t\t%s\t%p\n"
          OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/boostlite_cmake_tempfile.txt"
          WORKING_DIRECTORY "${path}")
      file(MD5 "${CMAKE_CURRENT_BINARY_DIR}/boostlite_cmake_tempfile.txt" MD5)
      set(${outvar} ${MD5} PARENT_SCOPE)
    endfunction()
  endif()
  md5_source_tree("${${PROJECT_NAME}_PATH}/include/${PROJECT_DIR}" ${PROJECT_NAME}_HEADERS_MD5)
#  md5_source_tree("${${PROJECT_NAME}_PATH}/src" ${PROJECT_NAME}_SOURCES_MD5)
#  md5_source_tree("${${PROJECT_NAME}_PATH}/test" ${PROJECT_NAME}_TESTS_MD5)
endif()
