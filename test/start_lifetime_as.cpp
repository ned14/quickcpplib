/* Test the start lifetime function
(C) 2022 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/quickcpplib/start_lifetime_as.hpp"
#include "../include/quickcpplib/boost/test/unit_test.hpp"


BOOST_AUTO_TEST_SUITE(start_lifetime)

BOOST_AUTO_TEST_CASE(start_lifetime_as / works, "Tests that start_lifetime_as works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::start_lifetime_as;
  struct Foo
  {
    int a;
  };
  {
    char buffer[sizeof(Foo) * 4];
    Foo *x = start_lifetime_as<Foo>(buffer);
    Foo(*y)[2] = start_lifetime_as<Foo[2]>(buffer);
    x = start_lifetime_as_array<Foo>(buffer, 2);
    y = start_lifetime_as_array<Foo[2]>(buffer, 2);
    (void) x;
    (void) y;
  }
  {
    char buffer_[sizeof(Foo) * 4];
    const char *buffer = buffer_;
    static_assert(std::is_const<std::remove_pointer<decltype(start_lifetime_as<Foo>(buffer))>::type>::value, "");
    static_assert(std::is_const<std::remove_pointer<decltype(start_lifetime_as<Foo[2]>(buffer))>::type>::value, "");
    const Foo *x = start_lifetime_as<Foo>(buffer);
    const Foo(*y)[2] = start_lifetime_as<Foo[2]>(buffer);
    x = start_lifetime_as_array<Foo>(buffer, 2);
    y = start_lifetime_as_array<Foo[2]>(buffer, 2);
    (void) x;
    (void) y;
  }
}

BOOST_AUTO_TEST_SUITE_END()
