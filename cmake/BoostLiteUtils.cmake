if(BoostLiteUtilsIncluded)
  return()
endif()
set(BoostLiteUtilsIncluded ON)

# Returns a path with forward slashes replaced with backslashes on WIN32
function(NativisePath outvar)
  list(REMOVE_AT ARGV 0)
  if(WIN32)
    string(REPLACE "/" "\\" new ${ARGV})
  else()
    set(new ${ARGV})
  endif()
  set(${outvar} ${new} PARENT_SCOPE)
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
#   #define BOOST_AFIO_PREVIOUS_COMMIT_REF  x
#   #define BOOST_AFIO_PREVIOUS_COMMIT_DATE "x"
# Lines 2 and 3 need their ending rewritten
function(UpdateRevisionHppFromGit hppfile)
  set(gitdir "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  if(NOT IS_DIRECTORY "${gitdir}")
    file(READ "${gitdir}" pathtogitdir)
    # This will have the form:
    # gitdir: ../../../../.git/modules/include/boost/afio/boost-lite
    string(SUBSTRING "${pathtogitdir}" 8 -1 pathtogitdir)
    string(STRIP "${pathtogitdir}" pathtogitdir)
    set(gitdir "${CMAKE_CURRENT_SOURCE_DIR}/${pathtogitdir}")
  endif()
  # Read .git/HEAD and the SHA and timestamp
  #message(STATUS "gitdir is ${gitdir}")
  file(READ "${gitdir}/HEAD" HEAD)
  string(SUBSTRING "${HEAD}" 5 -1 HEAD)
  string(STRIP "${HEAD}" HEAD)
  #message(STATUS "head is '${HEAD}'")
  file(READ "${gitdir}/${HEAD}" HEADSHA)
  string(STRIP "${HEADSHA}" HEADSHA)
  file(TIMESTAMP "${gitdir}/${HEAD}" HEADSTAMP "%Y-%m-%d %H:%M:%S +00:00" UTC)
  #message(STATUS "Last commit was ${HEADSHA} at ${HEADSTAMP}")

  file(READ "${hppfile}" HPPFILE)
  string(REGEX MATCH "(.*\n.* )([a-f0-9]+)([\r\n]+.* \")(.*)(\"[\r\n]+.*)" txt1 "${HPPFILE}")
  set(txt1 "${CMAKE_MATCH_1}")
  set(OLDSHA "${CMAKE_MATCH_2}")
  set(txt2 "${CMAKE_MATCH_3}")
  set(OLDSTAMP "${CMAKE_MATCH_4}")
  set(txt3 "${CMAKE_MATCH_5}")
  if(NOT HEADSHA STREQUAL OLDSHA)
    set(HPPFILE "${txt1}${HEADSHA}${txt2}${HEADSTAMP}${txt3}")
    file(WRITE "${hppfile}" "${HPPFILE}")
  endif()
endfunction()

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

