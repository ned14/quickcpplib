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

set(SPECIAL_BUILDS)  ## Used to add optional build targets for every build target

# Configure the static analyser build
if(MSVC AND NOT CLANG)
  list(APPEND SPECIAL_BUILDS sa)
  set(sa_COMPILE_FLAGS /analyze /analyze:stacksize 262144)  ## Chosen because OS X enforces this limit on stack usage
  #set(sa_LINK_FLAGS)
endif()
option(ENABLE_CLANG_STATIC_ANALYZER "Enable the clang static analyser as a special build. Be aware almost certainly the object files generated will not link, but it can be useful to enable this in a pinch rather than setting up scan-build" OFF)
if(CLANG AND ENABLE_CLANG_STATIC_ANALYZER)
  list(APPEND SPECIAL_BUILDS sa)
  set(sa_COMPILE_FLAGS --analyze)
  #set(sa_LINK_FLAGS)
endif()

if(GCC OR CLANG)
  # Does this compiler have the santisers?
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_ASAN)
  if(COMPILER_HAS_ASAN)
    set(asan_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
    set(asan_LINK_FLAGS -fsanitize=address)
    list(APPEND SPECIAL_BUILDS asan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=memory")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_MSAN)
  if(COMPILER_HAS_MSAN)
    set(msan_COMPILE_FLAGS -fsanitize=memory -fno-omit-frame-pointer -g)
    set(msan_LINK_FLAGS -fsanitize=memory)
    list(APPEND SPECIAL_BUILDS msan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_TSAN)
  if(COMPILER_HAS_TSAN)
    set(tsan_COMPILE_FLAGS -fsanitize=thread -fno-omit-frame-pointer -g)
    set(tsan_LINK_FLAGS -fsanitize=thread)
    list(APPEND SPECIAL_BUILDS tsan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_UBSAN)
  if(COMPILER_HAS_UBSAN)
    set(ubsan_COMPILE_FLAGS -fsanitize=undefined -fno-omit-frame-pointer -g)
    set(ubsan_LINK_FLAGS -fsanitize=undefined)
    list(APPEND SPECIAL_BUILDS ubsan)
  endif()
  foreach(special ${SPECIAL_BUILDS})
    set(${PROJECT_NAME}_${special}_TARGETS)
  endforeach()

  # This fellow probably ought to be compiled into every executable
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=safestack")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_SAFESTACK)
  if(COMPILER_HAS_SAFESTACK)
    set(SAFESTACK_COMPILE_FLAGS -fsanitize=safestack)
    set(SAFESTACKLINK_FLAGS -fsanitize=safestack)
  endif()
  # This fellow probably should just always be turned on period
  set(CMAKE_REQUIRED_FLAGS "-fstack-protector-strong")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_STACK_PROTECTOR)
  if(COMPILER_HAS_STACK_PROTECTOR)
    set(STACK_PROTECTOR_COMPILE_FLAGS -fstack-protector-strong)
  endif()
  add_compile_options(${STACK_PROTECTOR_COMPILE_FLAGS})  ## everything gets this flag
endif()
