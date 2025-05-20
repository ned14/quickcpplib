cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
# If necessary bring in the quickcpplib cmake tooling
list(FIND CMAKE_MODULE_PATH "quickcpplib" quickcpplib_idx)
if(${quickcpplib_idx} GREATER -1)
  return()
endif()
option(QUICKCPPLIB_ENABLE_DOXYGEN_DOCS_BUILD_SUPPORT "Enable doxygen docs build support" ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakelib")
include(QuickCppLibRequireOutOfSourceBuild)
include(QuickCppLibUtils)
include(QuickCppLibPolicies)
ensure_git_subrepo("${CMAKE_CURRENT_SOURCE_DIR}/include/quickcpplib/span-lite/include" "https://github.com/martinmoene/span-lite.git")
ensure_git_subrepo("${CMAKE_CURRENT_SOURCE_DIR}/include/quickcpplib/optional/optional.hpp" "https://github.com/akrzemi1/Optional.git" optional)
ensure_git_subrepo("${CMAKE_CURRENT_SOURCE_DIR}/include/quickcpplib/byte/include" "https://github.com/martinmoene/byte-lite.git" byte)

# Sets the usual PROJECT_NAME etc
project(quickcpplib VERSION 0.1.0.0 LANGUAGES C CXX)
include(CMakeDependentOption)
set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_17 FALSE)
set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20 FALSE)
if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20 TRUE)
  set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_17 TRUE)
elseif(CMAKE_CXX_STANDARD GREATER_EQUAL 17)
  set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_17 TRUE)
endif()
cmake_dependent_option(QUICKCPPLIB_REQUIRE_CXX20 "Add cxx20 compile feature to the target which in turn makes byte-lite, span-lite and akrzemi1/Optional optional." OFF "NOT CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20" ON)
cmake_dependent_option(QUICKCPPLIB_REQUIRE_CXX17 "Add cxx17 compile feature to the target which in turn makes akrzemi1/Optional optional." OFF "NOT CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_17" ON)
cmake_dependent_option(QUICKCPPLIB_USE_SYSTEM_BYTE_LITE "Whether to use embedded nonstd::byte-lite. Used by various package managers such as vcpkg." OFF "NOT QUICKCPPLIB_REQUIRE_CXX17" OFF)
cmake_dependent_option(QUICKCPPLIB_USE_SYSTEM_SPAN_LITE "Whether to use embedded nonstd::span-lite. Used by various package managers such as vcpkg." OFF "NOT QUICKCPPLIB_REQUIRE_CXX20" OFF)
# quickcpplib does not provide a master header file
set(quickcpplib_INTERFACE_DISABLED ON)
# Also set a *cmake* namespace for this project
unset(PROJECT_NAMESPACE)
# Setup this cmake environment for this project
include(QuickCppLibSetupProject)
if(NOT ${PROJECT_NAME}_IS_DEPENDENCY)
  # This file should be updated with the last git SHA next commit
  UpdateRevisionHppFromGit("${CMAKE_CURRENT_SOURCE_DIR}/include/quickcpplib/revision.hpp")
endif()
# Exclude any embedded CATCH
list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/CATCH")
list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/detail/config\\.hpp")
if (QUICKCPPLIB_USE_SYSTEM_SPAN_LITE OR QUICKCPPLIB_REQUIRE_CXX20)
  list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/span-lite/")
endif()
if (QUICKCPPLIB_USE_SYSTEM_BYTE_LITE OR QUICKCPPLIB_REQUIRE_CXX17)
  list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/byte/")
endif()
if (QUICKCPPLIB_REQUIRE_CXX17)
  list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/optional/")
endif()
# Make an interface only library so dependent CMakeLists can bring in this header-only library
include(QuickCppLibMakeHeaderOnlyLibrary)

if(QUICKCPPLIB_ENABLE_DOXYGEN_DOCS_BUILD_SUPPORT)
  # Make a docs target
  include(QuickCppLibMakeDoxygen)
endif()

