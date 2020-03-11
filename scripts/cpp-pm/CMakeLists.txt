cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

include("cmake/HunterVersion.cmake")
project(cpp-pm-test LANGUAGES C CXX)
include(FindThreads)
add_executable(mytest "test.cpp")
