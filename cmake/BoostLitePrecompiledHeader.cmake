include(BoostLiteUtils)

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

# Adds an object library target which generates a precompiled header,
# storing into outvar an interface library with the appropriate markup
# to use the precompiled header
function(add_precompiled_header outvar headerpath)
  # cmake 3.3 is needed for this function to work (adding dependencies to an INTERFACE target)
  if(CMAKE_VERSION VERSION_LESS 3.3)
    message(FATAL_ERROR "cmake v3.3 is required to use the add_precompiled_header() function")
  endif()
  get_filename_component(header "${headerpath}" NAME)
  set(sources "include/${headerpath}")
  if(MSVC)
    # MSVC PCH generation requires a source file to include the header
    # so we'll need to generate one
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${header}_pch_gen.cpp"
      "#include \"${headerpath}\"\n")
    list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${header}_pch_gen.cpp")
  endif()
  add_library(${outvar}_pch OBJECT ${sources})
  add_library(${outvar} INTERFACE)
  # This bit needs >= cmake 3.3
  add_dependencies(${outvar} ${outvar}_pch)
  # Whatever interface properties are set onto ${outvar} must also be set onto the PCH generation
  set_target_properties(${outvar}_pch PROPERTIES
    COMPILE_DEFINITIONS $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_DEFINITIONS>
    COMPILE_FEATURES $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_FEATURES>
    # Can't propagate options else the include of myself into dependencies gets propagated too :(
    #COMPILE_OPTIONS $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_OPTIONS>
    INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${outvar},INTERFACE_INCLUDE_DIRECTORIES>
  )
  if(MSVC)
    # Visual Studio generator outputs PCH to /Fp"afio_hl_pch.dir\Debug\afio_hl.pch"
    #
    # cmake converts /Fp<something> into the native .vcxproj stanza <PrecompiledHeaderOutputFile>
    # which causes Visual Studio to delete the PCH before every build. Yay.
    #
    # We therefore put the interface options for including a precompiled header into a response
    # file to work around Visual Studio's unhelpful behaviour
    target_compile_options(${outvar}_pch PRIVATE /W4 /Yc${headerpath})
    foreach(config Debug Release RelWithDebInfo MinSizeRel)
      NativisePath(pchpath ${outvar}_pch.dir/${config}/${outvar}_pch.pch)
      file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${outvar}_pch.dir/${config}/${outvar}_pch.response" "/Yu${headerpath} /FI${headerpath} /Fp${pchpath}")
    endforeach()
    target_compile_options(${outvar} INTERFACE @${outvar}_pch.dir/$<CONFIG>/${outvar}_pch.response)
  else()
    set_source_files_properties(${headerpath} PROPERTIES
      LANGUAGE CXX
    )
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set_source_files_properties(${headerpath} PROPERTIES
        COMPILE_FLAGS "-emit-pch"
      )
      target_compile_options(${outvar} INTERFACE -Winvalid-pch -include-pch ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}.dir/${headerpath}.o)
      message(STATUS "Precompiled header target ${headerpath} => ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}.dir/${headerpath}.o")
    else()
      target_compile_options(${outvar} INTERFACE -Winvalid-pch -include ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}.dir/${headerpath}.o)
      message(STATUS "Precompiled header target ${headerpath} => ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}.dir/${headerpath}.o")
    endif()
  endif()
endfunction()
