# Adds an interface target representing the header-only edition of ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_hl: Header only library target
#  * ${PROJECT_NAME}_hlm: Header only C++ Module target (where supported)

if(NOT DEFINED ${PROJECT_NAME}_HEADERS)
  message(FATAL_ERROR "FATAL: BoostLiteSetupProject has not been included yet.")
endif()
include(QuickCppLibPrecompiledHeader)

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

function(default_header_only_interface_library reason)
  indented_message(STATUS "NOTE: NOT compiling header only library for ${PROJECT_NAME} into a C++ Module nor a precompiled header due to ${reason}")
  add_library(${PROJECT_NAME}_hl INTERFACE)
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

# Do we have C++ Modules support on this compiler?
if(NOT DEFINED ENABLE_CXX_MODULES)
  if(MSVC_VERSION VERSION_GREATER 1999)  # VS2017
    option(ENABLE_CXX_MODULES "Enable C++ Modules support in this build (defaults to ON on a sufficiently recent MSVC)" ON)
  else()
    option(ENABLE_CXX_MODULES "Enable C++ Modules support in this build (defaults to ON on a sufficiently recent MSVC)" OFF)
  endif()
endif()
if(${PROJECT_NAME}_INTERFACE_DISABLED)
  default_header_only_interface_library("this project not providing a master interface header file")
elseif(CMAKE_VERSION VERSION_LESS 3.3)
  default_header_only_interface_library("using a cmake before v3.3")
elseif(ENABLE_CXX_MODULES AND DEFINED ${PROJECT_NAME}_INTERFACE_SOURCE)  # This library provides a C++ Module source file
  # Add a C++ Module for the PCH header file
  indented_message(STATUS "Compiling ${${PROJECT_NAME}_INTERFACE_SOURCE} into a C++ Module for the ${PROJECT_NAME}_hl target")
  add_cxx_module(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE_SOURCE})
elseif(MSVC)
  # MSVC can't share precompiled headers between targets so fall back onto an interface library
  if(DEFINED ${PROJECT_NAME}_INTERFACE_SOURCE)
    default_header_only_interface_library("this MSVC does not sufficiently support C++ Modules, and MSVC cannot share precompiled headers between targets")
  else()
    default_header_only_interface_library("this project not providing a C++ Module source file, and MSVC cannot share precompiled headers between targets")
  endif()
elseif(NOT PROJECT_IS_DEPENDENCY)
## Works on anything not Bash for Windows, but that's mostly what I'm testing with
##  # Add a precompiled header for the PCH header file
##  add_precompiled_header(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
##  # Include all my headers into the sources of anything consuming me
##  target_append_header_only_sources(${PROJECT_NAME}_hl)
  default_header_only_interface_library("Niall temporarily disabling precompiled headers support pending diagnosis")
else()
  default_header_only_interface_library("this project being a dependency of a higher level project")
endif()
list(APPEND ${PROJECT_NAME}_TARGETS ${PROJECT_NAME}_hl)
add_dependencies(_hl ${PROJECT_NAME}_hl)
# Set up the "nicer" target aliases
add_library(${PROJECT_NAMESPACE}${PROJECT_NAME}::hl ALIAS ${PROJECT_NAME}_hl)
