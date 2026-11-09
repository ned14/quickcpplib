cmake_minimum_required(VERSION 3.16)
project(quickcpplib_install_consumer CXX)

# A downstream consumer of an INSTALLED quickcpplib, used by the install-consumer
# lane in .github/workflows/ci.yml.
#
# Every other consumer test in this tree uses add_subdirectory() (see
# .ci/floor-consumer), which exercises the source tree and not the thing users
# actually get: the installed package, its Config file, its exported targets and
# its include layout. Nothing tested that, so the export surface could have been
# broken in any release and the first report would have come from a user.
#
# Configure it with -DCMAKE_PREFIX_PATH=<install prefix>; find_package() below
# then has to succeed without any hint beyond that path, exactly as a consumer
# would need it to.
find_package(quickcpplib CONFIG REQUIRED)

if(NOT TARGET quickcpplib::hl)
  message(FATAL_ERROR "the installed package did not export quickcpplib::hl")
endif()

add_executable(install_consumer main.cpp)
target_link_libraries(install_consumer PRIVATE quickcpplib::hl)

# The header the consumer includes is a real public one rather than a detail
# header, and it is included by its installed path, so a missing or mis-located
# install directory fails here too.
