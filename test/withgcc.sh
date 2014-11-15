#!/bin/sh
FAIL=0
mkdir -p build
for TEST in test_*.cpp; do
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
  g++ -o build/$PREPROCESSED -I.. $TEST
done
exit $FAIL
