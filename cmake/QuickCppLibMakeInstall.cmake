# Firstly, to install the library into /usr/local or whatever
# Note that this only installs this library, not any boost-lite imported
# dependencies, and so is actually quite useless.
#
# The showstopper question, which we also need to answer for cmake package
# support, is how best to implement cmake package dependencies.
foreach(header ${${PROJECT_NAME}_HEADERS})
  get_filename_component(dir ${header} DIRECTORY)
  # If not a boost library, install into a library named after the library
  if(NOT dir MATCHES "include/boost/")
    string(REGEX REPLACE "include/?(.*)" "include/${PROJECT_NAME}/\\1" dir "${dir}")
  endif()
  #indented_message(STATUS "*** Would install ${header} => ${dir}")
  install(FILES ${header}
    DESTINATION "${dir}"
  )
endforeach()
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
