# Note that this only installs this library, not any quickcpplib imported
# dependencies. It is on the installation consumer to also install the
# dependencies separately.
include(GNUInstallDirs)

add_custom_target(install.core
                  COMMAND
                      "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=core
                      -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)

foreach(header ${${PROJECT_NAME}_HEADERS})
  get_filename_component(dir ${header} DIRECTORY)
  install(FILES "${header}"
    COMPONENT core
    DESTINATION "${dir}"
  )
endforeach()
if(TARGET ${PROJECT_NAME}_hl)
    install(TARGETS ${PROJECT_NAME}_hl
            COMPONENT core
            EXPORT ${PROJECT_NAME}Exports
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_hl PROPERTIES EXPORT_NAME hl)
  add_dependencies(install.core ${PROJECT_NAME}_hl)
endif()
if(TARGET ${PROJECT_NAME}_sl)
    install(TARGETS ${PROJECT_NAME}_sl
            COMPONENT sl
            EXPORT ${PROJECT_NAME}SlExports
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_sl PROPERTIES EXPORT_NAME sl)
  add_custom_target(install.sl
                    DEPENDS install.core ${PROJECT_NAME}_sl
                    COMMAND
                        "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=sl
                        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  )
endif()
if(TARGET ${PROJECT_NAME}_dl)
    install(TARGETS ${PROJECT_NAME}_dl
            EXPORT ${PROJECT_NAME}DlExports
            COMPONENT dl
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  set_target_properties(${PROJECT_NAME}_dl PROPERTIES EXPORT_NAME dl)
  add_custom_target(install.dl
                    DEPENDS install.core ${PROJECT_NAME}_dl
                    COMMAND
                        "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=dl
                        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  )
endif()

# Create and install a find package file
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/ProjectConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  @ONLY
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  COMPONENT core
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)
