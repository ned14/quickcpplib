/* Test the packed backtrace
(C) 2017 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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
#include "../include/packed_backtrace.hpp"

#include <array>

BOOST_AUTO_TEST_SUITE(packed_backtrace)

static const void *sample1[] = {(void *) 0x000007fefd4e10ac, (void *) 0x000007fef48bffc7, (void *) 0x000007fef48bff70, (void *) 0x000007fef48bfe23, (void *) 0x000007fef48d51d8, (void *) 0x000007fef4995249,
                                (void *) 0x000007fef48aef28, (void *) 0x000007fef48aecc9, (void *) 0x000007fef071244c, (void *) 0x000007fef07111b5, (void *) 0x000007ff00150acf, (void *) 0x000007ff0015098c};
static const void *sample2[] = {(void *) 0x0000003d06e34950, (void *) 0x0000000000400bcd, (void *) 0x0000000000400bf5, (void *) 0x0000003d06e1ffe0, (void *) 0x00000000004009f9};

template <class T, size_t N> inline size_t test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::packed_backtrace<T> test, const void *(&input)[N])
{
  std::cout << "packed_backtrace stored " << test.size() << " items in the same storage as 5 items" << std::endl;
  BOOST_REQUIRE(test.size() <= N);
  auto it = test.begin();
  for(size_t n = 0; n < test.size(); n++, ++it)
  {
    BOOST_CHECK(input[n] == test[n]);
    BOOST_CHECK(input[n] == *it);
  }
  BOOST_CHECK(it == test.end());
  return test.size();
}

BOOST_AUTO_TEST_CASE(packed_backtrace / works, "Tests that the packed_backtrace works as advertised")
{
  size_t stored;
  char buffer[40];
  stored = test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, sample1), sample1);
  stored = test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, sample2), sample2);
}

BOOST_AUTO_TEST_SUITE_END()
