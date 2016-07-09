# Apply these compile definitions to all library targets
function(all_compile_definitions)
  if(TARGET ${PROJECT_NAME}_sl)
    target_compile_definitions(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_definitions(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl)
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
  if(TARGET ${PROJECT_NAME}_hl)
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
  if(TARGET ${PROJECT_NAME}_hl)
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
  if(TARGET ${PROJECT_NAME}_hl)
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
  if(TARGET ${PROJECT_NAME}_hl)
    target_link_libraries(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

if(WIN32)
  all_compile_definitions(PUBLIC _UNICODE UNICODE)                          # Unicode support
endif()

if(MSVC)
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
endif()
