/* Test the bit interleave functions
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

#include "../include/quickcpplib/algorithm/bit_interleave.hpp"
#include "../include/quickcpplib/algorithm/small_prng.hpp"

#include <chrono>

BOOST_AUTO_TEST_SUITE(bit_interleave)

BOOST_AUTO_TEST_CASE(bit_interleave / works, "Tests that bit_interleave works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm;
  static constexpr size_t bytes = 4096;
  small_prng::small_prng rand;

  std::vector<char> buffer(bytes);
  for(size_t n = 0; n < bytes / sizeof(small_prng::small_prng::value_type); n++)
  {
    reinterpret_cast<small_prng::small_prng::value_type *>(buffer.data())[n] = rand();
  }
  auto dotest = [](const auto *items, size_t len)
  {
    using type = std::decay_t<decltype(items[0])>;
    using interleaved_type = typename bit_interleave::detail::next_larger<type>::type;
    for(size_t n = 0; n < len / 2; n++)
    {
      interleaved_type shouldbe_interleaved = 0;
      const auto &a = items[n];
      const auto &b = items[len / 2 + n];
      for(size_t idx = 0; idx < sizeof(type) * 8; idx++)
      {
        const auto bit = (interleaved_type(1) << idx);
        if(a & bit)
        {
          shouldbe_interleaved |= bit << idx;
        }
        if(b & bit)
        {
          shouldbe_interleaved |= bit << (idx + 1);
        }
      }
      interleaved_type v = bit_interleave::bit_interleave(a, b);
      BOOST_CHECK(shouldbe_interleaved == v);
      auto x = bit_interleave::bit_deinterleave(v);
      BOOST_CHECK(x.evens == a);
      BOOST_CHECK(x.odds == b);
    }
  };
  dotest((const uint8_t *) buffer.data(), buffer.size() / sizeof(uint8_t));
  dotest((const uint16_t *) buffer.data(), buffer.size() / sizeof(uint16_t));
  dotest((const uint32_t *) buffer.data(), buffer.size() / sizeof(uint32_t));
}

BOOST_AUTO_TEST_CASE(bit_interleave / performance, "Tests that bit_interleave has reasonable performance")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm;
  static constexpr size_t items = 16384;
  small_prng::small_prng rand;

  std::vector<uint32_t> input1(items), input2(items);
  std::vector<uint64_t> output(items);
  for(size_t n = 0; n < items; n++)
  {
    input1[n] = rand();
    input2[n] = rand();
  }
  size_t count = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now(), end;
  do
  {
    for(size_t n = 0; n < items; n++)
    {
      output[n] = bit_interleave::bit_interleave(input1[n], input2[n]);
    }
    end = std::chrono::steady_clock::now();
    count += items;
  } while(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < 5);
  auto no = ((uint64_t) count *
             1000000000.0 / std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count());  // NOLINT
  std::cout << "Can do " << no << " 32-bit to 64-bit interleaves/sec which is " << (1000000000.0 / no) << "ns/interleave"
            << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
