rm -rf genmap
clang++ -o genmap genmap.cpp -g -std=c++11 -I/usr/lib/llvm-3.4/include /usr/lib/llvm-3.4/lib/libclang.so
./genmap include/atomic "std::[^_].*" atomic
./genmap include/chrono "std::[^_].*" chrono
./genmap include/mutex "std::[^_].*" mutex
./genmap include/thread "std::[^_].*" thread
