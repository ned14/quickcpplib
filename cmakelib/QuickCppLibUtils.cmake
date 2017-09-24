if(QuickCppLibUtilsIncluded)
  return()
endif()
set(QuickCppLibUtilsIncluded ON)
set(QuickCppLibCMakePath "${CMAKE_CURRENT_LIST_DIR}")
include(FindGit)
if(NOT GIT_FOUND)
  message(FATAL_ERROR "FATAL: The Boost-lite infrastructure is very tightly integrated with git"
                      " and requires it to be available")
endif()

# Returns a path with forward slashes replaced with backslashes on WIN32
function(NativisePath outvar)
  if(WIN32)
    string(REPLACE "/" "\\" new ${ARGN})
  else()
    set(new ${ARGN})
  endif()
  set(${outvar} ${new} PARENT_SCOPE)
endfunction()

# Simulate a target_link_options as cmake is missing such a thing
function(_target_link_options target_name)
  # Convert args to a string
  string(REPLACE ";" " " props "${ARGN}")
  get_target_property(oldprops ${target_name} LINK_FLAGS)
  if(oldprops MATCHES "NOTFOUND")
    set(oldprops)
  endif()
  set_target_properties(${target_name} PROPERTIES
    LINK_FLAGS "${oldprops} ${props}"
  )
endfunction()

# Add generator expressions to appendvar expanding at build time any remaining parameters
# if the <condition> is true at build time
function(expand_at_build_if condition appendvar)
  set(ret ${${appendvar}})
  set(items ${ARGN})
  separate_arguments(items)
  foreach(item ${items})
    list(APPEND ret $<${condition}:${item}>)
  endforeach()
  set(${appendvar} ${ret} PARENT_SCOPE)
endfunction()

# Emulate list(FILTER list INCLUDE|EXCLUDE REGEX regex) on cmake < 3.6
function(list_filter listname op regexqualifer regex)
  if(CMAKE_VERSION VERSION_GREATER 3.59)
    list(FILTER ${ARGV})
  else()
    set(out)
    foreach(item ${${listname}})
      string(REGEX MATCH "${regex}" match ${item})
      if("${op}" STREQUAL "INCLUDE")
        if(match)
          list(APPEND out ${item})
        endif()
      else()
        if(NOT match)
          list(APPEND out ${item})
        endif()
      endif()
    endforeach()
    set(${listname} ${out} PARENT_SCOPE)
  endif()
endfunction()

# Escape a string into a regex matching that string
function(escape_string_into_regex outvar)
  string(REGEX REPLACE "(\\^|\\$|\\.|\\[|\\]|\\*|\\+|\\?|\\(|\\)|\\\\)" "\\\\\\1" out ${ARGN})
  set(${outvar} ${out} PARENT_SCOPE)
endfunction()

# Indents a message by a global variable amount of whitespace
function(indented_message type)
  message(${type} "${MESSAGE_INDENT}" ${ARGN})
endfunction()

# Executes an external process, fatal erroring if it fails
function(checked_execute_process desc)
  execute_process(${ARGN} RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "FATAL: ${desc} failed with error '${result}'")
  endif()
endfunction()

# Determines if a git repo has changed
function(git_repo_changed dir outvar)
  execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain
    WORKING_DIRECTORY "${dir}"
    OUTPUT_VARIABLE status
    RESULT_VARIABLE result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "FATAL: git status failed with error '${result}'")
  endif()
#  message("${status}")
  if(status STREQUAL "")
    set(${outvar} FALSE PARENT_SCOPE)
  else()
    set(${outvar} TRUE PARENT_SCOPE)
  endif()
endfunction()

# Gets the committed SHA in the index for some entry
function(git_repo_get_entry_sha dir entry outvar)
  # git ls-files -s produces entries of the format:
  #   100644 e10ce7c26311e43f337b1f3929450e1804059adf 0       test/test.vcxproj
  execute_process(COMMAND "${GIT_EXECUTABLE}" ls-files -s
    WORKING_DIRECTORY "${dir}"
    OUTPUT_VARIABLE status
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "FATAL: git ls-files failed with error '${result}'")
  endif()
  escape_string_into_regex(entry "${entry}")
  if(status MATCHES "([0-9]+) ([0-9a-f]+) ([0-9]+)\t(${entry})\n")
    #set(cacheinfo "${CMAKE_MATCH_1}")
    set(sha "${CMAKE_MATCH_2}")
    #set(size "${CMAKE_MATCH_3}")
    #set(mentry "${CMAKE_MATCH_4}")
    set(${outvar} "${sha}" PARENT_SCOPE)
  else()
    unset(${outvar} PARENT_SCOPE)
  endif()
