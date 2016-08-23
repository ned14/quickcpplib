# Set up this cmake environment for this project

# Bring in CTest support
include(CTest)
# Bring in threads, this is after all the 21st century
find_package(Threads)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

# Scan this directory for library source code
include(BoostLiteDeduceLibrarySources)

# Configure an if(CLANG) and if(GCC) like if(MSVC)
if(NOT DEFINED CLANG)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CLANG 1)
  endif()
endif()
if(NOT DEFINED GCC)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(GCC 1)
  elseif(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(GCC 1)
  endif()
endif()
#message(STATUS "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID} MSVC=${MSVC} CLANG=${CLANG} GCC=${GCC}")

