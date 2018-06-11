/* Test the unsigned 128 bit integer
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
#include "../include/uint128.hpp"

#include <array>
#include <chrono>

BOOST_AUTO_TEST_SUITE(uint128)

struct uint128_test_item
{
  QUICKCPPLIB_NAMESPACE::integers128::uint128 a;
  uint32_t b;
  QUICKCPPLIB_NAMESPACE::integers128::uint128 added, subtracted, divided;
  uint32_t modulus;
};
static const uint128_test_item uint128set1[] = {
#include "uint128testdata1.h"
};
static const uint128_test_item uint128set2[] = {
#include "uint128testdata2.h"
};

BOOST_AUTO_TEST_CASE(uint128 / works, "Tests that uint128 works as advertised")
{
  for(const uint128_test_item &i : uint128set1)
  {
    BOOST_CHECK(i.a + i.b == i.added);
    BOOST_CHECK(i.a - i.b == i.subtracted);
    // BOOST_CHECK(i.a / i.b == i.divided);
    BOOST_CHECK(i.a % i.b == i.modulus);
  }
  for(const uint128_test_item &i : uint128set2)
  {
    BOOST_CHECK(i.a + i.b == i.added);
    BOOST_CHECK(i.a - i.b == i.subtracted);
    // BOOST_CHECK(i.a / i.b == i.divided);
    BOOST_CHECK(i.a % i.b == i.modulus);
  }
}

BOOST_AUTO_TEST_CASE(uint128 / performance, "Tests that the uint128 has reasonable performance")
{
  size_t count = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now(), end;
  do
  {
    for(const uint128_test_item &i : uint128set2)
    {
      volatile bool v = ((i.a % i.b) == i.modulus);
      (void) v;
    }
    end = std::chrono::steady_clock::now();
    count += sizeof(uint128set2) / sizeof(uint128set2[0]);
  } while(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < 5);
  auto no = ((uint64_t) count * std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000);  // NOLINT
  std::cout << "Can do " << no << " 128-bit moduli/sec which is " << (1000000000 / no) << "ns/modulus" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
