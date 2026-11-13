include(FindDoxygen)
include(FindGit)
if(NOT DOXYGEN_FOUND)
  indented_message(WARNING "WARNING: Doxygen not found, so disabling ${PROJECT_NAME}_docs target")
else()
  function(BoostLiteMakeDoxygen tgt)
    # Is there an examples directory? If so, compile every example in there
    set(example_bins)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/example")
      file(GLOB_RECURSE example_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
           "${CMAKE_CURRENT_SOURCE_DIR}/example/*.cpp"
           )
      # Assume the first target is enough to link the example
      list(GET ${PROJECT_NAME}_TARGETS 0 lib)
      foreach(example_src ${example_srcs})
        if(example_src MATCHES ".+/(.+)[.](c|cpp|cxx)$")
          set(example_bin "${PROJECT_NAME}-example_${CMAKE_MATCH_1}")
          add_executable(${example_bin} EXCLUDE_FROM_ALL "${example_src}")
          target_link_libraries(${example_bin} ${lib})
          set_target_properties(${example_bin} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            POSITION_INDEPENDENT_CODE ON
          )
          list(APPEND example_bins ${example_bin})
        endif()
      endforeach()
    endif()
    # We don't have wildcard file copy in cmake < 3.5, so choose a shell copy command
    if(WIN32)
      set(shell_copy_command (robocopy "doc\\html.old" "doc\\html" ".git*" /np /nfl /ndl /njh /njs) || set ERRORLEVEL=0)
    else()
      set(shell_copy_command cp "doc/html.old/.git*" "doc/html/")
    endif()
    # The doxygen warning gate is a cmake -P script rather than a shell snippet,
    # because a snippet held in a CMake list is split into one element per word
    # and COMMAND would then exec the program `if`; the $<SEMICOLON> spelling
    # that keeps it in one element loses its quoting when Ninja writes the
    # command line, so the `;` breaks the outer shell instead. A script has
    # neither problem and needs no per-platform branch: see
    # CheckDoxygenWarnings.cmake for the whole story.
    set(doxygen_warnings_script "${CMAKE_CURRENT_LIST_DIR}/CheckDoxygenWarnings.cmake")
    add_custom_target(${tgt}
      COMMAND ${CMAKE_COMMAND} -E rename "doc/html" "doc/html.old"
      COMMAND ${CMAKE_COMMAND} -E make_directory "doc/html"
      COMMAND ${shell_copy_command}
      COMMAND ${CMAKE_COMMAND} -E remove_directory "doc/html.old"
      COMMAND ${DOXYGEN_EXECUTABLE}
      COMMAND ${CMAKE_COMMAND} "-DWARNINGS_FILE=${CMAKE_CURRENT_SOURCE_DIR}/doxygen_warnings.log" -P "${doxygen_warnings_script}"
      COMMAND ${CMAKE_COMMAND} -E chdir "doc" "${GIT_EXECUTABLE}" add -A .
      COMMAND ${CMAKE_COMMAND} -E chdir "doc/html" "${GIT_EXECUTABLE}" add -A .
      DEPENDS ${example_bins} ${${PROJECT_NAME}_DOXYGEN_DEPENDS}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Regenerating documentation ..."
      SOURCES "Doxyfile" ${${PROJECT_NAME}_DOXYGEN_SOURCES}
    )
    add_dependencies(_docs ${tgt})
    set(${PROJECT_NAME}_EXAMPLE_TARGETS ${example_bins} PARENT_SCOPE)
  endfunction()
  BoostLiteMakeDoxygen(${PROJECT_NAME}_docs)
endif()
