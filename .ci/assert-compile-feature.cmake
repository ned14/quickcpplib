# Assert that the C++ compile feature quickcpplib requires actually reached a
# downstream consumer.
#
#   cmake -DFEATURES_FILE=<file> -DEXPECT_FEATURE=cxx_std_17 \
#         [-DFORBIDDEN_FEATURE=cxx_std_14] -P .ci/assert-compile-feature.cmake
#
# INTERFACE_COMPILE_FEATURES can only be read inside a configured project, and
# script mode (-P) has no targets, so .ci/floor-consumer/CMakeLists.txt captures
# the property into FEATURES_FILE with get_target_property(); this script does
# the assertion half. Exits non-zero (via FATAL_ERROR) unless the feature is
# required and, when given, the forbidden one is not.
cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED FEATURES_FILE OR NOT DEFINED EXPECT_FEATURE)
  message(FATAL_ERROR "usage: cmake -DFEATURES_FILE=<file> -DEXPECT_FEATURE=<feature> [-DFORBIDDEN_FEATURE=<feature>] -P ${CMAKE_CURRENT_LIST_FILE}")
endif()
if(NOT EXISTS "${FEATURES_FILE}")
  message(FATAL_ERROR "FATAL: ${FEATURES_FILE} does not exist: the consumer configure did not capture INTERFACE_COMPILE_FEATURES")
endif()

file(READ "${FEATURES_FILE}" quickcpplib_features)
string(REGEX REPLACE "[\r\n]+" " " quickcpplib_features "${quickcpplib_features}")
string(STRIP "${quickcpplib_features}" quickcpplib_features)

list(FIND quickcpplib_features "${EXPECT_FEATURE}" quickcpplib_expect_idx)
if(quickcpplib_expect_idx EQUAL -1)
  message(FATAL_ERROR "FATAL: quickcpplib's interface compile features are '${quickcpplib_features}', which does not require ${EXPECT_FEATURE}")
endif()

if(DEFINED FORBIDDEN_FEATURE AND NOT FORBIDDEN_FEATURE STREQUAL "")
  list(FIND quickcpplib_features "${FORBIDDEN_FEATURE}" quickcpplib_forbidden_idx)
  if(NOT quickcpplib_forbidden_idx EQUAL -1)
    message(FATAL_ERROR "FATAL: quickcpplib's interface compile features are '${quickcpplib_features}', which must not require ${FORBIDDEN_FEATURE} here")
  endif()
endif()

message(STATUS "OK: quickcpplib's interface compile features are '${quickcpplib_features}': requires ${EXPECT_FEATURE}")