endfunction()


# Determines what git revision SHA some path is currently on
# unsetting outvar if not a git repository
function(git_revision_from_path path outsha outtimestamp)
  set(gitdir "${path}/.git")
  unset(${outsha} PARENT_SCOPE)
  unset(${outtimestamp} PARENT_SCOPE)
  if(NOT EXISTS "${gitdir}")
    return()
  endif()
  # Are you a submodule?
  if(NOT IS_DIRECTORY "${gitdir}")
    file(READ "${gitdir}" pathtogitdir)
    # This will have the form:
    # gitdir: ../../../../.git/modules/include/boost/afio/boost-lite
    # gitdir: /home/paul/tmp/cget/cget/build/tmp-48d80d9e2c734b86800806772ac60260/boost.outcome/include/boost/outcome/boost-lite/.git
    string(SUBSTRING "${pathtogitdir}" 8 -1 pathtogitdir)
    string(STRIP "${pathtogitdir}" pathtogitdir)
    if("${pathtogitdir}" MATCHES "\.\./")
      set(gitdir "${path}/${pathtogitdir}")
    else()
      set(gitdir "${pathtogitdir}")
    endif()
  endif()
  # Read .git/HEAD and the SHA and timestamp
  #indented_message(STATUS "gitdir is ${gitdir}")
  file(READ "${gitdir}/HEAD" HEAD)
  string(SUBSTRING "${HEAD}" 5 -1 HEAD)
  string(STRIP "${HEAD}" HEAD)
  #indented_message(STATUS "head is '${HEAD}'")
  if(EXISTS "${gitdir}/${HEAD}")
    file(READ "${gitdir}/${HEAD}" HEADSHA)
    string(STRIP "${HEADSHA}" HEADSHA)
    file(TIMESTAMP "${gitdir}/${HEAD}" HEADSTAMP "%Y-%m-%d %H:%M:%S +00:00" UTC)
    #indented_message(STATUS "Last commit was ${HEADSHA} at ${HEADSTAMP}")
    string(SUBSTRING "${HEADSHA}" 0 8 HEADUNIQUE)
    set(${outsha} ${HEADSHA} PARENT_SCOPE)
    set(${outtimestamp} ${HEADSTAMP} PARENT_SCOPE)
  endif()
endfunction()

# We expect a header file with macros like
# #define BOOST_AFIO_VERSION_MAJOR    2
# 
# The first macros with _MAJOR, _MINOR, _PATCH and _REVISION at their end are parsed
function(ParseProjectVersionFromHpp hppfile outvar)
  file(READ ${hppfile} HPPFILE)
  string(REGEX MATCH "#[ \t]*define[ \t].*_MAJOR[ \t]+([0-9]+)" MAJORVER "${HPPFILE}")
  set(MAJORVER ${CMAKE_MATCH_1})
  string(REGEX MATCH "#[ \t]*define[ \t].*_MINOR[ \t]+([0-9]+)" MINORVER "${HPPFILE}")
  set(MINORVER ${CMAKE_MATCH_1})
  string(REGEX MATCH "#[ \t]*define[ \t].*_PATCH[ \t]+([0-9]+)" PATCHVER "${HPPFILE}")
  set(PATCHVER ${CMAKE_MATCH_1})
  string(REGEX MATCH "#[ \t]*define[ \t].*_REVISION[ \t]+([0-9]+)" REVISIONVER "${HPPFILE}")
  set(REVISIONVER ${CMAKE_MATCH_1})
  set(${outvar} ${MAJORVER}.${MINORVER}.${PATCHVER}.${REVISIONVER} PARENT_SCOPE)
endfunction()

