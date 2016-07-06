# Adds a custom command which generates a precompiled header
function(target_precompiled_header target headerpath)
  if(MSVC)
    add_custom_command(TARGET ${target} PRE_LINK
      COMMAND <CMAKE_CXX_COMPILER> <FLAGS> /Fp"${CMAKE_CURRENT_BINARY_DIR}/${target}.pch" /Yc"${headerpath}"
      COMMENT "Precompiling header ${headerpath} ..."
    )
  else()
  endif()
endfunction()

# Adds a custom target which generates a precompiled header
function(add_precompiled_header outvar headerpath)
  if(MSVC)
    add_custom_target(${outvar}
      ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}> $<$<CONFIG:RelWithDebInfo>:${CMAKE_CXX_FLAGS_RELWITHDEBINFO}> $<$<CONFIG:MinSizeRel>:${CMAKE_CXX_FLAGS_MINSIZEREL}> /Fp"${CMAKE_CURRENT_BINARY_DIR}/${target}.pch" /Yc"${headerpath}"
      COMMENT "Precompiling header ${headerpath} ..."
      SOURCES "${headerpath}"
    )
  else()
  endif()
#  set(${outvar} ${pch} PARENT_SCOPE)
endfunction()
