cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
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
option(QUICKCPPLIB_REQUIRE_CXX20 "Add cxx17 compile feature to the target which in turn makes byte-lite and gsl-lite optional." OFF)
cmake_dependent_option(QUICKCPPLIB_REQUIRE_CXX17 "Add cxx17 compile feature to the target which in turn makes byte-lite optional." OFF "NOT QUICKCPPLIB_REQUIRE_CXX20" OFF)
# quickcpplib does not provide a master header file
set(quickcpplib_INTERFACE_DISABLED ON)
# Also set a *cmake* namespace for this project
unset(PROJECT_NAMESPACE)
# Setup this cmake environment for this project
include(QuickCppLibSetupProject)
if(NOT PROJECT_IS_DEPENDENCY)
  # This file should be updated with the last git SHA next commit
  UpdateRevisionHppFromGit("${CMAKE_CURRENT_SOURCE_DIR}/include/quickcpplib/revision.hpp")
endif()
# Exclude any embedded CATCH
list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/CATCH")
if (QUICKCPPLIB_REQUIRE_CXX20)
  list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/span-lite/")
endif()
if (QUICKCPPLIB_REQUIRE_CXX17 OR QUICKCPPLIB_REQUIRE_CXX20)
  list_filter(quickcpplib_HEADERS EXCLUDE REGEX "include/quickcpplib/byte/")
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
elseif(QUICKCPPLIB_REQUIRE_CXX17)
  all_compile_features(PUBLIC cxx_std_17)
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
endif()
# On Linux, things using me must link to libdl
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  all_link_libraries(PUBLIC dl rt)
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
# fast_atomic_128 may need to link in libatomic
check_cxx_source_compiles("
int main() {
#if defined(__AVX__)
i am not a number;
#elif defined(_MSC_VER)
i am not a number;
#elif defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
i am not a number;
#elif defined(__aarch64__) || defined(_M_ARM64)
i am not a number;
#else
return 0;
#endif
}
" FAST_ATOMIC_128_NEEDS_LIBATOMIC)
if(FAST_ATOMIC_128_NEEDS_LIBATOMIC)
  all_link_libraries(PUBLIC atomic)
endif()

# Cache this library's auto scanned sources for later reuse
include(QuickCppLibCacheLibrarySources)

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
include(QuickCppLibMakeExport)
