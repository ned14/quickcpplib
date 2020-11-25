# Note that this only installs this library, not any quickcpplib imported
# dependencies. It is on the installation consumer to also install the
# dependencies separately.
include(GNUInstallDirs)

foreach(header ${${PROJECT_NAME}_HEADERS})
  get_filename_component(dir ${header} DIRECTORY)
  install(FILES "${header}"
    DESTINATION "${dir}"
  )
endforeach()
if(TARGET ${PROJECT_NAME}_hl)
    install(TARGETS ${PROJECT_NAME}_hl
            EXPORT ${PROJECT_NAME}Exports
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_hl PROPERTIES EXPORT_NAME hl)
endif()
if(TARGET ${PROJECT_NAME}_sl)
    install(TARGETS ${PROJECT_NAME}_sl
            EXPORT ${PROJECT_NAME}Exports
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES EXPORT_NAME sl)
endif()
if(TARGET ${PROJECT_NAME}_dl)
    install(TARGETS ${PROJECT_NAME}_dl
            EXPORT ${PROJECT_NAME}Exports
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES EXPORT_NAME dl)
endif()

# Create and install a find package file
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/ProjectConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  @ONLY
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
