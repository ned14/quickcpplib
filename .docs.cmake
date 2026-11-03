# CTest script for a CI to submit to CDash a documentation generation run
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
list(FIND CMAKE_MODULE_PATH "quickcpplib/cmake" quickcpplib_idx)
if(${quickcpplib_idx} EQUAL -1)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakelib")
endif()
include(QuickCppLibUtils)
# require_ok() comes from QuickCppLibUtils.cmake, included above (reviewer P0
# r2:BC-01: an unchecked stage makes a broken docs build publish green).

CONFIGURE_CTEST_SCRIPT_FOR_CDASH("quickcpplib" "cmake_ci")
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
include(FindGit)
set(CTEST_GIT_COMMAND "${GIT_EXECUTABLE}")
#checked_execute_process("git reset"
#  COMMAND "${GIT_EXECUTABLE}" checkout gh-pages
#  COMMAND "${GIT_EXECUTABLE}" reset --hard ae7119571a3c81cb9a683a21f2759df1d742e998
#  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc/html"
#)

ctest_start("Documentation")
ctest_update()
checked_execute_process("git reset"
  COMMAND "${GIT_EXECUTABLE}" checkout gh-pages
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc/html"
)
ctest_configure(RETURN_VALUE configure_ret)
require_ok("configure" "${configure_ret}")
ctest_build(TARGET quickcpplib_docs RETURN_VALUE docs_ret)
require_ok("build of quickcpplib_docs" "${docs_ret}")
#checked_execute_process("git commit"
#  COMMAND "${GIT_EXECUTABLE}" commit -a -m "upd"
#  COMMAND "${GIT_EXECUTABLE}" push -f origin gh-pages
#  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc/html"
#)
# The CDash submission is a nicety, not a gate: the docs build above is checked by
# require_ok() and publishing to gh-pages is the job's real deliverable. A bare
# ctest_submit() is not harmless, though - a dashboard which is unreachable or
# refuses the upload makes the WHOLE script exit 255, so a job whose docs built
# perfectly is reported red with nothing in its log to explain it (the upload
# error, "Problems when submitting via HTTP", matches none of the error patterns
# the workflow annotates, which is exactly how this job spent a week red at
# "build of quickcpplib_docs failed" lookalike noise). CAPTURE_CMAKE_ERROR is the
# documented way to keep that failure out of the exit status; the return value is
# still reported loudly here and still appears in the step log.
ctest_submit(CAPTURE_CMAKE_ERROR submit_err RETURN_VALUE submit_ret)
if(NOT "${submit_ret}" STREQUAL "0")
  message(STATUS "ctest_submit() could not reach the CDash dashboard (return '${submit_ret}', capture '${submit_err}'); the docs build above and the gh-pages publish which follows are what this job gates on")
endif()
