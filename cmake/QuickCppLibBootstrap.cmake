cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
# If necessary bring in the quickcpplib cmake tooling
set(quickcpplib_done OFF)
foreach(item ${CMAKE_MODULE_PATH})
  if(item MATCHES "quickcpplib/cmakelib")
    set(quickcpplib_done ON)
  endif()
endforeach()
if(NOT quickcpplib_done)
  # CMAKE_SOURCE_DIR is the very topmost parent cmake project
  # CMAKE_CURRENT_SOURCE_DIR is the current cmake subproject
  
  # If there is a magic .quickcpplib_use_siblings directory above the topmost project, use sibling edition
  if(EXISTS "${CMAKE_SOURCE_DIR}/../.quickcpplib_use_siblings") # AND NOT QUICKCPPLIB_DISABLE_SIBLINGS)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../quickcpplib/cmakelib")
    set(CTEST_QUICKCPPLIB_SCRIPTS "${CMAKE_SOURCE_DIR}/../quickcpplib/scripts")
  else()
    # Place into root binary directory, as we want to share quickcpplib
    find_package(quickcpplib QUIET CONFIG NO_DEFAULT_PATH PATHS "${CMAKE_BINARY_DIR}/quickcpplib/repo")
    if(NOT quickcpplib_FOUND)
      include(ExternalProject)
      message(STATUS "quickcpplib not found, cloning git repository and installing into build directory")
      include(FindGit)
      execute_process(COMMAND ${GIT_EXECUTABLE} clone "https://github.com/ned14/quickcpplib.git" repo
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/quickcpplib"
      )
      if(NOT EXISTS "${CMAKE_BINARY_DIR}/quickcpplib/repo/cmakelib")
        message(FATAL_ERROR "FATAL: Failed to git clone quickcpplib!")
      endif()
    endif()
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_BINARY_DIR}/quickcpplib/repo/cmakelib")
    set(CTEST_QUICKCPPLIB_SCRIPTS "${CMAKE_BINARY_DIR}/quickcpplib/repo/scripts")
  endif()

  # Copy latest version of myself into end user
  file(COPY "${CTEST_QUICKCPPLIB_SCRIPTS}/../cmake/QuickCppLibBootstrap.cmake" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
endif()
