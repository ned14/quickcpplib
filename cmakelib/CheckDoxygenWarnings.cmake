# Fail the docs target when doxygen reported any warning.
#
# This check used to be a shell snippet held in a CMake list, which cannot work
# for two independent reasons: `set(x if test -s f $<SEMICOLON> then ... fi)`
# stores one list element per word, so COMMAND ${x} execs a program called `if`;
# and the $<SEMICOLON> spelling that keeps the snippet in a single element loses
# its quoting when Ninja writes the command line, so the `;` splits the outer
# shell invocation instead. Either way the target dies for a reason that has
# nothing to do with doxygen - on every platform, including the WIN32 branch,
# which had the same defect.
#
# A `cmake -P` script has neither problem: no shell, no semicolons, one
# implementation for every platform instead of two, and it can be run on its own
# to show what it does.
#
# Invoked as: cmake -DWARNINGS_FILE=<path> -P CheckDoxygenWarnings.cmake
#
# `file(SIZE)` would be clearer but needs CMake 3.14 and this project supports
# 3.10, so the length is measured from the content.

if(NOT DEFINED WARNINGS_FILE)
  message(FATAL_ERROR "CheckDoxygenWarnings.cmake needs -DWARNINGS_FILE=<path to the doxygen warning log>")
endif()

# Doxygen always writes the log, but a docs target that ran as far as this check
# without one is a configuration problem rather than "no warnings", so say so.
if(NOT EXISTS "${WARNINGS_FILE}")
  message(FATAL_ERROR "Failing due to doxygen warnings: '${WARNINGS_FILE}' was never written, so the docs target cannot be judged clean")
endif()

file(READ "${WARNINGS_FILE}" warnings_body)
string(LENGTH "${warnings_body}" warnings_length)
if(warnings_length GREATER 0)
  message(FATAL_ERROR "Failing due to doxygen warnings:\n${warnings_body}")
endif()
