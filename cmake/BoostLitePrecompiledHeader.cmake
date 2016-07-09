# Add generator expressions to appendvar expanding at build time any remaining parameters
# if the build configuration is config
function(expand_at_build_if_config config appendvar)
  set(ret ${${appendvar}})
  set(items ${ARGV})
  list(REMOVE_AT items 0 1)
  separate_arguments(items)
  foreach(item ${items})
    list(APPEND ret $<$<CONFIG:${config}>:${item}>)
  endforeach()
  set(${appendvar} ${ret} PARENT_SCOPE)
endfunction()

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

# Adds an object library target which generates a precompiled header
function(add_precompiled_header outvar headerpath)
  get_filename_component(header "${headerpath}" NAME)
  set(sources ${headerpath})
  if(MSVC)
    # MSVC PCH generation requires a source file to include the header
    # so we'll need to generate one
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${header}_pch_gen.cpp"
      "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${headerpath}\"\n")
    list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${header}_pch_gen.cpp")
  endif()
  add_library(${outvar} OBJECT ${sources})
  if(MSVC)
    set_target_properties(${outvar} PROPERTIES
      COMPILE_FLAGS "/Yc${CMAKE_CURRENT_SOURCE_DIR}/${headerpath}"
    )
	# Visual Studio generator outputs PCH to /Fp"afio_hl.dir\Debug\afio_hl.pch" 
  else()
    set_source_files_properties(${headerpath} PROPERTIES LANGUAGE CXX) 
  endif()
endfunction()
