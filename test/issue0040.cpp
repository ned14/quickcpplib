/* Test algorithm::hash
(C) 2023 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/quickcpplib/boost/test/unit_test.hpp"
#include "../include/quickcpplib/span.hpp"

BOOST_AUTO_TEST_SUITE(span_)

BOOST_AUTO_TEST_CASE(span / span_constructor, "Tests that issue #40 is fixed")
{
#if QUICKCPPLIB_USE_STD_SPAN
  using namespace QUICKCPPLIB_NAMESPACE::span;
  struct buffer_type
  {
    buffer_type(span<char>) {}
  };
  struct const_buffer_type
  {
    const_buffer_type(span<const char>) {}
    // const_buffer_type(span<char>) {}
  };
  std::span<char> x;
  std::span<const char> y;
  buffer_type a[] = {{x}};
  const_buffer_type b[] = {{y}};
  const_buffer_type c[] = {{x}, {y}};
  (void) a;
  (void) b;
  (void) c;
#endif
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
