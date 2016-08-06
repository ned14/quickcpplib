include(FindDoxygen)
include(FindGit)
if(NOT DOXYGEN_FOUND)
  indented_message(WARNING "WARNING: Doxygen not found, so disabling ${PROJECT_NAME}_docs target")
else()
  function(BoostLiteMakeDoxygen tgt)
    # We don't have wildcard file copy in cmake < 3.5, so choose a shell copy command
    if(WIN32)
      set(shell_copy_command (robocopy "doc\\html.old" "doc\\html" ".git*" /np /nfl /ndl /njh /njs) || set ERRORLEVEL=0)
      set(shell_check_warnings_file_size for %%R in (doxygen_warnings.log) do if not %%~zR==0 (type doxygen_warnings.log & echo. & echo FATAL ERROR: Failing due to doxygen warnings & exit /b 1))
    else()
      set(shell_copy_command cp "doc/html.old/.git*" "doc/html/")
      set(shell_check_warnings_file_size if test -s doxygen_warnings.log $<SEMICOLON> then cat doxygen_warnings.log $<SEMICOLON> echo $<SEMICOLON> echo FATAL ERROR: Failing due to doxygen warnings $<SEMICOLON> echo $<SEMICOLON> exit 1 $<SEMICOLON> fi)
    endif()
    add_custom_target(${tgt}
      COMMAND ${CMAKE_COMMAND} -E rename "doc/html" "doc/html.old"
      COMMAND ${CMAKE_COMMAND} -E make_directory "doc/html"
      COMMAND ${shell_copy_command}
      COMMAND ${CMAKE_COMMAND} -E remove_directory "doc/html.old"
      COMMAND ${DOXYGEN_EXECUTABLE}
      COMMAND ${shell_check_warnings_file_size}
      COMMAND ${CMAKE_COMMAND} -E chdir "doc" "${GIT_EXECUTABLE}" add .
      COMMAND ${CMAKE_COMMAND} -E chdir "doc/html" "${GIT_EXECUTABLE}" add .
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Regenerating documentation ..."
      SOURCES "Doxyfile"
    )
  endfunction()
  BoostLiteMakeDoxygen(${PROJECT_NAME}_docs)
endif()
