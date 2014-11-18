#!/bin/sh
if [ -z "$CXX" ]; then
  CXX=g++
fi
EXTRA=
if [ "${CXX%++}" = "g" ] || [ "${CXX%++}" = "clang" ]; then
  EXTRA=-std=c++0x
fi
FAILED=0
mkdir -p build
if [ -e /usr/bin/python3 ]; then
  ../scripts/gen_guard_matrix.py BOOST_AFIO_NEED_DEFINE BOOST_AFIO_USE_BOOST_THREAD BOOST_AFIO_USE_BOOST_FILESYSTEM ASIO_STANDALONE > test_guard2.hpp
fi
for TEST in test_*.cpp; do
  FAIL=0
  PREPROCESSED=${TEST%.cpp}
  if [ -z "$VISUALSTUDIOVERSION" ] || [ "${VISUALSTUDIOVERSION%.0}" -gt "12" ]; then
    if [ -e "$CXX/$PREPROCESSED.i" ]; then
      "$CXX" -E -I.. $EXTRA "$TEST" > "build/$PREPROCESSED.it"
      sed '/^#/d' < "build/$PREPROCESSED.it" > "build/$PREPROCESSED.i"
      DIFF=$(diff -w -B "build/$PREPROCESSED.i" "$CXX/$PREPROCESSED.i")
      if [ $? -ne 0 ]; then
        echo ERROR: $TEST is not producing correct preprocessed output!
        echo "   " $DIFF
        FAIL=1
      fi
    fi
  fi
  rm -rf "build/$PREPROCESSED"
  OUTPUT=$($CXX -o "build/$PREPROCESSED" -I.. $EXTRA "$TEST")
  if [ $? -ne 0 ]; then
    echo ERROR: $TEST did not compile!
    echo "   " $OUTPUT
    FAIL=1
  else
    OUTPUT=$("build/$PREPROCESSED")
    if [ $? -ne 0 ]; then
      echo ERROR: $TEST returned failure!
      echo "   " $OUTPUT
      FAIL=1
    elif [ $FAIL -eq 0 ]; then
      echo PASS: $TEST
    fi
  fi
  if [ $FAIL -ne 0 ]; then
    FAILED=1
  fi
done
exit $FAILED
