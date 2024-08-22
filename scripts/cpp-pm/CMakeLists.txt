cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

option(BUILD_TESTING "" OFF)

#find_package(OpenSSL)
include("cmake/HunterVersion.cmake")
project(cpp-pm-test LANGUAGES C CXX)
find_package(Threads)
add_executable(mytest "test.cpp")
