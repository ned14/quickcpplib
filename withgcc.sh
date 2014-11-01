rm -rf genmap
mkdir -p bind/stl11/boost bind/stl11/std bind/stl1z/boost bind/stl1z/std bind/stl1z/asio
clang++ -o genmap genmap.cpp -g -O3 -std=c++11 -I/usr/lib/llvm-3.4/include /usr/lib/llvm-3.4/lib/libclang.so


# Generate STL11 bindings
./genmap bind/stl11/boost/atomic BOOST_STL11_ATOMIC_MAP_ "boost::([^_][^:]*),boost::atomics::([^_][^:]*)" boost/atomic.hpp "std::([^_][^:]*)" atomic
./genmap bind/stl11/std/atomic BOOST_STL11_ATOMIC_MAP_ "std::([^_][^:]*)" atomic "boost::([^_][^:]*),boost::atomics::([^_][^:]*)" boost/atomic.hpp

./genmap bind/stl11/boost/array BOOST_STL11_ARRAY_MAP_ "boost::([^_][^:]*)" boost/array.hpp "std::([^_][^:]*)" array
./genmap bind/stl11/std/array BOOST_STL11_ARRAY_MAP_ "std::([^_][^:]*)" array "boost::([^_][^:]*)" boost/array.hpp

./genmap bind/stl11/boost/chrono BOOST_STL11_CHRONO_MAP_ "boost::chrono::([^_].*)" boost/chrono.hpp "std::chrono::([^_].*)" chrono
./genmap bind/stl11/std/chrono BOOST_STL11_CHRONO_MAP_ "std::chrono::([^_].*)" chrono "boost::chrono::([^_].*)" boost/chrono.hpp

./genmap bind/stl11/boost/condition_variable BOOST_STL11_CONDITION_VARIABLE_MAP_ "boost::([^_][^:]*)" boost/thread.hpp "std::([^_][^:]*)" condition_variable
./genmap bind/stl11/std/condition_variable BOOST_STL11_CONDITION_VARIABLE_MAP_ "std::([^_][^:]*)" condition_variable "boost::([^_][^:]*)" boost/thread.hpp

./genmap bind/stl1z/boost/filesystem BOOST_STL1z_FILESYSTEM_MAP_ "boost::filesystem::([^_][^:]*)" boost/filesystem.hpp
#sed -e 's/boost\/filesystem.hpp/filesystem/g' -e 's/boost::filesystem/std::tr2::sys/g' bind/stl1z/boost/filesystem > bind/stl1z/std/filesystem

./genmap bind/stl11/boost/functional BOOST_STL11_FUNCTIONAL_MAP_ "boost::([^_][^:]*)" boost/bind.hpp "std::([^_][^:]*)" functional
./genmap bind/stl11/std/functional BOOST_STL11_FUNCTIONAL_MAP_ "std::([^_][^:]*)" functional "boost::([^_][^:]*)" boost/bind.hpp

./genmap bind/stl11/boost/future BOOST_STL11_FUTURE_MAP_ "boost::([^_][^:]*)" boost/thread.hpp "std::([^_][^:]*)" future
./genmap bind/stl11/std/future BOOST_STL11_FUTURE_MAP_ "std::([^_][^:]*)" future "boost::([^_][^:]*)" boost/thread.hpp

./genmap bind/stl11/boost/mutex BOOST_STL11_MUTEX_MAP_ "boost::([^_][^:]*)" boost/thread.hpp "std::([^_][^:]*)" mutex
./genmap bind/stl11/std/mutex BOOST_STL11_MUTEX_MAP_ "std::([^_][^:]*)" mutex "boost::([^_][^:]*)" boost/thread.hpp

./genmap bind/stl1z/boost/networking BOOST_STL1z_NETWORKING_MAP_ "boost::asio::([^_d].*)" boost/asio.hpp
sed -e 's/boost\/asio.hpp/networking/g' -e 's/boost/std::experimental/g' bind/stl1z/boost/networking > bind/stl1z/std/networking
CPLUS_INCLUDE_PATH=/home/ned/boost.afio/asio/asio/include ./genmap bind/stl1z/asio/networking BOOST_STL1z_NETWORKING_MAP_ "asio::([^_d].*)" asio.hpp || true
#sed -e 's/boost\/asio.hpp/asio.hpp/g' -e 's/boost::asio/asio/g' bind/stl1z/boost/networking > bind/stl1z/asio/networking

./genmap bind/stl11/boost/random BOOST_STL11_RANDOM_MAP_ "boost::random::([^_][^:]*)" boost/random.hpp "std::([^_][^:]*)" random
./genmap bind/stl11/std/random BOOST_STL11_RANDOM_MAP_ "std::([^_][^:]*)" random "boost::random::([^_][^:]*)" boost/random.hpp

./genmap bind/stl11/boost/ratio BOOST_STL11_RATIO_MAP_ "boost::([^_][^:]*)" boost/ratio.hpp "std::([^_][^:]*)" ratio
./genmap bind/stl11/std/ratio BOOST_STL11_RATIO_MAP_ "std::([^_][^:]*)" ratio "boost::([^_][^:]*)" boost/ratio.hpp

./genmap bind/stl11/boost/regex BOOST_STL11_REGEX_MAP_ "boost::([^_][^:]*)" boost/regex.hpp "std::([^_][^:]*)" regex
./genmap bind/stl11/std/regex BOOST_STL11_REGEX_MAP_ "std::([^_][^:]*)" regex "boost::([^_][^:]*)" boost/regex.hpp

./genmap bind/stl11/boost/system_error BOOST_STL11_SYSTEM_ERROR_MAP_ "boost::system::([^_][^:]*)" boost/system/system_error.hpp "std::([^_][^:]*)" system_error
./genmap bind/stl11/std/system_error BOOST_STL11_SYSTEM_ERROR_MAP_ "std::([^_][^:]*)" system_error "boost::system::([^_][^:]*)" boost/system/system_error.hpp

./genmap bind/stl11/boost/thread BOOST_STL11_THREAD_MAP_ "boost::([^_][^:]*),boost::(this_thread::[^_][^:]*)" boost/thread.hpp "std::([^_][^:]*),std::(this_thread::[^_][^:]*)" thread
./genmap bind/stl11/std/thread BOOST_STL11_THREAD_MAP_ "std::([^_][^:]*),std::(this_thread::[^_][^:]*)" thread "boost::([^_][^:]*),boost::(this_thread::[^_][^:]*)" boost/thread.hpp

./genmap bind/stl11/boost/tuple BOOST_STL11_TUPLE_MAP_ "boost::tuples::([^_][^:]*)" boost/tuple/tuple.hpp "std::([^_][^:]*)" tuple
./genmap bind/stl11/std/tuple BOOST_STL11_TUPLE_MAP_ "std::([^_][^:]*)" tuple "boost::tuples::([^_][^:]*)" boost/tuple/tuple.hpp

./genmap bind/stl11/std/typeindex BOOST_STL11_TYPEINDEX_MAP_ "std::([^_][^:]*)" typeindex

./genmap bind/stl11/boost/type_traits BOOST_STL11_TYPE_TRAITS_MAP_ "boost::([^_][^:]*)" boost/type_traits.hpp "std::([^_][^:]*)" type_traits
./genmap bind/stl11/std/type_traits BOOST_STL11_TYPE_TRAITS_MAP_ "std::([^_][^:]*)" type_traits "boost::([^_][^:]*)" boost/type_traits.hpp

