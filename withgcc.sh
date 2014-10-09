rm -rf genmap
clang++ -o genmap genmap.cpp -g -O3 -std=c++11 -I/usr/lib/llvm-3.4/include /usr/lib/llvm-3.4/lib/libclang.so
./genmap bind/stl11/atomic BOOST_STL11_MAP_ "std::([^_][^:]*)" atomic "boost::([^_][^:]*)" boost/atomic.hpp
./genmap bind/stl11/array BOOST_STL11_MAP_ "std::([^_][^:]*)" array "boost::([^_][^:]*)" boost/array.hpp
./genmap bind/stl11/chrono BOOST_STL11_MAP_ "std::chrono::([^_][^:]*)" chrono "boost::chrono::([^_][^:]*)" boost/chrono.hpp
./genmap bind/stl11/condition_variable BOOST_STL11_MAP_ "std::([^_][^:]*)" condition_variable "boost::([^_][^:]*)" boost/thread.hpp
./genmap bind/stl11/filesystem BOOST_STL11_MAP_ "boost::filesystem::([^_][^:]*)" boost/filesystem.hpp
./genmap bind/stl11/functional BOOST_STL11_MAP_ "std::([^_][^:]*)" functional "boost::([^_][^:]*)" boost/bind.hpp
./genmap bind/stl11/future BOOST_STL11_MAP_ "std::([^_][^:]*)" future "boost::([^_][^:]*)" boost/thread.hpp
./genmap bind/stl11/mutex BOOST_STL11_MAP_ "std::([^_][^:]*)" mutex "boost::([^_][^:]*)" boost/thread.hpp
./genmap bind/stl11/random BOOST_STL11_MAP_ "std::([^_][^:]*)" random "boost::random::([^_][^:]*)" boost/random.hpp
./genmap bind/stl11/ratio BOOST_STL11_MAP_ "std::([^_][^:]*)" ratio "boost::([^_][^:]*)" boost/ratio.hpp
./genmap bind/stl11/regex BOOST_STL11_MAP_ "std::([^_][^:]*)" regex "boost::([^_][^:]*)" boost/regex.hpp
./genmap bind/stl11/system_error BOOST_STL11_MAP_ "std::([^_][^:]*)" system_error "boost::system::([^_][^:]*)" boost/system/system_error.hpp
./genmap bind/stl11/thread BOOST_STL11_MAP_ "std::([^_][^:]*),std::(this_thread::[^_][^:]*)" thread "boost::([^_][^:]*),boost::(this_thread::[^_][^:]*)" boost/thread.hpp
./genmap bind/stl11/tuple BOOST_STL11_MAP_ "std::([^_][^:]*)" tuple "boost::tuples::([^_][^:]*)" boost/tuple/tuple.hpp
./genmap bind/stl11/typeindex BOOST_STL11_MAP_ "std::([^_][^:]*)" typeindex
./genmap bind/stl11/type_traits BOOST_STL11_MAP_ "std::([^_][^:]*)" type_traits "boost::([^_][^:]*)" boost/type_traits.hpp

