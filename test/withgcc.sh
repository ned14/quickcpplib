#!/bin/sh
FAILED=0
mkdir -p build
../scripts/gen_guard_matrix.py BOOST_AFIO_NEED_DEFINE BOOST_AFIO_USE_BOOST_THREAD BOOST_AFIO_USE_BOOST_FILESYSTEM ASIO_STANDALONE > test_guard2.hpp
for TEST in test_*.cpp; do
  FAIL=0
  PREPROCESSED=${TEST%.cpp}
  if [ -e $PREPROCESSED.i ]; then
    g++ -E -o build/$PREPROCESSED.it -I.. $TEST
    sed '/^#/d' < build/$PREPROCESSED.it > build/$PREPROCESSED.i
    DIFF=$(diff build/$PREPROCESSED.i $PREPROCESSED.i)
    if [ $? -ne 0 ]; then
      echo ERROR: $TEST is not producing correct preprocessed output!
      echo "   " $DIFF
      FAIL=1
    fi
  fi
  rm -rf build/$PREPROCESSED
  OUTPUT=$(g++ -o build/$PREPROCESSED -I.. $TEST)
  if [ $? -ne 0 ]; then
    echo ERROR: $TEST did not compile!
    echo "   " $OUTPUT
    FAIL=1
  else
    OUTPUT=$(build/$PREPROCESSED)
    if [ $? -ne 0 ]; then
      echo ERROR: $TEST returned failure!
      echo "   " $OUTPUT
      FAIL=1
    elif [ $FAIL -eq 0 ]; then
      echo PASS: $TEST
    fi
  fi
done
exit $FAILED