# We expect a header file like this:
#   // Comment
#   #define BOOST_AFIO_PREVIOUS_COMMIT_REF    x
#   #define BOOST_AFIO_PREVIOUS_COMMIT_DATE   "x"
#   #define BOOST_AFIO_PREVIOUS_COMMIT_UNIQUE x
# Lines 2, 3 and 4 need their ending rewritten
function(UpdateRevisionHppFromGit hppfile)
  #set(temphppfile "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}_revision.hpp")
  git_revision_from_path("${CMAKE_CURRENT_SOURCE_DIR}" HEADSHA HEADSTAMP)
  if(DEFINED HEADSHA)
    string(SUBSTRING "${HEADSHA}" 0 8 HEADUNIQUE)
    file(READ "${hppfile}" HPPFILE)
    string(REGEX MATCH "(.*\n.* )([a-f0-9]+)([\r\n]+.* \")(.*)(\"[\r\n]+.* )([a-f0-9]+)([\r\n]+.*)" txt1 "${HPPFILE}")
    set(txt1 "${CMAKE_MATCH_1}")
    set(OLDSHA "${CMAKE_MATCH_2}")
    set(txt2 "${CMAKE_MATCH_3}")
    set(OLDSTAMP "${CMAKE_MATCH_4}")
    set(txt3 "${CMAKE_MATCH_5}")
    set(OLDUNIQUE "${CMAKE_MATCH_6}")
    set(txt4 "${CMAKE_MATCH_7}")
    set(HPPFILE2 "${txt1}${HEADSHA}${txt2}${HEADSTAMP}${txt3}${HEADUNIQUE}${txt4}")
    if(NOT "${HPPFILE}" STREQUAL "${HPPFILE2}")
      file(WRITE "${hppfile}" "${HPPFILE2}")
    endif()
  endif()
endfunction()

# Apply OpenMP to a given target. Add REQUIRED to make it mandatory.
function(target_uses_openmp target)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    if(MSVC AND CLANG)
      # Currently doesn't work
    elseif(MSVC)
      target_compile_options(${target} PRIVATE ${OpenMP_CXX_FLAGS})
      return()
    else()
      target_compile_options(${target} PRIVATE ${OpenMP_CXX_FLAGS})
      set_target_properties(${target} PROPERTIES LINK_FLAGS -fopenmp)
      return()
    endif()
  endif()
  list(FIND ARGN "REQUIRED" required_idx)
  if(${required_idx} GREATER -1)
    indented_message(FATAL_ERROR "FATAL: Target ${target} requires OpenMP")
  endif()
endfunction()

# Preprocess a file
function(add_partial_preprocess target outfile infile)
  add_custom_target(${target} 
    ${PYTHON_EXECUTABLE} ${QuickCppLibCMakePath}/../pcpp/pcpp/cmd.py
    -o "${outfile}" "${infile}"
    ${ARGN}
    COMMENT "Preprocessing ${infile} into ${outfile} ..."
  )
endfunction()

