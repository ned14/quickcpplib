# Adds an interface target representing the header-only edition of ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_hl: Header only library target
#  * ${PROJECT_NAME}_hlm: Header only C++ Module target (where supported)

if(NOT DEFINED ${PROJECT_NAME}_HEADERS)
  message(FATAL_ERROR "FATAL: BoostLiteSetupProject has not been included yet.")
endif()
include(BoostLitePrecompiledHeader)

function(default_header_only_interface_library reason)
  indented_message(STATUS "NOTE: NOT compiling header only library for ${PROJECT_NAME} into a C++ Module nor a precompiled header due to ${reason}")
  add_library(${PROJECT_NAME}_hl INTERFACE)
  if(DEFINED ${PROJECT_NAME}_INTERFACE)
    # Cause my master header to appear in the sources of anything consuming me
    target_sources(${PROJECT_NAME}_hl INTERFACE
      "$<BUILD_INTERFACE:${${PROJECT_NAME}_PATH}/include/${${PROJECT_NAME}_INTERFACE}>"
      "$<INSTALL_INTERFACE:include/${${PROJECT_NAME}_INTERFACE}>"
    )
  else()
    # Include all my headers into the sources of anything consuming me
    set(sources)
    foreach(header ${${PROJECT_NAME}_HEADERS})
      list(APPEND sources
      "$<BUILD_INTERFACE:${${PROJECT_NAME}_PATH}/${header}>"
      "$<INSTALL_INTERFACE:${header}>"
    )
    endforeach()
    target_sources(${PROJECT_NAME}_hl INTERFACE ${sources})
  endif()
endfunction()

if(${PROJECT_NAME}_INTERFACE_DISABLED)
  default_header_only_interface_library("this project not providing a master interface header file")
elseif(CMAKE_VERSION VERSION_LESS 3.3)
  default_header_only_interface_library("using a cmake before v3.3")
elseif(MSVC AND NOT CLANG)
  if(MSVC_VERSION VERSION_GREATER 1999) # VS2017
    # Add a C++ Module for the PCH header file
    add_cxx_module(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
  else()
    # MSVC can't share precompiled headers between targets
    # so fall back onto an interface library
    default_header_only_interface_library("this MSVC does not sufficiently support C++ Modules, and MSVC cannot share precompiled headers between targets")
  endif()
elseif(NOT PROJECT_IS_DEPENDENCY)
  # Add a precompiled header for the PCH header file
  add_precompiled_header(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
else()
  default_header_only_interface_library("this project being a dependency of a higher level project")
endif()
list(APPEND ${PROJECT_NAME}_TARGETS ${PROJECT_NAME}_hl)
