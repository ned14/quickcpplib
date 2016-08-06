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

# Apply these sources to all library targets
function(all_sources)
  if(TARGET ${PROJECT_NAME}_sl)
    target_sources(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_sources(${PROJECT_NAME}_dl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_sources(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these target properties to all library targets
function(all_target_properties)
  if(TARGET ${PROJECT_NAME}_sl)
    set_target_properties(${PROJECT_NAME}_sl ${ARGV})
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    set_target_properties(${PROJECT_NAME}_dl ${ARGV})
  endif()
endfunction()

all_target_properties(PROPERTIES
  # Place all libraries into the lib directory
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  # Place all binaries into the bin directory
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# Originally we were setting these on all targets, but the CMP0063 warning
# just refused to go away despite us setting it in BoostLitePolicies. So,
# I give up and just set these properties on the shared library target only.
if(TARGET ${PROJECT_NAME}_dl)
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES
    # Only explicitly exported symbols are to be available from objects
    CXX_VISIBILITY_PRESET hidden
    # Reduce linking times by eliminating inlines from being linked
    VISIBILITY_INLINES_HIDDEN ON
  )
endif()

if(WIN32)
  all_compile_definitions(PUBLIC _UNICODE UNICODE)                          # Unicode support
endif()

if(MSVC AND NOT CLANG)
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
  if(TARGET ${PROJECT_NAME}_hl)
    all_compile_options(INTERFACE /W4)                                      # Stronger warnings
  endif()
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
  if(TARGET ${PROJECT_NAME}_hl)
    all_compile_options(INTERFACE -Wall -Wextra)                            # Stronger warnings
  endif()
  if(CLANG)
    # Make use of clang's ability to warn on bad doxygen markup
    all_compile_options(PRIVATE
      -Wdocumentation
      -fcomment-block-commands=raceguarantees,complexity,exceptionmodel
    )
  endif()
endif()
