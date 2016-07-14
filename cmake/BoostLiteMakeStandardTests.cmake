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
          add_executable(${testname}-${target} ${thistestsources})
          list(APPEND testtargets "${testname}-${target}")
          target_link_libraries(${testname}-${target} ${target})
          set_target_properties(${testname}-${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
          )
          add_test(NAME ${testname}-${target} COMMAND $<TARGET_FILE:${testname}-${target}>)
          if(MSVC)
            target_compile_options(${testname}-${target} PRIVATE /W4)                                          # Stronger warnings
          else()
            target_compile_options(${testname}-${target} PRIVATE -Wall -Wextra)                                # Stronger warnings
          endif()
        endforeach()
      endif()
    endforeach()
    set(${PROJECT_NAME}_TEST_TARGETS ${testtargets} PARENT_SCOPE)
  endfunction()
  generate_tests()
endif()
