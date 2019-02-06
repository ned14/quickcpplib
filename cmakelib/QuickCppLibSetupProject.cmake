# Set up this cmake environment for this project

# Bring in CTest support
include(CTest)
# Bring in threads, this is after all the 21st century
find_package(Threads)
link_libraries(${CMAKE_THREAD_LIBS_INIT})
# Find a python installation, if we have one we can do preprocessing
include(FindPythonInterp)

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

# Are we running winclang in MSVC-ish mode or in GCC-ish mode? Unfortunately the MSVC flag
# has the totally wrong setting, it should be on when winclang is in MSVC-ish mode, off otherwise.
if(CMAKE_GENERATOR_TOOLSET MATCHES "LLVM-vs.*" AND NOT MSVC)
  indented_message(WARNING "WARNING: Detected the LLVM toolset in MSVC-ish mode, forcing it to claim to be MSVC")
  set(MSVC 1)
endif()
if(CMAKE_GENERATOR_TOOLSET MATCHES ".*_clang_c2" AND MSVC)
  indented_message(WARNING "WARNING: Detected the clang C2 toolset in GCC-ish mode, stopping it claiming to be MSVC")
  unset(MSVC)
endif()

# On MSVC very annoyingly cmake puts /EHsc and /MD(d) into the global flags which means you
# get a warning when you try to disable exceptions or use the static CRT. I hate to use this
# globally imposed solution, but we are going to hack the global flags to use properties to
# determine whether they are on or off
#
# Create custom properties called CXX_EXCEPTIONS, CXX_RTTI and CXX_STATIC_RUNTIME
# These get placed at global, directory and target scopes
foreach(scope GLOBAL DIRECTORY TARGET)
  define_property(${scope} PROPERTY "CXX_EXCEPTIONS" INHERITED
    BRIEF_DOCS "Enable C++ exceptions, defaults to ON at global scope"
    FULL_DOCS "Not choosing ON nor OFF with exact capitalisation will lead to misoperation!"
  )
  define_property(${scope} PROPERTY "CXX_RTTI" INHERITED
    BRIEF_DOCS "Enable C++ runtime type information, defaults to ON at global scope"
    FULL_DOCS "Not choosing ON nor OFF with exact capitalisation will lead to misoperation!"
  )
  define_property(${scope} PROPERTY "CXX_STATIC_RUNTIME" INHERITED
    BRIEF_DOCS "Enable linking against the static C++ runtime, defaults to OFF at global scope"
    FULL_DOCS "Not choosing ON nor OFF with exact capitalisation will lead to misoperation!"
  )
endforeach()
# Set the default for these properties at global scope. If they are not set per target or
# whatever, the next highest scope will be looked up
set_property(GLOBAL PROPERTY CXX_EXCEPTIONS ON)
set_property(GLOBAL PROPERTY CXX_RTTI ON)
set_property(GLOBAL PROPERTY CXX_STATIC_RUNTIME OFF)
if(MSVC)
  # Purge unconditional use of these flags and remove all the ignored
  # cruft which cmake adds for the LLVM-vs* toolset.
  set(purgelist
    "/MDd"
    "/MD"
    "/EHsc"
    "/GR"
    "/Gm-"
    "-fms-extensions"
    "-fms-compatibility"
    #"-Wall"
    "-frtti"
    "-fexceptions"
    "-gline-tables-only"
    "-fno-inline"
    #"-O0"
  )
  foreach(flag
          CMAKE_C_FLAGS                CMAKE_CXX_FLAGS
          CMAKE_C_FLAGS_DEBUG          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_C_FLAGS_RELEASE        CMAKE_CXX_FLAGS_RELEASE
          CMAKE_C_FLAGS_MINSIZEREL     CMAKE_CXX_FLAGS_MINSIZEREL
          CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
          )
    foreach(item ${purgelist})
      string(REPLACE "${item}"  "" ${flag} "${${flag}}")
    endforeach()
    string(REPLACE "-O0"  "/O0" ${flag} "${${flag}}")
    string(REPLACE "-O1"  "/O1" ${flag} "${${flag}}")
    string(REPLACE "-O2"  "/O2" ${flag} "${${flag}}")
    #message(STATUS "${flag} = ${${flag}}")
  endforeach()
  # Restore those same, but now selected by the properties
  add_compile_options(
    $<$<STREQUAL:$<TARGET_PROPERTY:CXX_EXCEPTIONS>,ON>:/EHsc>
    $<$<STREQUAL:$<TARGET_PROPERTY:CXX_RTTI>,OFF>:/GR->
    $<$<STREQUAL:$<TARGET_PROPERTY:CXX_STATIC_RUNTIME>,OFF>:$<$<CONFIG:Debug>:/MDd>$<$<NOT:$<CONFIG:Debug>>:/MD>>
    $<$<STREQUAL:$<TARGET_PROPERTY:CXX_STATIC_RUNTIME>,ON>:$<$<CONFIG:Debug>:/MTd>$<$<NOT:$<CONFIG:Debug>>:/MT>>
  )
else()
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:$<$<STREQUAL:$<TARGET_PROPERTY:CXX_EXCEPTIONS>,ON>:-fexceptions>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<STREQUAL:$<TARGET_PROPERTY:CXX_RTTI>,ON>:-frtti>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<STREQUAL:$<TARGET_PROPERTY:CXX_EXCEPTIONS>,OFF>:-fno-exceptions>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<STREQUAL:$<TARGET_PROPERTY:CXX_RTTI>,OFF>:-fno-rtti>>
#    $<$<STREQUAL:$<TARGET_PROPERTY:CXX_STATIC_RUNTIME>,ON>:-static>
  )
