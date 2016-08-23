cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
# If necessary bring in the Boost lite cmake tooling
list(FIND CMAKE_MODULE_PATH "boost-lite" boost_lite_idx)
if(${boost_lite_idx} GREATER -1)
  return()
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BoostLiteRequireOutOfSourceBuild)
include(BoostLiteUtils)
include(BoostLitePolicies)

# Sets the usual PROJECT_NAME etc
project(boost-lite VERSION 0.1.0.0 LANGUAGES CXX)
# boost-lite does not provide a master header file
set(boost-lite_INTERFACE_DISABLED ON)
# This file should be updated with the last git SHA next commit
UpdateRevisionHppFromGit("${CMAKE_CURRENT_SOURCE_DIR}/include/revision.hpp")
# Setup this cmake environment for this project
include(BoostLiteSetupProject)
# Exclude any embedded CATCH
list_filter(boost-lite_HEADERS EXCLUDE REGEX "include/CATCH")
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
  cxx_lambda_init_captures
  cxx_generic_lambdas
)

# For all possible configurations of this library, add each test
list_filter(boost-lite_TESTS EXCLUDE REGEX "test_.*\\.cpp|bind.cpp")
include(BoostLiteMakeStandardTests)

# The spinlock_tribool test needs OpenMP
if(TARGET boost-lite_hl-spinlock_tribool)
  if(MSVC AND CLANG)
    # Currently doesn't work
  elseif(MSVC)
    target_compile_options(boost-lite_hl-spinlock_tribool PRIVATE /openmp)
  else()
    target_compile_options(boost-lite_hl-spinlock_tribool PRIVATE -fopenmp)
  endif()
endif()

# Make available this library for install and export
include(BoostLiteMakeInstall)
include(BoostLiteMakeExport)