# Set the standard definitions for these libraries and bring in the all_* helper functions
include(QuickCppLibApplyDefaultDefinitions)
# Set the C++ features this library requires
if(QUICKCPPLIB_REQUIRE_CXX20)
  all_compile_features(PUBLIC cxx_std_20)
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 202002L)
elseif(QUICKCPPLIB_REQUIRE_CXX17)
  all_compile_features(PUBLIC cxx_std_17)
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 201703L)
else()
  all_compile_features(PUBLIC
    cxx_alias_templates
    cxx_variadic_templates
    cxx_noexcept
    cxx_constexpr
    #cxx_thread_local          ## Technically a requirement, but not enforced due to Xcode 7
    cxx_lambda_init_captures
    cxx_generic_lambdas
  )
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 201402L)
endif()

if(QUICKCPPLIB_USE_SYSTEM_BYTE_LITE)
  find_package(byte-lite REQUIRED)
  all_link_libraries(PUBLIC nonstd::byte-lite)
endif()
if(QUICKCPPLIB_USE_SYSTEM_SPAN_LITE)
  find_package(span-lite REQUIRED)
  all_link_libraries(PUBLIC nonstd::span-lite)
endif()

# these need to be kept in sync with the definitions in cmake/config.hpp.in
all_compile_definitions(PUBLIC
  $<BUILD_INTERFACE:QUICKCPPLIB_REQUIRE_CXX_STANDARD=${QUICKCPPLIB_REQUIRE_CXX_STANDARD}>
  $<BUILD_INTERFACE:QUICKCPPLIB_USE_SYSTEM_BYTE_LITE=$<BOOL:${QUICKCPPLIB_USE_SYSTEM_BYTE_LITE}>>
  $<BUILD_INTERFACE:QUICKCPPLIB_USE_SYSTEM_SPAN_LITE=$<BOOL:${QUICKCPPLIB_USE_SYSTEM_SPAN_LITE}>>
)

# On Linux, things using me must link to libdl
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  all_link_libraries(PUBLIC dl)
endif()
# On FreeBSD, things using me must link to libexecinfo
if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  all_link_libraries(PUBLIC execinfo)
endif()
if (NOT (WIN32 OR APPLE))
  include(CheckLibraryExists)
  check_library_exists(rt timer_create "" HAVE_LIB_RT)
  if (HAVE_LIB_RT)
    all_link_libraries(PUBLIC rt)
  endif()
endif()

# For all possible configurations of this library, add each test
list_filter(quickcpplib_TESTS EXCLUDE REGEX "test_.*\\.cpp")
include(QuickCppLibMakeStandardTests)

# On MSVC the ringbuffer_log test needs to emit debug info to work
if(TARGET quickcpplib_hl--ringbuffer_log AND MSVC)
  target_compile_options(quickcpplib_hl--ringbuffer_log PRIVATE /Oy- /Z7)
  _target_link_options(quickcpplib_hl--ringbuffer_log PRIVATE /DEBUG)
endif()
# The spinlock_tribool test needs OpenMP
if(TARGET quickcpplib_hl--spinlock_tribool)
  target_uses_openmp(quickcpplib_hl--spinlock_tribool)
endif()
# MSVC's concepts implementation blow up unless permissive is off
if(MSVC AND NOT CLANG)
  foreach(target ${quickcpplib_TEST_TARGETS})
    if(NOT target MATCHES "spinlock_tribool")
      target_compile_options(${target} PRIVATE /permissive-)
    endif()
  endforeach()
endif()

# Cache this library's auto scanned sources for later reuse
include(QuickCppLibCacheLibrarySources)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/include/quickcpplib/detail")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/include/quickcpplib/detail/config.hpp")

# Make available this library for install and export
include(GNUInstallDirs)
set(PROJECT_CONFIG_TEMPLATE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmakelib/quickcpplibConfig.override.cmake.in")
set(quickcpplib_CMAKELIB_DIR "${CMAKE_INSTALL_DATADIR}/cmakelib")
set(quickcpplib_SCRIPTS_DIR "${CMAKE_INSTALL_DATADIR}/scripts")
set(PROJECT_PACKAGE_PATH_VARS quickcpplib_CMAKELIB_DIR quickcpplib_SCRIPTS_DIR)
include(QuickCppLibMakeInstall)
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/cmakelib/"
  COMPONENT headers
  DESTINATION "${quickcpplib_CMAKELIB_DIR}"
)
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/scripts/"
  COMPONENT headers
  DESTINATION "${quickcpplib_SCRIPTS_DIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/include/quickcpplib/detail/config.hpp"
  COMPONENT headers
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/quickcpplib/detail/"
)
include(QuickCppLibMakeExport)
