/* Convenience macros for importing local namespace binds
(C) 2014-2017 Niall Douglas <http://www.nedproductions.biz/> (9 commits)
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

#ifndef BOOSTLITE_BIND_IMPORT_HPP
#define BOOSTLITE_BIND_IMPORT_HPP

/* 2014-10-9 ned: I lost today figuring out the below. I really hate the C preprocessor now.
 *
 * Anyway, infinity = 8. It's easy to expand below if needed.
 */
#include "detail/preprocessor_macro_overload.h"

#define BOOSTLITE_BIND_STRINGIZE(a) #a
#define BOOSTLITE_BIND_STRINGIZE2(a) BOOSTLITE_BIND_STRINGIZE(a)
#define BOOSTLITE_BIND_NAMESPACE_VERSION8(a, b, c, d, e, f, g, h) a##_##b##_##c##_##d##_##e##_##f##_##g##_##h
#define BOOSTLITE_BIND_NAMESPACE_VERSION7(a, b, c, d, e, f, g) a##_##b##_##c##_##d##_##e##_##f##_##g
#define BOOSTLITE_BIND_NAMESPACE_VERSION6(a, b, c, d, e, f) a##_##b##_##c##_##d##_##e##_##f
#define BOOSTLITE_BIND_NAMESPACE_VERSION5(a, b, c, d, e) a##_##b##_##c##_##d##_##e
#define BOOSTLITE_BIND_NAMESPACE_VERSION4(a, b, c, d) a##_##b##_##c##_##d
#define BOOSTLITE_BIND_NAMESPACE_VERSION3(a, b, c) a##_##b##_##c
#define BOOSTLITE_BIND_NAMESPACE_VERSION2(a, b) a##_##b
#define BOOSTLITE_BIND_NAMESPACE_VERSION1(a) a
//! Concatenates each parameter with _
#define BOOSTLITE_BIND_NAMESPACE_VERSION(...) BOOSTLITE_CALL_OVERLOAD(BOOSTLITE_BIND_NAMESPACE_VERSION, __VA_ARGS__)

#define BOOSTLITE_BIND_NAMESPACE_SELECT_2(name, modifier) name
#define BOOSTLITE_BIND_NAMESPACE_SELECT2(name, modifier) ::name
#define BOOSTLITE_BIND_NAMESPACE_SELECT_1(name) name
#define BOOSTLITE_BIND_NAMESPACE_SELECT1(name) ::name
#define BOOSTLITE_BIND_NAMESPACE_SELECT_(...) BOOSTLITE_CALL_OVERLOAD_(BOOSTLITE_BIND_NAMESPACE_SELECT_, __VA_ARGS__)
#define BOOSTLITE_BIND_NAMESPACE_SELECT(...) BOOSTLITE_CALL_OVERLOAD_(BOOSTLITE_BIND_NAMESPACE_SELECT, __VA_ARGS__)
#define BOOSTLITE_BIND_NAMESPACE_EXPAND8(a, b, c, d, e, f, g, h) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c BOOSTLITE_BIND_NAMESPACE_SELECT d BOOSTLITE_BIND_NAMESPACE_SELECT e BOOSTLITE_BIND_NAMESPACE_SELECT f BOOSTLITE_BIND_NAMESPACE_SELECT g BOOSTLITE_BIND_NAMESPACE_SELECT h
#define BOOSTLITE_BIND_NAMESPACE_EXPAND7(a, b, c, d, e, f, g) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c BOOSTLITE_BIND_NAMESPACE_SELECT d BOOSTLITE_BIND_NAMESPACE_SELECT e BOOSTLITE_BIND_NAMESPACE_SELECT f BOOSTLITE_BIND_NAMESPACE_SELECT g
#define BOOSTLITE_BIND_NAMESPACE_EXPAND6(a, b, c, d, e, f) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c BOOSTLITE_BIND_NAMESPACE_SELECT d BOOSTLITE_BIND_NAMESPACE_SELECT e BOOSTLITE_BIND_NAMESPACE_SELECT f
#define BOOSTLITE_BIND_NAMESPACE_EXPAND5(a, b, c, d, e) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c BOOSTLITE_BIND_NAMESPACE_SELECT d BOOSTLITE_BIND_NAMESPACE_SELECT e
#define BOOSTLITE_BIND_NAMESPACE_EXPAND4(a, b, c, d) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c BOOSTLITE_BIND_NAMESPACE_SELECT d
#define BOOSTLITE_BIND_NAMESPACE_EXPAND3(a, b, c) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b BOOSTLITE_BIND_NAMESPACE_SELECT c
#define BOOSTLITE_BIND_NAMESPACE_EXPAND2(a, b) BOOSTLITE_BIND_NAMESPACE_SELECT_ a BOOSTLITE_BIND_NAMESPACE_SELECT b
#define BOOSTLITE_BIND_NAMESPACE_EXPAND1(a) BOOSTLITE_BIND_NAMESPACE_SELECT_ a
//! Expands into a::b::c:: ...
#define BOOSTLITE_BIND_NAMESPACE(...) BOOSTLITE_CALL_OVERLOAD(BOOSTLITE_BIND_NAMESPACE_EXPAND, __VA_ARGS__)

