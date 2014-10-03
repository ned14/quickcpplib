rm -rf genmap
clang++ -o genmap genmap.cpp -g -std=c++11 -I/usr/lib/llvm-3.4/include /usr/lib/llvm-3.4/lib/libclang.so
./genmap include/atomic "std::([^_].*)" atomic "boost::([^_].*)" boost/atomic.hpp
./genmap include/chrono "std::([^_].*)" chrono "boost::([^_].*)" boost/chrono.hpp
./genmap include/mutex "std::([^_].*)" mutex "boost::([^_].*)" boost/thread.hpp
./genmap include/thread "std::([^_].*)" thread "boost::([^_].*)" boost/thread.hpp
