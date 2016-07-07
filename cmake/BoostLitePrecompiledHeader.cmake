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
  get_filename_component(header "${headerpath}" NAME)
  set(pchpath ${CMAKE_CURRENT_BINARY_DIR}/${header}.dir/${CMAKE_CFG_INTDIR}/${header}.pch)
  set(flags ${CMAKE_CXX_FLAGS}

  )
  separate_arguments(flags)
  # FIXME: These generators work, but quote the expansion thus breaking the custom target :(
  set(flags ${flags}
#    $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>
    $<$<CONFIG:Debug>:/A/B/C>
    $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>
    $<$<CONFIG:RelWithDebInfo>:${CMAKE_CXX_FLAGS_RELWITHDEBINFO}>
    $<$<CONFIG:MinSizeRel>:${CMAKE_CXX_FLAGS_MINSIZEREL}>
  )
  MESSAGE(STATUS "${flags}")
  if(MSVC)
    add_custom_target(${outvar}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${header}.dir/${CMAKE_CFG_INTDIR}"
      COMMAND ${CMAKE_CXX_COMPILER} /c ${flags} /Fp"${pchpath}" /Yc"${header}" /Tp"${CMAKE_CURRENT_SOURCE_DIR}/${headerpath}"
      COMMENT "Precompiling header ${headerpath} ..."
      SOURCES "${headerpath}"
    )
  else()
  endif()
#  set(${outvar} ${pch} PARENT_SCOPE)
endfunction()
