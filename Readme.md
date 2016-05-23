# Boost-lite - Abstraction layer for the Boost C++ Libraries

(C) 2014-2016 Niall Douglas http://www.nedproductions.biz/

Linux: [![Build Status](https://travis-ci.org/ned14/boost-lite.svg?branch=master)](https://travis-ci.org/ned14/boost-lite) Windows: [![Build status](https://ci.appveyor.com/api/projects/status/y84ca5mdqwrstaw0?svg=true)](https://ci.appveyor.com/project/ned14/boost-apibind)

Herein is a set of tooling to enable a Boost library to swap in the C++ 11 standard library
instead of Boost, thus making it optionally independent of Boost. The following libraries
are swappable:

* array
* atomic
* chrono
* condition_variable
* filesystem
* functional
* future
* mutex
* random
* ratio
* regex
* system_error
* thread
* tuple
* type_traits
* typeindex

Examine `include/import.h` for macros helping bind these. A clang AST based bindings generator
which compares two namespaces and extracts the interoperable definitions can be found in
`build/genmap.cpp`. Be aware that you must port/write your Boost library to use this abstraction
layer instead of Boost, it requires substantial code refactoring. See Boost.Outcome or
Boost.AFIO for examples of practice.

The usual Boost config macros like `BOOST_CONSTEXPR` are defined for you in
`include/boost/config.hpp`. A minimum emulation of Boost.Test is available in
`include/boost/test/unit_test.hpp` which uses a threadsafe edition of the CATCH C++ unit test
library.

Boost-lite is compatible with Boost - just make SURE you include Boost first in a translation
unit to avoid compiler warnings.

Other useful bits and pieces have snuck in over time:

* `include/cpp_feature.h`: Makes consistent a set of C++ 17 feature test macros on any compiler.
* `include/execinfo_win64.h`: Implements `backtrace()` and `backtrace_symbols()` from glibc on
Windows.
* `include/ringbuffer_log.hpp`: Implements a very high performance fixed size threadsafe
ringbuffer logger with a STL container interface. Can take stack backtraces, log additional
data, indeed Boost.Outcome misuses it to retain extended error information when doing lossy
error state conversions.
* `include/spinlock.hpp`: A family of policy driven spinlocks which can substitute in for
a mutex.
* `include/tribool.hpp`: A constexpr three state boolean. Useful for ternary logic.
