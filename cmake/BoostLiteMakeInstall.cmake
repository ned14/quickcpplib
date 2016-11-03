# Firstly, to install the library into /usr/local or whatever
foreach(header ${${PROJECT_NAME}_HEADERS})
  get_filename_component(dir ${header} DIRECTORY)
  install(FILES ${header}
    DESTINATION "${dir}"
  )
endforeach()
# TODO FIXME: Need to install all headers from all dependencies too
if(TARGET ${PROJECT_NAME}_sl)
  install(FILES ${${PROJECT_NAME}_sl}
    DESTINATION "lib"
  )
endif()
if(TARGET ${PROJECT_NAME}_dl)
  install(FILES ${${PROJECT_NAME}_dl}
    DESTINATION "lib"
  )
endif()


# All interface headers in all targets are public headers
#foreach(target ${${PROJECT_NAME}_TARGETS})
#  set_target_properties(${target} PROPERTIES PUBLIC_HEADER "${${PROJECT_NAME}_HEADERS}")
#endforeach()
#install(TARGETS ${${PROJECT_NAME}_TARGETS} #EXPORT ${PROJECT_NAMESPACE}${PROJECT_NAME}
#  RUNTIME DESTINATION "bin"
#  ARCHIVE DESTINATION "lib"
#  LIBRARY DESTINATION "lib"
#  INCLUDES DESTINATION "include/${PROJECT_DIR}"
#)
#install(EXPORT ${PROJECT_NAMESPACE}${PROJECT_NAME}
#  DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#  CONFIGURATIONS Release
#  NAMESPACE ${PROJECT_NAMESPACE}
#)