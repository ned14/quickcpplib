/* MSVC capable preprocessor macro overloading
(C) 2014-2017 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Aug 2014


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOSTLITE_PREPROCESSOR_MACRO_OVERLOAD_H
#define BOOSTLITE_PREPROCESSOR_MACRO_OVERLOAD_H

#define BOOSTLITE_GLUE(x, y) x y

#define BOOSTLITE_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, count, ...) count
#define BOOSTLITE_EXPAND_ARGS(args) BOOSTLITE_RETURN_ARG_COUNT args
#define BOOSTLITE_COUNT_ARGS_MAX8(...) BOOSTLITE_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define BOOSTLITE_OVERLOAD_MACRO2(name, count) name##count
#define BOOSTLITE_OVERLOAD_MACRO1(name, count) BOOSTLITE_OVERLOAD_MACRO2(name, count)
#define BOOSTLITE_OVERLOAD_MACRO(name, count) BOOSTLITE_OVERLOAD_MACRO1(name, count)

#define BOOSTLITE_CALL_OVERLOAD(name, ...) BOOSTLITE_GLUE(BOOSTLITE_OVERLOAD_MACRO(name, BOOSTLITE_COUNT_ARGS_MAX8(__VA_ARGS__)), (__VA_ARGS__))

#define BOOSTLITE_GLUE_(x, y) x y

#define BOOSTLITE_RETURN_ARG_COUNT_(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, count, ...) count
#define BOOSTLITE_EXPAND_ARGS_(args) BOOSTLITE_RETURN_ARG_COUNT_ args
#define BOOSTLITE_COUNT_ARGS_MAX8_(...) BOOSTLITE_EXPAND_ARGS_((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define BOOSTLITE_OVERLOAD_MACRO2_(name, count) name##count
#define BOOSTLITE_OVERLOAD_MACRO1_(name, count) BOOSTLITE_OVERLOAD_MACRO2_(name, count)
#define BOOSTLITE_OVERLOAD_MACRO_(name, count) BOOSTLITE_OVERLOAD_MACRO1_(name, count)

#define BOOSTLITE_CALL_OVERLOAD_(name, ...) BOOSTLITE_GLUE_(BOOSTLITE_OVERLOAD_MACRO_(name, BOOSTLITE_COUNT_ARGS_MAX8_(__VA_ARGS__)), (__VA_ARGS__))

#endif
