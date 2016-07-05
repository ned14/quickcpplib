# Adds two targets, one a static and the other a shared library for ${PROJECT_NAME}

include(DeduceBoostLiteLibrarySources)
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

add_library(${PROJECT_NAME}_sl STATIC ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
check_if_cmake_incomplete(${PROJECT_NAME}_sl ${${PROJECT_NAME}_HEADERS_MD5} "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}")

add_library(${PROJECT_NAME}_dl SHARED ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
check_if_cmake_incomplete(${PROJECT_NAME}_dl ${${PROJECT_NAME}_HEADERS_MD5} "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_DIR}")

function(all_compile_definitions)
  target_compile_definitions(${PROJECT_NAME}_sl ${ARGV})
  target_compile_definitions(${PROJECT_NAME}_dl ${ARGV})
endfunction()
function(all_compile_options)
  target_compile_options(${PROJECT_NAME}_sl ${ARGV})
  target_compile_options(${PROJECT_NAME}_dl ${ARGV})
endfunction()
function(all_include_directories)
  target_include_directories(${PROJECT_NAME}_sl ${ARGV})
  target_include_directories(${PROJECT_NAME}_dl ${ARGV})
endfunction()
function(all_link_libraries)
  target_link_libraries(${PROJECT_NAME}_sl ${ARGV})
  target_link_libraries(${PROJECT_NAME}_dl ${ARGV})
endfunction()

all_compile_definitions(INTERFACE BOOST_AFIO_HEADERS_ONLY=0)                # Anyone using these libraries is not using the header only variant
if(WIN32)
  all_compile_definitions(PRIVATE _UNICODE UNICODE)                         # Unicode support
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
endif()
