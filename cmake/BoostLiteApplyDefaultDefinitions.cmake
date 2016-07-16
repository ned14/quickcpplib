# Apply these compile definitions to all library targets
function(all_compile_definitions)
  if(TARGET ${PROJECT_NAME}_sl)
    target_compile_definitions(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_definitions(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_compile_definitions(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these compile options to all library targets
function(all_compile_options)
  if(TARGET ${PROJECT_NAME}_sl)
    target_compile_options(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_options(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_compile_options(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these compile features to all library targets
function(all_compile_features)
  if(TARGET ${PROJECT_NAME}_sl)
    target_compile_features(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_features(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_compile_features(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these include directories to all library targets
function(all_include_directories)
  if(TARGET ${PROJECT_NAME}_sl)
    target_include_directories(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_include_directories(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_include_directories(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these link libraries to all library targets
function(all_link_libraries)
  if(TARGET ${PROJECT_NAME}_sl)
    target_link_libraries(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_link_libraries(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_link_libraries(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these target libraries to all library targets
function(all_target_properties)
  if(TARGET ${PROJECT_NAME}_sl)
    set_target_properties(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    set_target_properties(${PROJECT_NAME}_dl ${ARGV})
  endif()
endfunction()

# Add this include directory to anyone using this library
all_include_directories(INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
# Place all binaries into the bin directory
all_target_properties(PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(WIN32)
  all_compile_definitions(PUBLIC _UNICODE UNICODE)                          # Unicode support
endif()

if(MSVC)
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
  if(TARGET ${PROJECT_NAME}_hl)
    all_compile_options(INTERFACE /W4)                                      # Stronger warnings
  endif()
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
  if(TARGET ${PROJECT_NAME}_hl)
    all_compile_options(INTERFACE -Wall -Wextra)                            # Stronger warnings
  endif()
endif()
