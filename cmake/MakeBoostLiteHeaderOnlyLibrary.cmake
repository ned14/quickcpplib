# Adds an interface target representing the header-only edition of ${PROJECT_NAME}

include(DeduceBoostLiteLibrarySources)
add_library(${PROJECT_NAME}_hl INTERFACE)
target_include_directories(${PROJECT_NAME}_hl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
