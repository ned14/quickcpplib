cmake_minimum_required(VERSION 3.16)
project(quickcpplib_floor_consumer CXX)

# A minimal downstream consumer of quickcpplib, used by the C++ standard floor
# lanes in .github/workflows/ci.yml.
#
# quickcpplib's floor is a COMPILE FEATURE on its interface targets, so it
# propagates: whatever CMAKE_CXX_STANDARD this project is configured with (the
# floor lane passes 14), std_byte_consumer below is still built at C++17 because
# it links quickcpplib::hl. That is the whole point of the feature replacing the
# old configure-time probe, which simply refused to configure here.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." quickcpplib)

# The lanes assert on this, so capture it. get_target_property() exists only in
# project mode - a `cmake -P` script has no targets - hence the file: the
# assertion itself lives in .ci/assert-compile-feature.cmake.
get_target_property(quickcpplib_hl_compile_features quickcpplib_hl INTERFACE_COMPILE_FEATURES)
message(STATUS "floor-consumer: quickcpplib_hl INTERFACE_COMPILE_FEATURES=${quickcpplib_hl_compile_features}")
message(STATUS "floor-consumer: CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} QUICKCPPLIB_MIN_CXX_STANDARD=${QUICKCPPLIB_MIN_CXX_STANDARD}")
file(WRITE "${CMAKE_BINARY_DIR}/quickcpplib-interface-compile-features.txt"
  "${quickcpplib_hl_compile_features}\n")

# Unconditionally uses std::byte, a C++17 facility. In the default mode this must
# build and run even though this project asks for C++14 (the feature raises the
# target); in the opt-in C++14 mode its build is expected to FAIL.
add_executable(std_byte_consumer std_byte.cpp)
target_link_libraries(std_byte_consumer PRIVATE quickcpplib::hl)
