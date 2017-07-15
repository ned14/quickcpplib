include(QuickCppLibUtils)

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
    indented_message(FATAL_ERROR "cmake v3.3 is required to use the add_precompiled_header() function")
  endif()
  get_filename_component(header "${headerpath}" NAME)
  get_filename_component(header_noext "${headerpath}" NAME_WE)
  get_filename_component(headerdir "${headerpath}" DIRECTORY)
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
    # Can't propagate $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_OPTIONS> else the include of
    # myself into dependencies gets propagated too :(
    #COMPILE_OPTIONS ${INTERFACE_COMPILE_OPTIONS}
    #INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${outvar},INTERFACE_INCLUDE_DIRECTORIES>
    #SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${outvar},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
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
    indented_message(STATUS "MSVC type precompiled header target ${headerpath} => ${pchpath}")
  else()
    set_source_files_properties(${sources} PROPERTIES
      LANGUAGE CXX
    )
    set_target_properties(${outvar}_pch PROPERTIES
      POSITION_INDEPENDENT_CODE ON
    )
    if(CLANG)
      if(MSVC)
        target_compile_options(${outvar} INTERFACE -Winvalid-pch -include-pch ${CMAKE_CURRENT_BINARY_DIR}/${outvar}_pch.dir/$<CONFIG>/${header_noext}.obj)
        indented_message(STATUS "Winclang type precompiled header target ${headerpath} => ${CMAKE_CURRENT_BINARY_DIR}/${outvar}_pch.dir/$<CONFIG>/${header_noext}.obj")
      else()
        target_compile_options(${outvar} INTERFACE -Winvalid-pch -include-pch ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}_pch.dir/include/${headerpath}.o)
        indented_message(STATUS "GNU clang type precompiled header target ${headerpath} => ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}_pch.dir/include/${headerpath}.o")
      endif()
    else()
      # GCC really needs the precompiled output to have a .gch extension and
      # to live in the same directory as its .hpp file. So copy over the .hpp
      # file into the output next to the gch file and add an include search
      # path so it picks up its siblings
      add_custom_target(${outvar}_gch
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${headerpath}.o ${headerpath}.gch
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${sources} ${headerpath}
        DEPENDS ${outvar}_pch
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}_pch.dir/include
        COMMENT "Setting up precompiled header ${headerpath}.gch for forced inclusion ..."
        VERBATIM
      )
      add_dependencies(${outvar} ${outvar}_gch)
      target_compile_options(${outvar} INTERFACE -Winvalid-pch -I${CMAKE_CURRENT_SOURCE_DIR}/include/${headerdir} -include ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}_pch.dir/include/${headerpath})
      indented_message(STATUS "GCC type precompiled header target ${headerpath} => ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${outvar}_pch.dir/include/${headerpath}.o")
    endif()
  endif()
endfunction()

# Adds an object library target which generates a C++ Module,
# storing into outvar an interface library with the appropriate markup
# to use the C++ Module
function(add_cxx_module outvar ixxpath)
  # cmake 3.3 is needed for this function to work (adding dependencies to an INTERFACE target)
  if(CMAKE_VERSION VERSION_LESS 3.3)
    indented_message(FATAL_ERROR "cmake v3.3 is required to use the add_cxx_module() function")
  endif()
  add_library(${outvar}_ixx OBJECT "${ixxpath}")
  set_source_files_properties("${ixxpath}" PROPERTIES
      LANGUAGE CXX
  )
  add_library(${outvar} INTERFACE)
  # This bit needs >= cmake 3.3
  add_dependencies(${outvar} ${outvar}_ixx)
  # Whatever interface properties are set onto ${outvar} must also be set onto the C++ Module
  set_target_properties(${outvar}_ixx PROPERTIES
    COMPILE_DEFINITIONS $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_DEFINITIONS>
    COMPILE_FEATURES $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_FEATURES>
    # Can't propagate options else the include of myself into dependencies gets propagated too :(
    #COMPILE_OPTIONS $<TARGET_PROPERTY:${outvar},INTERFACE_COMPILE_OPTIONS>
    #INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${outvar},INTERFACE_INCLUDE_DIRECTORIES>
  )
  if(MSVC)
    target_compile_options(${outvar}_ixx PRIVATE /experimental:module /D__cpp_modules=1)
    target_compile_options(${outvar} INTERFACE /experimental:module /D__cpp_modules=1 /module:search ${CMAKE_CURRENT_BINARY_DIR}/${outvar}_ixx.dir/$<CONFIG>)
  else()
    indented_message(FATAL_ERROR "FATAL: C++ Modules not implemented for compilers other than MSVC yet")
  endif()
endfunction()
