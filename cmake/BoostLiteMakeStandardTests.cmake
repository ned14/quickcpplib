# Adds standard ctest tests for each *.c;*.cpp;*.cxx in
# ${PROJECT_NAME}_TESTS for all ${PROJECT_NAME}_targets
#
# Sets ${PROJECT_NAME}_TEST_TARGETS to the test targets generated

if(DEFINED ${PROJECT_NAME}_TESTS AND NOT PROJECT_IS_DEPENDENCY)
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
          set(target_name "${target}-${testname}")
          add_executable(${target_name} ${thistestsources})
          list(APPEND testtargets "${target_name}")
          target_link_libraries(${target_name} ${target})
          set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
          )
          # FIXME: Cannot assume tests are using CATCH like this
          add_test(NAME ${target_name}
            COMMAND $<TARGET_FILE:${target_name}> --reporter junit --out $<TARGET_FILE:${target_name}>.junit.xml
          )
          if(MSVC)
            target_compile_options(${target_name} PRIVATE /W4)                                          # Stronger warnings
          else()
            target_compile_options(${target_name} PRIVATE -Wall -Wextra)                                # Stronger warnings
          endif()
        endforeach()
      endif()
    endforeach()
    set(${PROJECT_NAME}_TEST_TARGETS ${testtargets} PARENT_SCOPE)
  endfunction()
  generate_tests()
endif()