#define BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT2(name, modifier)                                                                                                                                                                                                                                                            \
  modifier namespace name                                                                                                                                                                                                                                                                                                      \
  {
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT1(name)                                                                                                                                                                                                                                                                      \
  namespace name                                                                                                                                                                                                                                                                                                               \
  {
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT(...) BOOSTLITE_CALL_OVERLOAD_(BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT, __VA_ARGS__)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND8(a, b, c, d, e, f, g, h) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND7(b, c, d, e, f, g, h)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND7(a, b, c, d, e, f, g) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND6(b, c, d, e, f, g)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND6(a, b, c, d, e, f) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND5(b, c, d, e, f)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND5(a, b, c, d, e) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND4(b, c, d, e)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND4(a, b, c, d) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND3(b, c, d)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND3(a, b, c) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND2(b, c)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND2(a, b) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND1(b)
#define BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND1(a) BOOSTLITE_BIND_NAMESPACE_BEGIN_NAMESPACE_SELECT a

//! Expands into namespace a { namespace b { namespace c ...
#define BOOSTLITE_BIND_NAMESPACE_BEGIN(...) BOOSTLITE_CALL_OVERLOAD(BOOSTLITE_BIND_NAMESPACE_BEGIN_EXPAND, __VA_ARGS__)

#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT2(name, modifier)                                                                                                                                                                                                                                                            \
  modifier namespace name                                                                                                                                                                                                                                                                                                      \
  {
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT1(name)                                                                                                                                                                                                                                                                      \
  export namespace name                                                                                                                                                                                                                                                                                                               \
  {
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT(...) BOOSTLITE_CALL_OVERLOAD_(BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT, __VA_ARGS__)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND8(a, b, c, d, e, f, g, h) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND7(b, c, d, e, f, g, h)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND7(a, b, c, d, e, f, g) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND6(b, c, d, e, f, g)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND6(a, b, c, d, e, f) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND5(b, c, d, e, f)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND5(a, b, c, d, e) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND4(b, c, d, e)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND4(a, b, c, d) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND3(b, c, d)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND3(a, b, c) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND2(b, c)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND2(a, b) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND1(b)
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND1(a) BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_NAMESPACE_SELECT a

//! Expands into export namespace a { namespace b { namespace c ...
#define BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN(...) BOOSTLITE_CALL_OVERLOAD(BOOSTLITE_BIND_NAMESPACE_EXPORT_BEGIN_EXPAND, __VA_ARGS__)

#define BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT2(name, modifier) }
#define BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT1(name) }
#define BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT(...) BOOSTLITE_CALL_OVERLOAD_(BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT, __VA_ARGS__)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND8(a, b, c, d, e, f, g, h) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND7(b, c, d, e, f, g, h)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND7(a, b, c, d, e, f, g) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND6(b, c, d, e, f, g)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND6(a, b, c, d, e, f) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND5(b, c, d, e, f)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND5(a, b, c, d, e) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND4(b, c, d, e)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND4(a, b, c, d) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND3(b, c, d)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND3(a, b, c) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND2(b, c)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND2(a, b) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a BOOSTLITE_BIND_NAMESPACE_END_EXPAND1(b)
#define BOOSTLITE_BIND_NAMESPACE_END_EXPAND1(a) BOOSTLITE_BIND_NAMESPACE_END_NAMESPACE_SELECT a

//! Expands into } } ...
#define BOOSTLITE_BIND_NAMESPACE_END(...) BOOSTLITE_CALL_OVERLOAD(BOOSTLITE_BIND_NAMESPACE_END_EXPAND, __VA_ARGS__)

//! Expands into a static const char string array used to mark BindLib compatible namespaces
#define BOOSTLITE_BIND_DECLARE(decl, desc) static const char *boost_bindlib_out[] = {#decl, desc};

#endif
