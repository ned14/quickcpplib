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
  file(READ "${hppfile}" HPPFILE)
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
  message(STATUS "gitdir is ${gitdir}")
  file(READ "${gitdir}/HEAD" HEAD)
  string(SUBSTRING "${HEAD}" 5 -1 HEAD)
  string(STRIP "${HEAD}" HEAD)
  message(STATUS "head is '${HEAD}'")
  file(READ "${gitdir}/${HEAD}" HEADSHA)
  string(STRIP "${HEADSHA}" HEADSHA)
  file(TIMESTAMP "${gitdir}/${HEAD}" HEADSTAMP)
  message(STATUS "Last commit was ${HEADSHA} at ${HEADSTAMP}")
endfunction()
