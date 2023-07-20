# Adds an interface target representing the header-only edition of ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_hl: Header only library target
#  * ${PROJECT_NAME}_hlm: Header only C++ Module target (where supported)

if(NOT DEFINED ${PROJECT_NAME}_HEADERS)
  message(FATAL_ERROR "FATAL: QuickCppLibSetupProject has not been included yet.")
endif()

function(target_append_header_only_sources tgt)
  set(sources)
  foreach(header ${${PROJECT_NAME}_HEADERS})
    list(APPEND sources
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${header}>"
    "$<INSTALL_INTERFACE:${header}>"
    )
    #indented_message(STATUS "${header}")
  endforeach()
  target_sources(${tgt} INTERFACE ${sources})
endfunction()

function(default_header_only_interface_library)
  if(ARGN)
    indented_message(STATUS "NOTE: NOT compiling header only library for ${PROJECT_NAME} into a precompiled header due to ${ARGN}")
  endif()
  add_library(${PROJECT_NAME}_hl INTERFACE)
  target_include_directories(${PROJECT_NAME}_hl INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
#    "$<INSTALL_INTERFACE:include>"  ## unneeded, as adding sources does this for us
  )
#  if(DEFINED ${PROJECT_NAME}_INTERFACE)
#    foreach(source ${${PROJECT_NAME}_INTERFACE})
#      # Cause my master header to appear in the sources of anything consuming me
#      if(NOT EXISTS "${${PROJECT_NAME}_PATH}/${source}")
#        set(source "include/${source}")
#      endif()
#      #indented_message(STATUS "*** ${source}")
#      target_sources(${PROJECT_NAME}_hl INTERFACE
#        "$<BUILD_INTERFACE:${${PROJECT_NAME}_PATH}/${source}>"
#        "$<INSTALL_INTERFACE:${source}>"
#      )
#    endforeach()
#  else()
    # Include all my headers into the sources of anything consuming me
    target_append_header_only_sources(${PROJECT_NAME}_hl)
#  endif()
endfunction()

if(${PROJECT_NAME}_INTERFACE_DISABLED)
  default_header_only_interface_library()
elseif(NOT ${PROJECT_NAME}_IS_DEPENDENCY)
  if(COMMAND target_precompile_headers)
    default_header_only_interface_library()
    set(pch_sources ${${PROJECT_NAME}_INTERFACE})
    list_filter(pch_sources EXCLUDE REGEX "\\.natvis$")
    target_precompile_headers(${PROJECT_NAME}_hl INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${pch_sources}>")
  else()
    default_header_only_interface_library("this cmake does not support target_precompile_headers()")
  endif()
else()
  default_header_only_interface_library("this project being a dependency of a higher level project")
endif()
list(APPEND ${PROJECT_NAME}_TARGETS ${PROJECT_NAME}_hl)
add_dependencies(_hl ${PROJECT_NAME}_hl)
# Set up the "nicer" target aliases
add_library(${PROJECT_NAMESPACE}${PROJECT_NAME}::hl ALIAS ${PROJECT_NAME}_hl)
