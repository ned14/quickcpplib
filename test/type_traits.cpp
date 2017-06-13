/* Unit testing for traits
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (3 commits)


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

#include "../include/boost/test/unit_test.hpp"
#include "../include/type_traits.hpp"

#include <array>
#include <initializer_list>
#include <vector>

BOOST_AUTO_TEST_SUITE(all)

BOOST_AUTO_TEST_CASE(works / type_traits, "Tests that the type traits work as intended")
{
  using namespace QUICKCPPLIB_NAMESPACE::type_traits;
  static_assert(!is_sequence<char>::value, "");
  static_assert(!is_sequence<void>::value, "");
  static_assert(is_sequence<std::array<char, 2>>::value, "");
  static_assert(is_sequence<std::initializer_list<char>>::value, "");
  static_assert(is_sequence<std::vector<char>>::value, "");
  struct Foo  // NOLINT
  {
    Foo() = delete;
    explicit Foo(int /*unused*/) {}
    Foo(const Foo & /*unused*/) = delete;
    Foo(Foo && /*unused*/) noexcept {}
  };
  static_assert(!is_sequence<Foo>::value, "");
  static_assert(is_sequence<std::vector<Foo>>::value, "");
}

BOOST_AUTO_TEST_SUITE_END()
