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

# Sets the usual PROJECT_NAME etc
project(quickcpplib VERSION 0.1.0.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Language standard floor
#
# quickcpplib defaults to C++17 and later. C++14 exists only as an explicit opt-in
# for a consumer that must build at 14: Outcome does, and sets
# QUICKCPPLIB_MIN_CXX_STANDARD=14 before finding quickcpplib. In that mode the two
# std::byte headers (mem_flush_loads_stores.hpp, detach_cast.hpp) are disabled and
# the C++17-only tests are excluded - loudly, see the message below.
#
# QUICKCPPLIB_MIN_CXX_STANDARD is the switch: 17 (default) or 14 (the opt-in
# compatibility mode). It is a cache variable precisely so that a parent can set
# it before add_subdirectory(quickcpplib)/find_package(quickcpplib).
#
# The floor itself is enforced as a CMake COMPILE FEATURE on the public/interface
# targets, further down, and deliberately not as a configure-time probe of the
# standard in force. A compile feature is what CMake is for here: it PROPAGATES
# to consumers, so a parent project which sets CMAKE_CXX_STANDARD=14 still gets
# anything linking quickcpplib::hl built at C++17 - CMake raises just that
# target's standard and the parent's own TUs stay at 14. Probing the standard in
# force and failing the configure instead both duplicated CMake's own job and
# rejected the very configurations this library is meant to support (clang,
# AppleClang and MSVC all default below C++17).
set(QUICKCPPLIB_MIN_CXX_STANDARD 17 CACHE STRING
  "The minimum C++ standard quickcpplib may require: 17 (default) or 14 (opt-in compatibility mode; Outcome sets 14)")
if(NOT QUICKCPPLIB_MIN_CXX_STANDARD MATCHES "^[0-9]+$" OR QUICKCPPLIB_MIN_CXX_STANDARD LESS 14)
  message(FATAL_ERROR "QUICKCPPLIB_MIN_CXX_STANDARD=${QUICKCPPLIB_MIN_CXX_STANDARD} is invalid: set it to 14 (the opt-in compatibility mode) or 17 or later.")
endif()

include(CMakeDependentOption)
set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20 FALSE)
if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  set(CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20 TRUE)
endif()
cmake_dependent_option(QUICKCPPLIB_REQUIRE_CXX20 "Add cxx20 compile feature to the target which in turn makes span-lite optional." OFF "NOT CMAKE_CXX_STANDARD_IS_GREATER_EQUAL_20" ON)
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
# Make an interface only library so dependent CMakeLists can bring in this header-only library
include(QuickCppLibMakeHeaderOnlyLibrary)

if(QUICKCPPLIB_ENABLE_DOXYGEN_DOCS_BUILD_SUPPORT)
  # Make a docs target
  include(QuickCppLibMakeDoxygen)
endif()

# Set the standard definitions for these libraries and bring in the all_* helper functions
include(QuickCppLibApplyDefaultDefinitions)
# Set the C++ features this library requires. QUICKCPPLIB_MIN_CXX_STANDARD is the
# floor, and this is where it is enforced: all_compile_features() turns these into
# INTERFACE compile features, so they propagate to anything linking
# quickcpplib::hl and raise that consumer target's standard as needed.
# QUICKCPPLIB_REQUIRE_CXX20 alone selects the C++20 facilities (span-lite is the
# fallback for std::span), which is feature selection, not a floor.
if(QUICKCPPLIB_REQUIRE_CXX20)
  all_compile_features(PUBLIC cxx_std_20)
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 202002L)
elseif(QUICKCPPLIB_MIN_CXX_STANDARD GREATER_EQUAL 17)
  all_compile_features(PUBLIC cxx_std_17)
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 201703L)
else()
  # The explicit opt-in C++14 mode. This is what a consumer gets by setting
  # QUICKCPPLIB_MIN_CXX_STANDARD=14; it requires C++14 and deliberately does not
  # force C++17 on the consumer's targets. cxx_std_14 subsumes the individual
  # C++14 features this used to list one by one (cxx_alias_templates, ...) and
  # cxx_thread_local with them - the Xcode 7-era carve-out for that one is gone,
  # as no supported compiler lacks it.
  all_compile_features(PUBLIC cxx_std_14)
  set(QUICKCPPLIB_REQUIRE_CXX_STANDARD 201402L)
endif()

if(QUICKCPPLIB_USE_SYSTEM_SPAN_LITE)
  find_package(span-lite REQUIRED)
  all_link_libraries(PUBLIC nonstd::span-lite)
endif()

# these need to be kept in sync with the definitions in cmake/config.hpp.in
all_compile_definitions(PUBLIC
  $<BUILD_INTERFACE:QUICKCPPLIB_REQUIRE_CXX_STANDARD=${QUICKCPPLIB_REQUIRE_CXX_STANDARD}>
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
if (NOT (WIN32 OR APPLE OR ANDROID))
  include(CheckLibraryExists)
  check_library_exists(rt timer_create "" HAVE_LIB_RT)
  if (HAVE_LIB_RT)
    # The extra generator expression check is because under cross compilation
    # it would appear ANDROID isn't always set
    all_link_libraries(PUBLIC $<$<PLATFORM_ID:Linux>:rt>)
  endif()
endif()

# For all possible configurations of this library, add each test.
# NOTE: there used to be a `list_filter(quickcpplib_TESTS EXCLUDE REGEX
# "test_.*\\.cpp")` here. It existed only to hide the AFIO-era harness test TUs
# (test_guard.cpp, test_import.cpp, test_message.cpp) from the build; those files
# have been deleted (they #included a header path that does not exist and
# exercised BOOST_BINDLIB_* macros defined nowhere, so they had not compiled in
# years). The filter is deliberately NOT replaced: a filter that selects nothing
# is exactly what kept them invisible, so the scan now lists whatever real
# test_*.cpp files exist, now or later.
# In the opt-in C++14 mode, drop the test sources that cannot compile below
# C++17. This is deliberately loud: a silently shorter test list is exactly how
# the old C++14 path rotted, so every exclusion is named at configure time. The
# unfiltered scan is kept in quickcpplib_TESTS_SCANNED for the committed cache.
if(QUICKCPPLIB_MIN_CXX_STANDARD LESS 17)
  set(quickcpplib_TESTS_SCANNED ${quickcpplib_TESTS})
  set(quickcpplib_CXX14_SKIPPED_TESTS
    "test/bitwise_trie.cpp"            # <memory_resource>/std::pmr is C++17
    "test/casts.cpp"                   # detach_cast/in_place_detach_attach need std::byte
    "test/mem_flush_loads_stores.cpp"  # mem_flush_loads_stores.hpp needs std::byte
  )
  set(quickcpplib_CXX14_SKIPPED_TEST_NAMES)
  foreach(quickcpplib_skip ${quickcpplib_CXX14_SKIPPED_TESTS})
    list(REMOVE_ITEM quickcpplib_TESTS "${quickcpplib_skip}")
    get_filename_component(quickcpplib_skip_name "${quickcpplib_skip}" NAME_WE)
    list(APPEND quickcpplib_CXX14_SKIPPED_TEST_NAMES "${quickcpplib_skip_name}")
  endforeach()
  message(STATUS "quickcpplib: opt-in C++14 mode (QUICKCPPLIB_MIN_CXX_STANDARD=14) - SKIPPING ${quickcpplib_CXX14_SKIPPED_TEST_NAMES} because they require C++17 (std::byte or std::pmr); see CMakeLists.txt for the per-test reason. This is a deliberate exclusion, not a missing test.")
endif()
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