endif()
# Looks like cmake's toolset for LLVM-vs* has some serious problems
if(CMAKE_GENERATOR_TOOLSET MATCHES "LLVM-vs.*")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Z7")
endif()

# Scan this directory for library source code
include(QuickCppLibParseLibrarySources)

set(SPECIAL_BUILDS)  ## Used to add optional build targets for every build target

# Configure the static analyser build
if(MSVC)
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

if(NOT MSVC)
  # Does this compiler have the santisers?
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_UBSAN)
  if(COMPILER_HAS_UBSAN)
    set(ubsan_COMPILE_FLAGS -fsanitize=undefined -fno-omit-frame-pointer -g)
    set(ubsan_LINK_FLAGS -fsanitize=undefined)
    list(APPEND SPECIAL_BUILDS ubsan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_ASAN)
  if(COMPILER_HAS_ASAN)
    if(COMPILER_HAS_UBSAN)
      set(asan_COMPILE_FLAGS -fsanitize=address ${ubsan_COMPILE_FLAGS})
      set(asan_LINK_FLAGS -fsanitize=address ${ubsan_LINK_FLAGS})
    else()
      set(asan_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
      set(asan_LINK_FLAGS -fsanitize=address)
    endif()
    list(APPEND SPECIAL_BUILDS asan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=memory")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_MSAN)
  if(COMPILER_HAS_MSAN)
    set(msan_COMPILE_FLAGS -fsanitize=memory ${ubsan_COMPILE_FLAGS})
    set(msan_LINK_FLAGS -fsanitize=memory ${ubsan_LINK_FLAGS})
    list(APPEND SPECIAL_BUILDS msan)
  endif()
  set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
  check_cxx_source_compiles("int main() { return 0; }" COMPILER_HAS_TSAN)
  if(COMPILER_HAS_TSAN)
    set(tsan_COMPILE_FLAGS -fsanitize=thread ${ubsan_COMPILE_FLAGS})
    set(tsan_LINK_FLAGS -fsanitize=thread ${ubsan_LINK_FLAGS})
    list(APPEND SPECIAL_BUILDS tsan)
  endif()
  unset(CMAKE_REQUIRED_FLAGS)
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
  if(COMPILER_HAS_STACK_PROTECTOR AND NOT MINGW)
    set(STACK_PROTECTOR_COMPILE_FLAGS $<$<COMPILE_LANGUAGE:CXX>:-fstack-protector-strong>)
  endif()
  add_compile_options(${STACK_PROTECTOR_COMPILE_FLAGS})  ## everything gets this flag
endif()

unset(CLANG_TIDY_EXECUTABLE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy" AND NOT DISABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXECUTABLE "clang-tidy" DOC "Path to clang-tidy executable")
  if(CLANG_TIDY_EXECUTABLE MATCHES "CLANG_TIDY_EXECUTABLE-NOTFOUND")
    indented_message(WARNING "WARNING: .clang-tidy file found for project ${PROJECT_NAME}, yet clang-tidy not on PATH so disabling lint pass")
  else()
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    if(NOT TARGET ${PROJECT_NAME}_lint)
      add_custom_target(${PROJECT_NAME}_lint
        "${CMAKE_COMMAND}" -E make_directory ${PROJECT_NAME}_fixes
        COMMAND "${PYTHON_EXECUTABLE}" "${CTEST_QUICKCPPLIB_SCRIPTS}/run-clang-tidy.py" -clang-tidy-binary="${CLANG_TIDY_EXECUTABLE}" -target-filter="${PROJECT_NAME}_hl--" -header-filter=.* -export-fixes=${PROJECT_NAME}_fixes/fixes.yaml
        COMMAND echo clang-tidy fixes have been written to ${PROJECT_NAME}_fixes, use 'clang-apply-replacements ${PROJECT_NAME}_fixes' to apply.
        COMMENT "Running clang-tidy on ${PROJECT_NAME} ..."
      )
    endif()
    if(NOT TARGET ${PROJECT_NAME}_lint-fix)
      add_custom_target(${PROJECT_NAME}_lint-fix
        "${PYTHON_EXECUTABLE}" "${CTEST_QUICKCPPLIB_SCRIPTS}/run-clang-tidy.py" -clang-tidy-binary="${CLANG_TIDY_EXECUTABLE}" -target-filter="${PROJECT_NAME}_hl--"  -header-filter=.* -fix -format
        COMMENT "Running clang-tidy -fix -format on ${PROJECT_NAME} ..."
      )
    endif()
  endif()
endif()

# Create custom category targets to build all of some kind of thing
if(NOT TARGET _hl)
  add_custom_target(_hl COMMENT "Building all header-only library based code ...")
endif()
if(NOT TARGET _sl)
  add_custom_target(_sl COMMENT "Building all static library based code ...")
endif()
if(NOT TARGET _dl)
  add_custom_target(_dl COMMENT "Building all dynamic library based code ...")
endif()
if(NOT TARGET _docs)
  add_custom_target(_docs COMMENT "Building all documentation ...")
endif()
foreach(special ${SPECIAL_BUILDS})
  if(NOT TARGET _hl-${special})
    add_custom_target(_hl-${special} COMMENT "Building special header-only build ${special} ...")
  endif()
  if(NOT TARGET _sl-${special})
    add_custom_target(_sl-${special} COMMENT "Building special static library build ${special} ...")
  endif()
  if(NOT TARGET _dl-${special})
    add_custom_target(_dl-${special} COMMENT "Building special dynamic library build ${special} ...")
  endif()
endforeach()
