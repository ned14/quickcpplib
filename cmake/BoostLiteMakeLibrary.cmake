# Adds two targets, one a static and the other a shared library for ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_sl: Static library target
#  *  ${PROJECT_NAME}_dl: Dynamic library target
#  * ${PROJECT_NAME}_slm: Static C++ Module target (where supported)
#  * ${PROJECT_NAME}_dlm: Dynamic C++ Module target (where supported)

include(BoostLiteDeduceLibrarySources)
if(WIN32)
  function(check_if_cmake_incomplete target md5 path)
    string(REPLACE "/" "\\" TEMPFILE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}\\boostlite_cmake_tempfile_${target}.txt")
    #string(REPLACE "/" "\\" CMAKE "\"${CMAKE_COMMAND}\")
    set(CMAKE "cmake")
    string(REPLACE "/" "\\" CMAKECACHE "${CMAKE_CURRENT_BINARY_DIR}\\CMakeCache.txt")
    add_custom_command(TARGET ${target} PRE_BUILD
      COMMAND echo Checking if files have been added to ${target} since cmake last auto globbed the source tree ...
\ndir /b /a-d /s > \"${TEMPFILE}\"
\nfor /f \"delims=\" %%a in ('${CMAKE} -E md5sum \"${TEMPFILE}\"') do @set MD5=%%a
\nfor /f \"tokens=1\" %%G IN (\"%MD5%\") DO set MD5=%%G
\nif NOT \"%MD5%\" == \"${md5} \" (echo WARNING cmake needs to be rerun! %MD5% != ${md5} & copy /b \"${CMAKECACHE}\" +,,)
      WORKING_DIRECTORY "${path}"
    )
  endfunction()
else()
  function(check_if_cmake_incomplete target md5 path)
    add_custom_command(TARGET ${target} PRE_BUILD
      COMMAND echo Checking if files have been added to ${target} since cmake last auto globbed the source tree ...
\nfind . -type f -printf \"%t\\t%s\\t%p\\n\" > \"${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/boostlite_cmake_tempfile_${target}.txt\"
\nMD5=$(\"${CMAKE_COMMAND}\" -E md5sum \"${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/boostlite_cmake_tempfile_${target}.txt\" | cut -d " " -f1)
\nif [ \"$MD5\" != \"${md5}\" ]; then echo WARNING cmake needs to be rerun! $MD5 != ${md5}; touch \"${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt\"; fi
      WORKING_DIRECTORY "${path}"
    )
  endfunction()
endif()

# If not set yet, all binaries go into the toplevel bin directory and libraries into lib directory
if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
  set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
endif()

add_library(${PROJECT_NAME}_sl STATIC ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
check_if_cmake_incomplete(${PROJECT_NAME}_sl ${${PROJECT_NAME}_HEADERS_MD5} "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}")

add_library(${PROJECT_NAME}_dl SHARED ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
check_if_cmake_incomplete(${PROJECT_NAME}_dl ${${PROJECT_NAME}_HEADERS_MD5} "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}")

function(all_compile_definitions)
  target_compile_definitions(${PROJECT_NAME}_sl ${ARGV})
  target_compile_definitions(${PROJECT_NAME}_dl ${ARGV})
  if(NOT all_stuff_no_hl AND ${PROJECT_NAME}_hl AND NOT "${ARGV0}" STREQUAL "PRIVATE")
    set(args ${ARGV})
    list(REMOVE_AT args 0)
    target_compile_definitions(${PROJECT_NAME}_hl INTERFACE ${args})
  endif()
endfunction()
function(all_compile_options)
  target_compile_options(${PROJECT_NAME}_sl ${ARGV})
  target_compile_options(${PROJECT_NAME}_dl ${ARGV})
  if(NOT all_stuff_no_hl AND ${PROJECT_NAME}_hl AND NOT "${ARGV0}" STREQUAL "PRIVATE")
    set(args ${ARGV})
    list(REMOVE_AT args 0)
    target_compile_options(${PROJECT_NAME}_hl INTERFACE ${args})
  endif()
endfunction()
function(all_include_directories)
  target_include_directories(${PROJECT_NAME}_sl ${ARGV})
  target_include_directories(${PROJECT_NAME}_dl ${ARGV})
  if(NOT all_stuff_no_hl AND ${PROJECT_NAME}_hl AND NOT "${ARGV0}" STREQUAL "PRIVATE")
    set(args ${ARGV})
    list(REMOVE_AT args 0)
    target_include_directories(${PROJECT_NAME}_hl INTERFACE ${args})
  endif()
endfunction()
function(all_link_libraries)
  target_link_libraries(${PROJECT_NAME}_sl ${ARGV})
  target_link_libraries(${PROJECT_NAME}_dl ${ARGV})
  if(NOT all_stuff_no_hl AND ${PROJECT_NAME}_hl AND NOT "${ARGV0}" STREQUAL "PRIVATE")
    set(args ${ARGV})
    list(REMOVE_AT args 0)
    target_link_libraries(${PROJECT_NAME}_hl INTERFACE ${args})
  endif()
endfunction()

if(WIN32)
  all_compile_definitions(PRIVATE _UNICODE UNICODE)                         # Unicode support
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
endif()

# Temporarily disable _hl
set(all_stuff_no_hl ON)
if(${PROJECT_NAME}_NON_HEADER_ONLY_COMPILE_DEFINITIONS)
  all_compile_definitions(${${PROJECT_NAME}_NON_HEADER_ONLY_COMPILE_DEFINITIONS})
endif()
if(${PROJECT_NAME}_NON_HEADER_ONLY_COMPILE_OPTIONS)
  all_compile_options(${${PROJECT_NAME}_NON_HEADER_ONLY_COMPILE_OPTIONS})
endif()
if(${PROJECT_NAME}_NON_HEADER_ONLY_INCLUDE_DIRECTORIES)
  all_include_directories(${${PROJECT_NAME}_NON_HEADER_ONLY_INCLUDE_DIRECTORIES})
endif()
if(${PROJECT_NAME}_NON_HEADER_ONLY_LINK_LIBRARIES)
  all_link_libraries(${${PROJECT_NAME}_NON_HEADER_ONLY_LINK_LIBRARIES})
endif()
set(${PROJECT_NAME}_hl ${${PROJECT_NAME}_hl_temp})
unset(all_stuff_no_hl)

include(BoostLitePrecompiledHeader)
# Now the config is ready, generate a private precompiled header for
# ${PROJECT_NAME}_INTERFACE and have the sources in ${PROJECT_NAME}_SOURCES
# use the precompiled header UNLESS there is only one source file
# 
# todo

