# CTest script for a CI to submit to CDash a run of configuration,
# building and testing
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
list(FIND CMAKE_MODULE_PATH "quickcpplib" quickcpplib_idx)
if(${quickcpplib_idx} EQUAL -1)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakelib")
  set(CTEST_QUICKCPPLIB_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
endif()
include(QuickCppLibUtils)


CONFIGURE_CTEST_SCRIPT_FOR_CDASH("quickcpplib" "prebuilt")
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
include(FindGit)
set(CTEST_GIT_COMMAND "${GIT_EXECUTABLE}")

ctest_start("Experimental")
ctest_update()
ctest_configure()
ctest_build()
if(WIN32)
  if(EXISTS "prebuilt/bin/Release/quickcpplib_dl-0.1-Windows-x64-Release.dll")
    checked_execute_process("Tarring up binaries 1"
      COMMAND "${CMAKE_COMMAND}" -E make_directory quickcpplib/prebuilt/bin/Release
      COMMAND "${CMAKE_COMMAND}" -E make_directory quickcpplib/prebuilt/lib/Release
      COMMAND xcopy doc quickcpplib\\doc\\ /s /q
      COMMAND xcopy include quickcpplib\\include\\ /s /q
    )
    checked_execute_process("Tarring up binaries 2"
      COMMAND "${CMAKE_COMMAND}" -E copy Readme.md quickcpplib/
    )
    checked_execute_process("Tarring up binaries 3"
      COMMAND "${CMAKE_COMMAND}" -E copy prebuilt/lib/Release/quickcpplib_sl-0.1-Windows-x64-Release.lib quickcpplib/prebuilt/lib/Release/
    )
    checked_execute_process("Tarring up binaries 4"
      COMMAND "${CMAKE_COMMAND}" -E copy prebuilt/lib/Release/quickcpplib_dl-0.1-Windows-x64-Release.lib quickcpplib/prebuilt/lib/Release/
    )
    checked_execute_process("Tarring up binaries 5"
      COMMAND "${CMAKE_COMMAND}" -E copy prebuilt/bin/Release/quickcpplib_dl-0.1-Windows-x64-Release.dll quickcpplib/prebuilt/bin/Release/
    )
    checked_execute_process("Tarring up binaries final"
      COMMAND "${CMAKE_COMMAND}" -E tar cfv quickcpplib-v0.1-binaries-win64.zip --format=zip quickcpplib/
    )
    get_filename_component(toupload quickcpplib-v0.1-binaries-win64.zip ABSOLUTE)
  endif()
else()
  if(EXISTS "prebuilt/lib/libquickcpplib_dl-0.1-Linux-x86_64-Release.so")
    checked_execute_process("Tarring up binaries"
      COMMAND mkdir quickcpplib
      COMMAND cp -a doc quickcpplib/
      COMMAND cp -a include quickcpplib/
      COMMAND cp -a Readme.md quickcpplib/
      COMMAND cp -a --parents prebuilt/lib/libquickcpplib_sl-0.1-Linux-x86_64-Release.a quickcpplib/
      COMMAND cp -a --parents prebuilt/lib/libquickcpplib_dl-0.1-Linux-x86_64-Release.so quickcpplib/
      COMMAND "${CMAKE_COMMAND}" -E tar cfz quickcpplib-v0.1-binaries-linux-x64.tgz quickcpplib
    )
    get_filename_component(toupload quickcpplib-v0.1-binaries-linux-x64.tgz ABSOLUTE)
  endif()
  if(EXISTS "prebuilt/lib/libquickcpplib_dl-0.1-Linux-armhf-Release.so")
    checked_execute_process("Tarring up binaries"
      COMMAND mkdir quickcpplib
      COMMAND cp -a doc quickcpplib/
      COMMAND cp -a include quickcpplib/
      COMMAND cp -a Readme.md quickcpplib/
      COMMAND cp -a --parents prebuilt/lib/libquickcpplib_sl-0.1-Linux-armhf-Release.a quickcpplib/
      COMMAND cp -a --parents prebuilt/lib/libquickcpplib_dl-0.1-Linux-armhf-Release.so quickcpplib/
      COMMAND "${CMAKE_COMMAND}" -E tar cfz quickcpplib-v0.1-binaries-linux-armhf.tgz quickcpplib
    )
    get_filename_component(toupload quickcpplib-v0.1-binaries-linux-armhf.tgz ABSOLUTE)
  endif()
  if(EXISTS "prebuilt/lib/libquickcpplib_dl-0.1-Darwin-x86_64-Release.dylib")
    checked_execute_process("Tarring up binaries"
      COMMAND mkdir quickcpplib
      COMMAND cp -a doc quickcpplib/
      COMMAND cp -a include quickcpplib/
      COMMAND cp -a Readme.md quickcpplib/
      COMMAND mkdir -p quickcpplib/prebuilt/lib
      COMMAND cp -a prebuilt/lib/libquickcpplib_sl-0.1-Darwin-x86_64-Release.a quickcpplib/prebuilt/lib/
      COMMAND cp -a prebuilt/lib/libquickcpplib_dl-0.1-Darwin-x86_64-Release.dylib quickcpplib/prebuilt/lib/
      COMMAND "${CMAKE_COMMAND}" -E tar cfz quickcpplib-v0.1-binaries-darwin-x64.tgz quickcpplib
    )
    get_filename_component(toupload quickcpplib-v0.1-binaries-darwin-x64.tgz ABSOLUTE)
  endif()
endif()
ctest_test(RETURN_VALUE retval EXCLUDE "signal_guard")  # pending the signal_guard reimplementation
merge_junit_results_into_ctest_xml()
if(EXISTS "${toupload}")
  ctest_upload(FILES "${toupload}")
endif()
ctest_submit()
if(NOT retval EQUAL 0)
  message(FATAL_ERROR "FATAL: Running tests exited with ${retval}")
endif()
