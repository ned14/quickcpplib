# Apply these compile definitions to all library targets
function(all_compile_definitions)
  if(TARGET ${PROJECT_NAME}_sl)
    target_compile_definitions(${PROJECT_NAME}_sl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_compile_definitions(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_definitions(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_compile_definitions(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
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
    foreach(special ${SPECIAL_BUILDS})
      target_compile_options(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_options(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_compile_options(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
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
    foreach(special ${SPECIAL_BUILDS})
      target_compile_features(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_compile_features(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_compile_features(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
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
    foreach(special ${SPECIAL_BUILDS})
      target_include_directories(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_include_directories(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_include_directories(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
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
    foreach(special ${SPECIAL_BUILDS})
      target_link_libraries(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_link_libraries(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_link_libraries(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
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
    foreach(special ${SPECIAL_BUILDS})
      target_sources(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    target_sources(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      target_sources(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_hl AND NOT ${ARGV0} STREQUAL "PRIVATE")
    list(REMOVE_AT ARGV 0)
    list(INSERT ARGV 0 INTERFACE)
    target_sources(${PROJECT_NAME}_hl ${ARGV})
  endif()
endfunction()

# Apply these target properties to all library targets with real outputs
function(all_target_output_properties)
  if(TARGET ${PROJECT_NAME}_sl)
    set_target_properties(${PROJECT_NAME}_sl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      set_target_properties(${PROJECT_NAME}_sl-${special} ${ARGV})
    endforeach()
  endif()
  if(TARGET ${PROJECT_NAME}_dl)
    set_target_properties(${PROJECT_NAME}_dl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      set_target_properties(${PROJECT_NAME}_dl-${special} ${ARGV})
    endforeach()
  endif()
endfunction()

# Apply these target properties to all library targets
function(all_target_properties)
  all_target_output_properties(${ARGV})
  if(TARGET ${PROJECT_NAME}_hl)
    set_target_properties(${PROJECT_NAME}_hl ${ARGV})
    foreach(special ${SPECIAL_BUILDS})
      set_target_properties(${PROJECT_NAME}_hl-${special} ${ARGV})
    endforeach()
  endif()
endfunction()

if(NOT PROJECT_IS_DEPENDENCY AND CMAKE_GENERATOR MATCHES "Visual Studio")
  # Ensure project files maintain the file hierarchy
  function(preserve_structure dependency)
    if(dependency)
      set(PATH "${${dependency}_PATH}")
    else()
      set(PATH "${${PROJECT_NAME}_PATH}")
    endif()
    foreach(item ${ARGN})
      get_filename_component(basepath ${item} DIRECTORY)
      string(REPLACE "/" "\\" _basepath ${basepath})
      if(dependency)
        set(_basepath "dependency\\${dependency}\\${_basepath}")
      endif()
      #indented_message(STATUS "source_group(${_basepath} FILES ${PATH}/${item}")
      source_group("${_basepath}" FILES "${PATH}/${item}")
    endforeach()
  endfunction()
  # Map this library's headers, sources and tests into the root
  preserve_structure(0 ${${PROJECT_NAME}_HEADERS})
  preserve_structure(0 ${${PROJECT_NAME}_SOURCES})
  preserve_structure(0 ${${PROJECT_NAME}_TESTS})
  # Map our dependencies into dependency/lib
  foreach(dependency ${${PROJECT_NAME}_DEPENDENCIES})
    preserve_structure(${dependency} ${${dependency}_HEADERS})
    preserve_structure(${dependency} ${${dependency}_SOURCES})
    preserve_structure(${dependency} ${${dependency}_TESTS})
  endforeach()
endif()

all_target_output_properties(PROPERTIES
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

option(ENABLE_VALGRIND "Compiles in valgrind instrumentation such that locks are checked for correctness etc in optimised builds. Always on in Debug builds, defaults to off in optimised builds" OFF)
if(ENABLE_VALGRIND)
  indented_message(STATUS "ENABLE_VALGRIND=ON: Compiling in valgrind instrumentation so correctness can be checked")
  all_compile_definitions(PUBLIC QUICKCPPLIB_ENABLE_VALGRIND=1)
else()
  all_compile_definitions(PUBLIC $<$<CONFIG:Debug>:QUICKCPPLIB_ENABLE_VALGRIND=1>)
endif()

all_target_output_properties(PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

if(WIN32)
  all_compile_definitions(PUBLIC _UNICODE UNICODE)                          # Unicode support
endif()

if(MSVC)
  all_compile_options(PRIVATE /W4)                                          # Stronger warnings
else()
  all_compile_options(PRIVATE -Wall -Wextra)                                # Stronger warnings
  if(CLANG)
    # Make use of clang's ability to warn on bad doxygen markup
    all_compile_options(PRIVATE
      -Wdocumentation
      -fcomment-block-commands=raceguarantees,complexity,exceptionmodel
    )
  endif()
endif()
