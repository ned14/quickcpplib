# Adds standard ctest tests for each *.c;*.cpp;*.cxx in
# ${PROJECT_NAME}_TESTS for all ${PROJECT_NAME}_targets
#
# Sets ${PROJECT_NAME}_TEST_TARGETS to the test targets generated

if(NOT PROJECT_IS_DEPENDENCY)
  if(DEFINED ${PROJECT_NAME}_TESTS)
    enable_testing()
    function(generate_tests)
      # Firstly get all non-source files
      foreach(testsource ${${PROJECT_NAME}_TESTS})
        if(NOT testsource MATCHES ".+/(.+)[.](c|cpp|cxx)$")
          list(APPEND testnonsources ${testsource})
        endif()
      endforeach()
      function(do_add_test outtargets testsource special nogroup)
        unset(${outtargets} PARENT_SCOPE)
        set(target_names)
        if(testsource MATCHES ".+/(.+)[.](c|cpp|cxx)$")
          # We'll assume the test name is the source file name
          set(testname ${CMAKE_MATCH_1})
          # Path could however be test/tests/<name>/*.cpp
          if(testsource MATCHES ".+/tests/([^/]+)/(.+)[.](c|cpp|cxx)$")
            set(testname ${CMAKE_MATCH_1})
          endif()
          set(thistestsources ${testsource})
          foreach(testnonsource ${testnonsources})
            if(testnonsource MATCHES ${testname})
              list(APPEND thistestsources ${testnonsource})
            endif()
          endforeach()
          foreach(_target ${${PROJECT_NAME}_TARGETS})
            set(target ${_target})
            if(nogroup)
              unset(group)
            elseif(target MATCHES "_hl$")
              set(group _hl)
            elseif(target MATCHES "_sl$")
              set(group _sl)
            elseif(target MATCHES "_dl$")
              set(group _dl)
            else()
              unset(group)
            endif()
            if(special STREQUAL "")
              set(target_name "${target}--${testname}")
            else()
              set(group ${group}-${special})
              set(target_name "${target}-${special}-${testname}")
              if(NOT target MATCHES "_hl$")
                set(target "${target}-${special}")
              endif()
            endif()
            add_executable(${target_name} ${thistestsources})
            set_target_properties(${target_name} PROPERTIES
              EXCLUDE_FROM_ALL ON
            )
            if(DEFINED group)
              add_dependencies(${group} ${target_name})
            endif()
            target_link_libraries(${target_name} PRIVATE ${target})
            set_target_properties(${target_name} PROPERTIES
              RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
              POSITION_INDEPENDENT_CODE ON
            )
            if(WIN32)
              target_compile_definitions(${target_name} PRIVATE _CRT_NONSTDC_NO_WARNINGS)                 # Shut up about using POSIX functions
            endif()
            if(MSVC)
              target_compile_options(${target_name} PRIVATE /W4)                                          # Stronger warnings
            else()
              target_compile_options(${target_name} PRIVATE -Wall -Wextra)                                # Stronger warnings
            endif()
          list(APPEND target_names "${target_name}")
          endforeach()
          if(target_names)
            set(${outtargets} "${target_names}" PARENT_SCOPE)
          endif()
        endif()
      endfunction()
      
      # Deal with normal tests + special builds of them first
      set(testtargets)
      foreach(testsource ${${PROJECT_NAME}_TESTS})
        foreach(special ${SPECIAL_BUILDS} "")
          do_add_test(target_names "${testsource}" "${special}" OFF)
          foreach(target_name ${target_names})
            # This is a normal test target run for success
            list(APPEND testtargets "${target_name}")
            if(special STREQUAL "")
              # Output test detail to JUnit XML
              add_test(NAME ${target_name} CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel
                COMMAND $<TARGET_FILE:${target_name}> --reporter junit --out $<TARGET_FILE:${target_name}>.junit.xml
              )
            else()
              add_test(NAME ${target_name} COMMAND $<TARGET_FILE:${target_name}> CONFIGURATIONS ${special})
              if(DEFINED ${special}_LINK_FLAGS)
                _target_link_options(${target_name} ${${special}_LINK_FLAGS})
              endif()
              target_compile_options(${target_name} PRIVATE ${${special}_COMPILE_FLAGS})
              string(REGEX MATCH "_(hl|sl|dl)-" out ${target_name})
              list(APPEND ${PROJECT_NAME}_${CMAKE_MATCH_1}_${special}_TARGETS ${target_name})
            endif()
          endforeach()
        endforeach()
      endforeach()
      set(${PROJECT_NAME}_TEST_TARGETS ${testtargets} PARENT_SCOPE)
      foreach(special ${SPECIAL_BUILDS})
        if(DEFINED ${PROJECT_NAME}_hl_${special}_TARGETS)
          set(${PROJECT_NAME}_hl_${special}_TARGETS ${${PROJECT_NAME}_hl_${special}_TARGETS} PARENT_SCOPE)
        endif()
        if(DEFINED ${PROJECT_NAME}_sl_${special}_TARGETS)
          set(${PROJECT_NAME}_sl_${special}_TARGETS ${${PROJECT_NAME}_sl_${special}_TARGETS} PARENT_SCOPE)
        endif()
        if(DEFINED ${PROJECT_NAME}_dl_${special}_TARGETS)
          set(${PROJECT_NAME}_dl_${special}_TARGETS ${${PROJECT_NAME}_dl_${special}_TARGETS} PARENT_SCOPE)
        endif()
      endforeach()

      # Deal with tests which require the compilation to succeed
      foreach(testsource ${${PROJECT_NAME}_COMPILE_TESTS})
        do_add_test(target_names "${testsource}" "" OFF)
      endforeach()

      # Deal with tests which require the compilation to fail in an exact way
      foreach(testsource ${${PROJECT_NAME}_COMPILE_FAIL_TESTS})
        do_add_test(target_names "${testsource}" "" ON)
        foreach(target_name ${target_names})
          # Do not build these normally, only on request
          set_target_properties(${target_name} PROPERTIES
            EXCLUDE_FROM_ALL ON
            EXCLUDE_FROM_DEFAULT_BUILD ON
            CXX_CLANG_TIDY ""
          )
          # This test tries to build this test expecting failure
          add_test(NAME ${target_name}
            COMMAND ${CMAKE_COMMAND} --build . --target ${target_name} --config $<CONFIGURATION>
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
          )
          # Fetch the regex to detect correct failure
          file(STRINGS "${testsource}" firstline LIMIT_COUNT 2)
          list(GET firstline 1 firstline)
          set_tests_properties(${target_name} PROPERTIES PASS_REGULAR_EXPRESSION "${firstline}")
        endforeach()
      endforeach()
    endfunction()
    generate_tests()
  endif()

#  # For all special builds, create custom "all" target for each of those so one can build "everything with asan" etc
#  foreach(special ${SPECIAL_BUILDS})
#    if(DEFINED ${PROJECT_NAME}_hl_${special}_TARGETS)
#      indented_message(STATUS "Creating non-default all target for special build ${PROJECT_NAME}_hl-${special}")
#      indented_message(STATUS "*** ${${PROJECT_NAME}_hl_${special}_TARGETS}")
#      add_custom_target(${PROJECT_NAME}_hl-${special} DEPENDS ${${PROJECT_NAME}_hl_${special}_TARGETS})
#    endif()
#  endforeach()
  
  # We need to now decide on some default build target group. Prefer static libraries
  # unless this is a header only library
  if(TARGET ${PROJECT_NAME}_sl)
    set_target_properties(_sl PROPERTIES EXCLUDE_FROM_ALL OFF)
  else()
    set_target_properties(_hl PROPERTIES EXCLUDE_FROM_ALL OFF)
  endif()
endif()
