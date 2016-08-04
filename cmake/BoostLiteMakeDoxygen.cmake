include(FindDoxygen)
include(FindGit)
if(NOT DOXYGEN_FOUND)
  indented_message(WARNING "WARNING: Doxygen not found, so disabling ${PROJECT_NAME}_docs target")
else()
  # We don't have wildcard file copy in cmake < 3.5, so choose a shell copy command
  if(WIN32)
    set(shell_copy_command (robocopy "doc\\html.old" "doc\\html" ".git*" /np /nfl /ndl /njh /njs) || set ERRORLEVEL=0)
  else()
    set(shell_copy_command cp "doc/html.old/.git*" "doc/html/")
  endif()
  add_custom_target(${PROJECT_NAME}_docs
    COMMAND ${CMAKE_COMMAND} -E rename "doc/html" "doc/html.old"
    COMMAND ${CMAKE_COMMAND} -E make_directory "doc/html"
    COMMAND ${shell_copy_command}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "doc/html.old"
    COMMAND ${DOXYGEN_EXECUTABLE}
    COMMAND ${CMAKE_COMMAND} -E chdir "doc" "${GIT_EXECUTABLE}" add .
    COMMAND ${CMAKE_COMMAND} -E chdir "doc/html" "${GIT_EXECUTABLE}" add .
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Regenerating documentation ..."
    VERBATIM
    SOURCES "Doxyfile"
  )
endif()
