if(CMAKE_VERSION VERSION_LESS 3.3)  # cmake before this doesn't support exporting header only libraries
  indented_message(STATUS "NOTE: Disabling cmake build target export due to using a cmake before v3.3")
else()
  export(
    TARGETS ${${PROJECT_NAME}_TARGETS}
    NAMESPACE ${PROJECT_NAMESPACE}
    FILE "${PROJECT_NAME}Targets.cmake"
    EXPORT_LINK_INTERFACE_LIBRARIES
  )
#install(EXPORT ${PROJECT_NAMESPACE}${PROJECT_NAME}
#  DESTINATION lib/${PROJECT_NAME}${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#  CONFIGURATIONS Release
#  NAMESPACE ${PROJECT_NAMESPACE}
#)
endif()
