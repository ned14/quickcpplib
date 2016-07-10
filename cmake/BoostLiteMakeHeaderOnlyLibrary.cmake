# Adds an interface target representing the header-only edition of ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_hl: Header only library target
#  * ${PROJECT_NAME}_hlm: Header only C++ Module target (where supported)

include(BoostLiteDeduceLibrarySources)
include(BoostLitePrecompiledHeader)

if(CMAKE_VERSION VERSION_LESS 3.3)
  message(WARNING "WARNING: Disabling precompilation of header-only library as using a cmake before v3.3")
  # Add an interface library representing the PCH header file
  add_library(${PROJECT_NAME}_hl INTERFACE)
elseif(MSVC)
  if(MSVC_VERSION VERSION_GREATER 1999) # VS2017
    # Add a C++ Module for the PCH header file
    add_cxx_module(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
  else()
    # MSVC can't share precompiled headers between targets
    # so fall back onto an interface library
    add_library(${PROJECT_NAME}_hl INTERFACE)
  endif()
else()
  # Add a precompiled header for the PCH header file
  add_precompiled_header(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
endif()
list(APPEND ${PROJECT_NAME}_targets ${PROJECT_NAME}_hl)