# Finds a quickcpplib library
#
# quickcpplib libraries can be located via these means in order of preference:
# Only if "../.quickcpplib_use_siblings" exists:
#   1) "../${library}"                         (e.g. ../outcome)
# Otherwise it looks up ${library} in .gitmodules
#
# If we use a sibling edition, we update the current git index to point at the 
# git SHA of the sibling edition. That way when we git commit, we need not arse
# around with manually updating the embedded submodules.
function(find_quickcpplib_library libraryname version)
  if(NOT PROJECT_NAME)
    message(FATAL_ERROR "FATAL: find_quickcpplib_library() must only be called after a project()")
  endif()
  get_filename_component(boostishdir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
  if(IS_DIRECTORY "${boostishdir}/.quickcpplib_use_siblings")
    set(siblingenabled ON)
  else()
    set(siblingenabled OFF)
  endif()
  if(NOT DEFINED ${libraryname}_FOUND)
    unset(gitsubmodulepath)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules")
      # Read in .gitmodules and look for the dependency
      file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules" GITMODULESCONTENTS)
      if(GITMODULESCONTENTS MATCHES ".*\\n?\\[submodule \"([^\"]+\\/${libraryname})\"\\]")
        set(gitsubmodulepath "${CMAKE_MATCH_1}")
      endif()
    endif()
    if(NOT DEFINED gitsubmodulepath)
      message(FATAL_ERROR "FATAL: Dependent library ${libraryname} not found in .gitmodules")
    endif()
    # Prefer sibling editions of dependencies to embedded editions
    if(siblingenabled AND EXISTS "${boostishdir}/${libraryname}/.quickcpplib")
      set(GITREPO "${boostishdir}/${libraryname}")
      git_revision_from_path("${GITREPO}" GITSHA GITTS)
      indented_message(STATUS "Found ${libraryname} depended upon by ${PROJECT_NAMESPACE}${PROJECT_NAME} at sibling ../${libraryname} git revision ${GITSHA} last commit ${GITTS}")
      set(MESSAGE_INDENT "${MESSAGE_INDENT}  ")
      add_subdirectory("${GITREPO}"
        "${CMAKE_CURRENT_BINARY_DIR}/${libraryname}"
        EXCLUDE_FROM_ALL
      )
      # One of the only uses of a non-target specific cmake command anywhere,
      # but this is local to the calling CMakeLists.txt and is the correct
      # thing to use.
      include_directories(SYSTEM "${boostishdir}/.quickcpplib_use_siblings/a/a")
      include_directories(SYSTEM "${boostishdir}/.quickcpplib_use_siblings/a")
      include_directories(SYSTEM "${boostishdir}/.quickcpplib_use_siblings")
      include_directories(SYSTEM "${boostishdir}")
      set(${libraryname}_PATH "${GITREPO}")
      set(${libraryname}_FOUND TRUE)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}/.quickcpplib")
      indented_message(STATUS "Found ${libraryname} depended upon by ${PROJECT_NAMESPACE}${PROJECT_NAME} at embedded ${gitsubmodulepath}")
      set(MESSAGE_INDENT "${MESSAGE_INDENT}  ")
      add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}"
        EXCLUDE_FROM_ALL
      )
      # If we are using an embedded dependency, for any unit tests make the
      # dependencies appear as if at the same location as for the headers
      include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}/..")
      set(${libraryname}_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}")
      set(${libraryname}_FOUND TRUE)
    else()
      set(${libraryname}_FOUND FALSE)
    endif()
    # Reset policies after using add_subdirectory() which usually means a cmake_minimum_required()
    # was called which resets policies to default
    include(QuickCppLibPolicies)
    # I may need to update git submodule SHAs in the index to those of the sibling repo
    if(DEFINED GITSHA)
      # Get the SHA used by our repo for the subrepo
      git_repo_get_entry_sha("${CMAKE_CURRENT_SOURCE_DIR}" "${gitsubmodulepath}" subreposha)
      if(NOT DEFINED subreposha)
        message(FATAL_ERROR "FATAL: Failed to get a SHA for the subrepo '${gitsubmodulepath}'")
      endif()
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}/.git")
        # We need to delete any files inside the "${gitsubmodulepath}"
        # to prevent the submodule SHA restamp confusing git
        git_repo_changed("${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}" subrepoochanged)
        if(subrepoochanged)
          message(FATAL_ERROR "FATAL: About to replace embedded subrepo '${gitsubmodulepath}'"
                              " with sibling subrepo '${GITREPO}' but git says that embedded subrepo has been"
                              " changed. Please save any changes and hard reset the embedded subrepo.")
        endif()
        file(REMOVE_RECURSE "${gitsubmodulepath}")
        file(MAKE_DIRECTORY "${gitsubmodulepath}")
      endif()
      # Git uses the magic cacheinfo of 160000 for subrepos for some reason as can be evidenced by:
      # ned@LYTA:~/windocs/boostish/afio$ git ls-files -s | grep -e ^160000
      #   160000 cc293d14a48bf1ee3fb78743c3ad5cf61d63f3ff 0       doc/html
      #   160000 2682d240406a8a68be442227a6c15df8e2261b94 0       include/boost/afio/boost-lite
      #   160000 f436d33188b0117c1ecaa40ad9ebadabfdc69c3f 0       include/boost/afio/gsl-lite
      #   160000 7fb9617c21cae96e04f3a9afa54310a08ad87a57 0       include/boost/afio/outcome
      #   160000 7f583ce7cc36d2a8baefd3c09445457503614cb8 0       test/kerneltest
      if(NOT GITSHA STREQUAL subreposha)
        indented_message(STATUS "Sibling repo for embedded subrepo '${gitsubmodulepath}' has"
                                " SHA ${GITSHA} but our index uses SHA ${subreposha}, updating our index"
        )
        checked_execute_process("git update-index"
          COMMAND "${GIT_EXECUTABLE}" update-index --cacheinfo 160000 ${GITSHA} ${gitsubmodulepath}
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
      endif()
    endif()
  endif()
  # We don't cache this as we want to rerun the above git SHA stamping etc. per build
  set(${libraryname}_PATH "${${libraryname}_PATH}" PARENT_SCOPE)
  set(${libraryname}_FOUND ${${libraryname}_FOUND} PARENT_SCOPE)
  if(${libraryname}_FOUND)
    set(${PROJECT_NAME}_DEPENDENCIES ${${PROJECT_NAME}_DEPENDENCIES} "${libraryname}" PARENT_SCOPE)
  else()
    list(FIND ARGN "QUIET" quiet_idx)
    if(${quiet_idx} EQUAL -1)
      indented_message(WARNING "WARNING: quickcpplib library ${libraryname} depended upon by ${PROJECT_NAMESPACE}${PROJECT_NAME} not found")
      indented_message(STATUS "Tried: ")
      if(siblingenabled)
        indented_message(STATUS "  ${boostishdir}/${libraryname}/.quickcpplib")
      endif()
      indented_message(STATUS "  ${CMAKE_CURRENT_SOURCE_DIR}/${gitsubmodulepath}/.quickcpplib")
      if(NOT siblingenabled)
        indented_message(STATUS "  (sibling library use disabled due to lack of ${boostishdir}/.quickcpplib_use_siblings)")
      endif()
    endif()
    list(FIND ARGN "REQUIRED" required_idx)
    if(${required_idx} GREATER -1)
      indented_message(FATAL_ERROR "FATAL: quickcpplib library ${libraryname} required by ${PROJECT_NAMESPACE}${PROJECT_NAME} not found")
    endif()
  endif()
endfunction()



# Configures a CTest script with a sensible set of defaults
# for doing a configure, build, test and submission run
macro(CONFIGURE_CTEST_SCRIPT_FOR_CDASH projectname bindir)
  set(CTEST_PROJECT_NAME "${projectname}")
  set(CTEST_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set(CTEST_BINARY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${bindir}")
  set(CTEST_CONFIGURATION_TYPE Release)
  if(WIN32)
    if(NOT DEFINED CTEST_CMAKE_GENERATOR)
      # TODO Figure out how to use winclang via adding in -T v140_clang_c2
      set(CTEST_CMAKE_GENERATOR "Visual Studio 15 2017 Win64")
    endif()
    set(CTEST_CMAKE_CI_BIN_DIR "${bindir}/bin/${CTEST_CONFIGURATION_TYPE}")
    set(CTEST_SITE $ENV{COMPUTERNAME})
  else()
    if(NOT DEFINED CTEST_CMAKE_GENERATOR)
      set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
    endif()
    set(CTEST_CMAKE_CI_BIN_DIR "${bindir}/bin")
    set(CTEST_SITE $ENV{NAME})
  endif()
  set(CTEST_BUILD_NAME "${CMAKE_SYSTEM}-${CMAKE_SYSTEM_PROCESSOR}")

  find_program(CTEST_PYTHON_COMMAND NAMES python)
endmacro()

function(merge_junit_results_into_ctest_xml)
  if(NOT DEFINED CTEST_PYTHON_COMMAND)
    message(FATAL_ERROR "Please call the macro CONFIGURE_CTEST_SCRIPT_FOR_CDASH() to configure the ctest environment first")
  endif()
  # Merge all the junit XML files from the testing into one junit XML file
  execute_process(COMMAND "${CTEST_PYTHON_COMMAND}" "${CTEST_QUICKCPPLIB_SCRIPTS}/merge_junit_results.py" "${CTEST_BINARY_DIRECTORY}/merged_junit_results.xml" "${CTEST_CMAKE_CI_BIN_DIR}/*.junit.xml"
    RESULT_VARIABLE result
  )
  message(STATUS "Merging junit XML results into a single junit XML returned with status ${result}")
  # Figure out where this iteration's Test.xml lives
  file(READ "${CTEST_BINARY_DIRECTORY}/Testing/TAG" tag_file)
  string(REGEX MATCH "[^\n]*" xml_dir "${tag_file}")
  set(CTEST_XML_DIR "${CTEST_BINARY_DIRECTORY}/Testing/${xml_dir}")
  # Add the combined junit XML file into our Test.xml
  execute_process(COMMAND "${CTEST_PYTHON_COMMAND}" "${CTEST_QUICKCPPLIB_SCRIPTS}/add_junit_results_to_ctest.py" "${CTEST_XML_DIR}/Test.xml" "${CTEST_BINARY_DIRECTORY}/merged_junit_results.xml"
    RESULT_VARIABLE result
  )
  message(STATUS "Merging junit XML results into the CTest XML returned with status ${result}")
endfunction()
