# Adds an interface target representing the header-only edition of ${PROJECT_NAME}
# 
# Outputs:
#  *  ${PROJECT_NAME}_hl: Header only library target
#  * ${PROJECT_NAME}_hlm: Header only C++ Module target (where supported)

include(BoostLiteDeduceLibrarySources)
include(BoostLitePrecompiledHeader)
# Generate a precompiled header only library target
add_precompiled_header(${PROJECT_NAME}_hl ${${PROJECT_NAME}_INTERFACE})
list(APPEND ${PROJECT_NAME}_targets ${PROJECT_NAME}_hl)
