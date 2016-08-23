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
      set(testtargets)
      foreach(testsource ${${PROJECT_NAME}_TESTS})
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
          foreach(target ${${PROJECT_NAME}_TARGETS})
            foreach(special ${SPECIAL_BUILDS} "")
              if(special STREQUAL "")
                set(target_name "${target}--${testname}")
              else()
                set(target_name "${target}-${special}-${testname}")
                if(NOT target MATCHES "_hl$")
                  set(target "${target}-${special}")
                endif()
              endif()
              add_executable(${target_name} ${thistestsources})
              list(APPEND testtargets "${target_name}")
              target_link_libraries(${target_name} ${target})
              set_target_properties(${target_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
              )
              if(special STREQUAL "")
                # FIXME: Cannot assume tests are using CATCH like this
                add_test(NAME ${target_name} CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel
                  COMMAND $<TARGET_FILE:${target_name}> --reporter junit --out $<TARGET_FILE:${target_name}>.junit.xml
                )
              else()
                add_test(NAME ${target_name} COMMAND $<TARGET_FILE:${target_name}> CONFIGURATIONS ${special})
                set_target_properties(${target_name} PROPERTIES
                  EXCLUDE_FROM_ALL ON
                  LINK_FLAGS ${${special}_COMPILE_FLAGS}
                )
                target_compile_options(${target_name} PRIVATE ${${special}_COMPILE_FLAGS})
                list(APPEND ${PROJECT_NAME}_${special}_TARGETS ${target_name})
              endif()
              if(MSVC AND NOT CLANG)
                target_compile_options(${target_name} PRIVATE /W4)                                          # Stronger warnings
              else()
                target_compile_options(${target_name} PRIVATE -Wall -Wextra)                                # Stronger warnings
              endif()
            endforeach()
          endforeach()
        endif()
      endforeach()
      set(${PROJECT_NAME}_TEST_TARGETS ${testtargets} PARENT_SCOPE)
      foreach(special ${SPECIAL_BUILDS})
        set(${PROJECT_NAME}_${special}_TARGETS ${${PROJECT_NAME}_${special}_TARGETS} PARENT_SCOPE)
      endforeach()
    endfunction()
    generate_tests()
  endif()

  # For all special builds, create custom "all" target for each of those so one can build "everything with asan" etc
  foreach(special ${SPECIAL_BUILDS})
    indented_message(STATUS "Creating non-default all target for special build ${PROJECT_NAME}-${special}")
    add_custom_target(${PROJECT_NAME}-${special} DEPENDS ${${PROJECT_NAME}_${special}_TARGETS})
  endforeach()
endif()
