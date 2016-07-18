cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
# If necessary bring in the Boost lite cmake tooling
list(FIND CMAKE_MODULE_PATH "boost-lite" boost_lite_idx)
if(${boost_lite_idx} GREATER -1)
  return()
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BoostLiteRequireOutOfSourceBuild)
include(BoostLiteUtils)

# Sets the usual PROJECT_NAME etc
project(boost-lite VERSION 0.1.0.0 LANGUAGES CXX)
# boost-lite does not provide a master header file
set(boost-lite_INTERFACE_DISABLED ON)

# Update this header file with the current git SHA and last timestamp
UpdateRevisionHppFromGit("${CMAKE_CURRENT_SOURCE_DIR}/include/revision.hpp")
# Make an interface only library so dependent CMakeLists can bring in this header-only library
include(BoostLiteMakeHeaderOnlyLibrary)

# Set the standard definitions for these libraries and bring in the all_* helper functions
include(BoostLiteApplyDefaultDefinitions)
# Set the C++ features this library requires
all_compile_features(PUBLIC
  cxx_alias_templates
  cxx_variadic_templates
  cxx_noexcept
  cxx_constexpr
  cxx_thread_local
)

# For all possible configurations of this library, add each test
list_filter(boost-lite_TESTS EXCLUDE REGEX "test_.*\\.cpp|bind.cpp")
include(BoostLiteMakeStandardTests)

# Make available this library for install and export
include(BoostLiteMakeInstall)
include(BoostLiteMakeExport)
