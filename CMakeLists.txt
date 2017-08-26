cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
# If necessary bring in the quickcpplib cmake tooling
list(FIND CMAKE_MODULE_PATH "quickcpplib" quickcpplib_idx)
if(${quickcpplib_idx} GREATER -1)
  return()
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakelib")
include(QuickCppLibRequireOutOfSourceBuild)
include(QuickCppLibUtils)
include(QuickCppLibPolicies)
indented_message(STATUS "NOTE: CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# Sets the usual PROJECT_NAME etc
project(quickcpplib VERSION 0.1.0.0 LANGUAGES C CXX)
# quickcpplib does not provide a master header file
set(quickcpplib_INTERFACE_DISABLED ON)
# Also set a *cmake* namespace for this project
unset(PROJECT_NAMESPACE)
# Setup this cmake environment for this project
include(QuickCppLibSetupProject)
if(NOT PROJECT_IS_DEPENDENCY)
  # This file should be updated with the last git SHA next commit
  UpdateRevisionHppFromGit("${CMAKE_CURRENT_SOURCE_DIR}/include/revision.hpp")
endif()
# Exclude any embedded CATCH
list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/CATCH")
# Make an interface only library so dependent CMakeLists can bring in this header-only library
include(QuickCppLibMakeHeaderOnlyLibrary)
# Make a docs target
include(QuickCppLibMakeDoxygen)

# Set the standard definitions for these libraries and bring in the all_* helper functions
include(QuickCppLibApplyDefaultDefinitions)
# Set the C++ features this library requires
all_compile_features(PUBLIC
  cxx_alias_templates
  cxx_variadic_templates
  cxx_noexcept
  cxx_constexpr
#  cxx_thread_local          ## Technically a requirement, but not enforced due to Xcode 7
  cxx_lambda_init_captures
  cxx_generic_lambdas
)
# On Linux, things using me must link to libdl
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  all_link_libraries(PUBLIC dl)
endif()
# On FreeBSD, things using me must link to libexecinfo
if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  all_link_libraries(PUBLIC execinfo)
endif()

# For all possible configurations of this library, add each test
list_filter(quickcpplib_TESTS EXCLUDE REGEX "test_.*\\.cpp")
include(QuickCppLibMakeStandardTests)

# On MSVC the ringbuffer_log test needs to emit debug info to work
if(TARGET quickcpplib_hl--ringbuffer_log AND MSVC)
  target_compile_options(quickcpplib_hl--ringbuffer_log PRIVATE /Oy- /Z7)
  _target_link_options(quickcpplib_hl--ringbuffer_log /DEBUG)
endif()
# The spinlock_tribool test needs OpenMP
if(TARGET quickcpplib_hl--spinlock_tribool)
  target_uses_openmp(quickcpplib_hl--spinlock_tribool)
endif()

# Cache this library's auto scanned sources for later reuse
include(QuickCppLibCacheLibrarySources)

# Make available this library for install and export
include(QuickCppLibMakeInstall)
include(QuickCppLibMakeExport)
