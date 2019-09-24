install(EXPORT ${PROJECT_NAME}Exports
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/FixupInstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}FixupInstall.cmake"
  @ONLY
)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}FixupInstall.cmake")
