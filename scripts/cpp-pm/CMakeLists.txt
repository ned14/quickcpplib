cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

include("cmake/HunterVersion.cmake")
project(cpp-pm-test LANGUAGES C CXX)
find_package(Threads)
find_package(OpenSSL)
add_executable(mytest "test.cpp")
