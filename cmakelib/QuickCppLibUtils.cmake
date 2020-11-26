if(QuickCppLibUtilsIncluded)
  return()
endif()
set(QuickCppLibUtilsIncluded ON)
set(QuickCppLibCMakePath "${CMAKE_CURRENT_LIST_DIR}")
include(FindGit)
if(NOT GIT_FOUND)
  message(FATAL_ERROR "FATAL: The quickcpplib infrastructure is very tightly integrated with git"
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
function(_target_link_options target_name visibility)
  if(NOT visibility MATCHES "PRIVATE")
    message(FATAL_ERROR "FATAL: _target_link_options() used with non-PRIVATE visibility")
  endif()
  if(COMMAND target_link_options)
    target_link_options(${target_name} ${visibility} ${ARGN})
  else()
    # Convert args to a string
    string(REPLACE ";" " " props "${ARGN}")
    get_target_property(oldprops ${target_name} LINK_FLAGS)
    if(oldprops MATCHES "NOTFOUND")
      set(oldprops)
    endif()
    set_target_properties(${target_name} PROPERTIES
      LINK_FLAGS "${oldprops} ${props}"
    )
  endif()
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
  execute_process(${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE errout
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "FATAL: ${desc} failed with error '${result}'\n\nstdout was: ${out}\n\nstderr was: ${errout}")
  endif()
  #message("stdout was: ${out}\n\nstderr was: ${errout}")
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
function(add_partial_preprocess target outfile depend infile)
  if(EXISTS "${QuickCppLibCMakePath}/../pcpp/pcpp/pcmd.py")
    add_custom_command(OUTPUT "${outfile}"
      COMMAND "${PYTHON_EXECUTABLE}" "${QuickCppLibCMakePath}/../pcpp/pcpp/pcmd.py"
      -o "${outfile}" "${infile}"
      ${ARGN}
      DEPENDS "${depend}"
      COMMENT "Preprocessing ${infile} into ${outfile} ..."
    )
    add_custom_target(${target} DEPENDS "${outfile}")
  else()
    indented_message(WARNING "WARNING: '${QuickCppLibCMakePath}/../pcpp/pcpp/pcmd.py' was not found, cannot do partial preprocessing")
    add_custom_target(${target})
  endif()
endfunction()

# Have cmake download, build, and install some git repo
function(download_build_install)
  cmake_parse_arguments(DBI "" "NAME;DESTINATION;GIT_REPOSITORY;GIT_TAG" "CMAKE_ARGS;EXTERNALPROJECT_ARGS" ${ARGN})
  configure_file("${QuickCppLibCMakePath}/DownloadBuildInstall.cmake.in" "${DBI_DESTINATION}/CMakeLists.txt" @ONLY)
  checked_execute_process("Configure download, build and install of ${DBI_NAME} with ${DBI_CMAKE_ARGS}"
    COMMAND "${CMAKE_COMMAND}" .
    WORKING_DIRECTORY "${DBI_DESTINATION}"
  )
  checked_execute_process("Execute download, build and install of ${DBI_NAME} with ${DBI_CMAKE_ARGS}" 
    COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${DBI_DESTINATION}"
  )
endfunction()

# Finds a quickcpplib library
#
#          QUIET: print no messages
#       REQUIRED: fail if not found
#          LOCAL: always git clone a local edition instead of using cmake package edition
#        INBUILD: add the dependency as a part of this build instead of using the installed edition
# GIT_REPOSITORY: git repository to clone if library not found
#        GIT_TAG: git branch or tag or SHA to clone
#
# quickcpplib libraries can be located via these means in order of preference:
#
# 1. Only if "../.quickcpplib_use_siblings" exists, "../${libraryname}". These are always INBUILD.
#
# 2. If not LOCAL, find_package(${libraryname}).
#
# 3. ExternalProject_Add() to CMAKE_BINARY_DIR followed by find_package(${libraryname} NO_DEFAULT_PATH).
function(find_quickcpplib_library libraryname)
  if(NOT PROJECT_NAME)
    message(FATAL_ERROR "FATAL: find_quickcpplib_library() must only be called after a project()")
  endif()
  get_filename_component(boostishdir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
  if(IS_DIRECTORY "${boostishdir}/.quickcpplib_use_siblings" AND NOT QUICKCPPLIB_DISABLE_SIBLINGS)
    set(siblingenabled ON)
  else()
    set(siblingenabled OFF)
  endif()
  cmake_parse_arguments(FINDLIB "QUIET;REQUIRED;LOCAL;INBUILD;IS_HEADER_ONLY" "GIT_REPOSITORY;GIT_TAG" "" ${ARGN})
  if(FINDLIB_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "FATAL: No values given for keywords ${FINDLIB_KEYWORDS_MISSING_VALUES}")
  endif()
  if(FINDLIB_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "FATAL: Unrecognised arguments ${FINDLIB_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT FINDLIB_GIT_REPOSITORY)
    message(FATAL_ERROR "FATAL: GIT_REPOSITORY must be specified!")
  endif()
  set(FINDLIB_LOCAL_PATH)
  if(NOT ${libraryname}_FOUND)
    get_property(${libraryname}_FOUND GLOBAL PROPERTY ${libraryname}_FOUND)
  endif()
  if(NOT DEFINED QUICKCPPLIB_ROOT_BINARY_DIR)
    set(QUICKCPPLIB_ROOT_BINARY_DIR "${CMAKE_BINARY_DIR}")
  endif()
  if(NOT ${libraryname}_FOUND OR QUICKCPPLIB_REFRESH_FIND_LIBRARY_${libraryname})
    set(${libraryname}_FOUND FALSE)
    # Prefer sibling editions of dependencies
    if(siblingenabled AND EXISTS "${boostishdir}/${libraryname}/.quickcpplib")
      set(FINDLIB_LOCAL_PATH "${boostishdir}/${libraryname}")
      set(${libraryname}_FOUND TRUE)
    elseif(NOT FINDLIB_LOCAL)
      find_package(${libraryname} QUIET CONFIG)
      if(NOT ${libraryname}_FOUND)
        indented_message(STATUS "Missing dependency ${libraryname} is NOT installed in cmake packages!")
      endif()
    endif()
    
    if(NOT ${libraryname}_FOUND)
      if(FINDLIB_INBUILD AND EXISTS "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}")
        set(FINDLIB_LOCAL_PATH "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}")
        set(${libraryname}_FOUND TRUE)
      else()
        find_package(${libraryname} QUIET CONFIG NO_DEFAULT_PATH PATHS "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}")
        indented_message(STATUS "Missing dependency ${libraryname} is NOT found at ${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}!")
      endif()
    endif()
    if(NOT ${libraryname}_FOUND)
      foreach(config Debug Release RelWithDebInfo MinSizeRel)
        indented_message(STATUS "Superbuilding missing dependency ${libraryname} with config ${config}, this may take a while ...")
        set(cmakeargs "-DCMAKE_BUILD_TYPE=${config} -G \"${CMAKE_GENERATOR}\" -DPROJECT_IS_DEPENDENCY=TRUE \"-DQUICKCPPLIB_ROOT_BINARY_DIR=${QUICKCPPLIB_ROOT_BINARY_DIR}\"")
        if(DEFINED CMAKE_TOOLCHAIN_FILE)
          set(cmakeargs "${cmakeargs} \"-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}\"")
        endif()
        set(extraargs "GIT_SHALLOW 1;GIT_SUBMODULES \"\"")
        #indented_message(STATUS "DEBUG: download_build_install() QUICKCPPLIB_ROOT_BINARY_DIR = '${QUICKCPPLIB_ROOT_BINARY_DIR}'")
        #indented_message(STATUS "DEBUG: download_build_install() cmakeargs = '${cmakeargs}'")
        download_build_install(NAME ${libraryname}
          CMAKE_ARGS ${cmakeargs}
          EXTERNALPROJECT_ARGS ${extraargs}
          GIT_REPOSITORY "${FINDLIB_GIT_REPOSITORY}"
          GIT_TAG "${FINDLIB_GIT_TAG}"
          DESTINATION "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}"
        )
        if(FINDLIB_IS_HEADER_ONLY)
          break()
        endif()
      endforeach()
      if(FINDLIB_INBUILD)
        set(FINDLIB_LOCAL_PATH "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}")
        set(${libraryname}_FOUND TRUE)
      else()
        include(GNUInstallDirs)
        # Normally, the following ${libraryname}_DIR would not be needed if
        # PATHS "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}" were provided.
        # However, when CMAKE_SYSROOT is set, the paths provided by the PATHS option are assumed to be system paths
        # even when they are in the build directory, and get rewritten into the sysroot.
        set(${libraryname}_DIR "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}/${CMAKE_INSTALL_LIBDIR}/cmake/${libraryname}")
        find_package(${libraryname} CONFIG REQUIRED NO_DEFAULT_PATH)
      endif()
    endif()
    
    if(FINDLIB_LOCAL_PATH)
      set(MESSAGE_INDENT "${MESSAGE_INDENT}  ")
      set(PROJECT_IS_DEPENDENCY TRUE)
      add_subdirectory("${FINDLIB_LOCAL_PATH}"
        "${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}_sibling"
        EXCLUDE_FROM_ALL
      )
      set(${libraryname}_DIR "${FINDLIB_LOCAL_PATH}")
      set(${libraryname}_FOUND TRUE)
      # Reset policies after using add_subdirectory() which usually means a cmake_minimum_required()
      # was called which resets policies to default
      include(QuickCppLibPolicies)
    endif()
  endif()
  if(${libraryname}_FOUND)
    set_property(GLOBAL PROPERTY ${libraryname}_FOUND TRUE)
    set_property(GLOBAL PROPERTY ${libraryname}_DIR "${${libraryname}_DIR}")
    set_property(GLOBAL PROPERTY ${libraryname}_DEPENDENCIES "${${PROJECT_NAME}_DEPENDENCIES};${libraryname}")
  else()
    if(NOT FINDLIB_QUIET)
      indented_message(WARNING "WARNING: quickcpplib library ${libraryname} depended upon by ${PROJECT_NAMESPACE}${PROJECT_NAME} not found")
      indented_message(STATUS "Tried: ")
      if(siblingenabled)
        indented_message(STATUS "  ${boostishdir}/${libraryname}/.quickcpplib")
      endif()
      indented_message(STATUS "  ${QUICKCPPLIB_ROOT_BINARY_DIR}/${libraryname}/.quickcpplib")
      if(NOT siblingenabled)
        indented_message(STATUS "  (sibling library use disabled due to lack of ${boostishdir}/.quickcpplib_use_siblings)")
      endif()
      if(NOT FINDLIB_LOCAL)
        indented_message(STATUS "  find_package(${libraryname})")
      endif()
    endif()
    if(FINDLIB_REQUIRED)
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
      set(CTEST_CMAKE_GENERATOR "Visual Studio 16 2019")
      set(CTEST_CMAKE_GENERATOR_PLATFORM "x64")
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


# Figures out whether this compiler support C++ Concepts, and if so
# applies the appropriate config to the targets specified to enable them
function(apply_cxx_concepts_to visibility)
  include(CheckCXXSourceCompiles)
  # Do we have the Concepts TS?
  function(CheckCXXHasConcepts iter)
    set(CMAKE_REQUIRED_FLAGS ${ARGN})
    if(CMAKE_CXX_STANDARD)
      if(MSVC)
        if(CMAKE_CXX_STANDARD EQUAL 20)
          list(APPEND CMAKE_REQUIRED_FLAGS /std:c++latest)
        else()
          list(APPEND CMAKE_REQUIRED_FLAGS /std:c++${CMAKE_CXX_STANDARD})
        endif()
      elseif(CLANG OR GCC)
        list(APPEND CMAKE_REQUIRED_FLAGS -std=c++${CMAKE_CXX_STANDARD})
      endif()
    endif()
    check_cxx_source_compiles("
  #if !defined(_MSC_VER) && !defined(__clang__) && (__GNUC__ < 9 || __cplusplus < 202000L)
  #define OUTCOME_GCC6_CONCEPT_BOOL bool
  #else
  #define OUTCOME_GCC6_CONCEPT_BOOL
  #endif
  namespace detail
  {
    template <class T, class U> concept OUTCOME_GCC6_CONCEPT_BOOL SameHelper = true;
    template <class T, class U> concept OUTCOME_GCC6_CONCEPT_BOOL same_as = detail::SameHelper<T, U> &&detail::SameHelper<U, T>;
  }  // namespace detail
  template <class U> concept OUTCOME_GCC6_CONCEPT_BOOL ValueOrNone = requires(U a)
  {
    {
      a.has_value()
    }
    ->detail::same_as<bool>;
    {a.value()};
  };
  int main() { return 0; }
  " CXX_HAS_CONCEPTS${iter})
    set(CXX_HAS_CONCEPTS${iter} ${CXX_HAS_CONCEPTS${iter}} PARENT_SCOPE)
  endfunction()
  set(CXX_CONCEPTS_FLAGS "deduce" CACHE STRING "The flags to enable C++ Concepts for this compiler")
  if(CXX_CONCEPTS_FLAGS STREQUAL "deduce")
    set(HAVE_CONCEPTS 0)
    CheckCXXHasConcepts(_BY_DEFAULT)
    if(CXX_HAS_CONCEPTS_BY_DEFAULT)
      set(HAVE_CONCEPTS 1)
      set(CXX_CONCEPTS_FLAGS "" CACHE STRING "The flags to enable C++ Concepts for this compiler" FORCE)
      set(CXX_HAS_CONCEPTS "1" CACHE STRING "Whether this compiler supports C++ Concepts or not" FORCE)
    endif()
    if(NOT HAVE_CONCEPTS)
      if(CLANG OR GCC)
        CheckCXXHasConcepts(_CLANG_GCC -fconcepts)
        if(CXX_HAS_CONCEPTS_CLANG_GCC)
          set(CXX_CONCEPTS_FLAGS "-fconcepts" CACHE STRING "The flags to enable C++ Concepts for this compiler" FORCE)
          set(CXX_HAS_CONCEPTS "2" CACHE STRING "Whether this compiler supports C++ Concepts or not" FORCE)
          set(HAVE_CONCEPTS 2)
        endif()
      endif()
    endif()
    if(NOT HAVE_CONCEPTS)
      set(CXX_CONCEPTS_FLAGS "unsupported" CACHE STRING "The flags to enable C++ Concepts for this compiler" FORCE)
      set(CXX_HAS_CONCEPTS "0" CACHE STRING "Whether this compiler supports C++ Concepts or not" FORCE)
    endif()
  endif()
  if(CXX_CONCEPTS_FLAGS AND NOT CXX_CONCEPTS_FLAGS STREQUAL "unsupported")
    foreach(target ${ARGN})
      target_compile_options(${target} ${visibility} ${CXX_CONCEPTS_FLAGS})
    endforeach()
  endif()
endfunction()

# Figures out whether this compiler support C++ Coroutines, and if so
# applies the appropriate config to the targets specified to enable them
function(apply_cxx_coroutines_to visibility)
  include(CheckCXXSourceCompiles)
  # Do we have the Coroutines TS?
  function(CheckCXXHasCoroutines iter)
    set(CMAKE_REQUIRED_FLAGS ${ARGN})
    if(CMAKE_CXX_STANDARD)
      if(MSVC)
        if(CMAKE_CXX_STANDARD EQUAL 20)
          list(APPEND CMAKE_REQUIRED_FLAGS /std:c++latest)
        else()
          list(APPEND CMAKE_REQUIRED_FLAGS /std:c++${CMAKE_CXX_STANDARD})
        endif()
      elseif(CLANG OR GCC)
        list(APPEND CMAKE_REQUIRED_FLAGS -std=c++${CMAKE_CXX_STANDARD})
      endif()
    endif()
    check_cxx_source_compiles("
#if __has_include(<coroutine>) && (!defined(_MSC_VER) || _HAS_CXX20)
#include <coroutine>
using std::suspend_never;
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
using std::experimental::suspend_never;
#endif
class resumable{
public:
  struct promise_type
  {
    resumable get_return_object() { return {}; }
    auto initial_suspend() { return suspend_never(); }
    auto final_suspend() noexcept { return suspend_never(); }
    void return_value(int) { }
    void unhandled_exception() {}
  };
  bool resume() { return true; }
  int get() { return 0; }
};
resumable g() { co_return 0; }
int main() { return g().get(); }
" CXX_HAS_COROUTINES${iter})
    set(CXX_HAS_COROUTINES${iter} ${CXX_HAS_COROUTINES${iter}} PARENT_SCOPE)
  endfunction()
  set(CXX_COROUTINES_FLAGS "deduce" CACHE STRING "The flags to enable C++ Coroutines for this compiler")
  set(CXX_COROUTINES_LINKER_FLAGS "" CACHE STRING "The linker flags to enable C++ Coroutines for this compiler")
  if(CXX_COROUTINES_FLAGS STREQUAL "deduce")
    set(HAVE_COROUTINES 0)
    CheckCXXHasCoroutines(_BY_DEFAULT)
    if(CXX_HAS_COROUTINES_BY_DEFAULT)
      set(HAVE_COROUTINES 1)
      set(CXX_COROUTINES_FLAGS "" CACHE STRING "The flags to enable C++ Coroutines for this compiler" FORCE)
      set(CXX_HAS_COROUTINES "1" CACHE STRING "Whether this compiler has C++ Coroutines or not" FORCE)
    endif()
    if(NOT HAVE_COROUTINES)
      if(MSVC)
        CheckCXXHasCoroutines(_MSVC "/EHsc /await")
        if(CXX_HAS_COROUTINES_MSVC)
          set(CXX_COROUTINES_FLAGS "/await" CACHE STRING "The flags to enable C++ Coroutines for this compiler" FORCE)
          set(CXX_HAS_COROUTINES "2" CACHE STRING "Whether this compiler has C++ Coroutines or not" FORCE)
          set(HAVE_COROUTINES 2)
        endif()
      endif()
      if(CLANG OR GCC)
        CheckCXXHasCoroutines(_WITH_FLAG "-fcoroutines")
        if(CXX_HAS_COROUTINES_WITH_FLAG)
          set(CXX_COROUTINES_FLAGS "-fcoroutines" CACHE STRING "The flags to enable C++ Coroutines for this compiler" FORCE)
          set(CXX_HAS_COROUTINES "2" CACHE STRING "Whether this compiler has C++ Coroutines or not" FORCE)
          set(HAVE_COROUTINES 2)
        endif()
        CheckCXXHasCoroutines(_WITH_FLAG_TS "-fcoroutines-ts")
        if(CXX_HAS_COROUTINES_WITH_FLAG_TS)
          set(CXX_COROUTINES_FLAGS "-fcoroutines-ts" CACHE STRING "The flags to enable C++ Coroutines for this compiler" FORCE)
          set(CXX_HAS_COROUTINES "2" CACHE STRING "Whether this compiler has C++ Coroutines or not" FORCE)
          set(HAVE_COROUTINES 2)
        endif()
      endif()
    endif()
    if(NOT HAVE_COROUTINES)
      set(CXX_COROUTINES_FLAGS "unsupported" CACHE STRING "The flags to enable C++ Coroutines for this compiler" FORCE)
      set(CXX_HAS_COROUTINES "0" CACHE STRING "Whether this compiler has C++ Coroutines or not" FORCE)
    endif()
  endif()
  if(CXX_COROUTINES_FLAGS AND NOT CXX_COROUTINES_FLAGS STREQUAL "unsupported")
    foreach(target ${ARGN})
      target_compile_options(${target} ${visibility} ${CXX_COROUTINES_FLAGS})
      if(CXX_COROUTINES_LINKER_FLAGS)
        _target_link_options(${target} ${visibility} ${CXX_COROUTINES_LINKER_FLAGS})
      endif()
    endforeach()
  endif()
endfunction()

function(ensure_git_subrepo path url)
  if(NOT EXISTS "${path}")
    include(FindGit)
    message(STATUS "NOTE: Due to missing ${path}, running ${GIT_EXECUTABLE} submodule update --init --recursive --depth 1 --jobs 8 ...")
    execute_process(COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive --depth 1 --jobs 8
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE retcode
    )
    if(retcode)
      # Try pulling repo directly from github
      get_filename_component(path "${path}" DIRECTORY)
      get_filename_component(path "${path}" DIRECTORY)
      message(WARNING "WARNING: git submodule update failed with code ${retcode}, trying a direct git clone ...")
      execute_process(COMMAND "${GIT_EXECUTABLE}" clone --recurse-submodules --depth 1 --jobs 8 --shallow-submodules ${url} ${ARGN}
        WORKING_DIRECTORY "${path}"
        RESULT_VARIABLE retcode
      )
      if(retcode)
        message(FATAL_ERROR "FATAL: git clone failed with code ${retcode}")
      endif()
    endif()
  endif()
endfunction()
