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

#include "../include/algorithm/small_prng.hpp"
#include "../include/boost/test/unit_test.hpp"
#include "../include/packed_backtrace.hpp"

#include <array>
#include <chrono>

BOOST_AUTO_TEST_SUITE(packed_backtrace)

static const void *sample1[] = {(void *) 0x000007fefd4e10ac, (void *) 0x000007fef48bffc7, (void *) 0x000007fef48bff70, (void *) 0x000007fef48bfe23,   // NOLINT
                                (void *) 0x000007fef48d51d8, (void *) 0x000007fef4995249, (void *) 0x000007fef48aef28, (void *) 0x000007fef48aecc9,   // NOLINT
                                (void *) 0x000007fef071244c, (void *) 0x000007fef07111b5, (void *) 0x000007ff00150acf, (void *) 0x000007ff0015098c};  // NOLINT
static const void *sample2[] = {(void *) 0x000007feda9e6ac0, (void *) 0x000000013f15e177, (void *) 0x000000013f1613cf, (void *) 0x000000013f162184,   // NOLINT
                                (void *) 0x000000013f162027, (void *) 0x000000013f161eee, (void *) 0x000000013f1621a9, (void *) 0x00000000775f59cd,   // NOLINT
                                (void *) 0x000000007782a561, (void *) 0x0000003d06e34950, (void *) 0x0000000000400bcd, (void *) 0x0000000000400bf5,   // NOLINT
                                (void *) 0x0000003d06e1ffe0, (void *) 0x00000000004009f9};                                                            // NOLINT
// Failures found during fuzzed input testing
static const void *failure1[] = {(void *) 0x431BDE82D7B634DB, (void *) 0x9E77EA437D500000};  // NOLINT

template <class T, size_t N> inline size_t test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::packed_backtrace<T> test, const void *(&input)[N])
{
  // std::cout << "packed_backtrace stored " << test.size() << " items in the same storage as 5 items" << std::endl;
  BOOST_REQUIRE(test.size() <= N);
  auto it = test.begin();
  for(size_t n = 0; n < test.size(); n++, ++it)
  {
    BOOST_CHECK(input[n] == test[n]);
    if(input[n] != test[n])
    {
      std::cerr << "Failed sequence:\n  " << input[n - 1] << "\n  " << input[n] << " " << test[n] << std::endl;
    }
    BOOST_CHECK(input[n] == *it);
  }
  BOOST_CHECK(it == test.end());
  return test.size();
}

BOOST_AUTO_TEST_CASE(packed_backtrace / works, "Tests that the packed_backtrace works as advertised")
{
  char buffer[40];
  // Test fuzzed failures
  test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, failure1), failure1);

  size_t stored;
  stored = test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, sample1), sample1);
  std::cout << "Stored " << (stored * sizeof(void *)) << " bytes into 40 bytes, a " << (100.0 * ((stored * sizeof(void *)) - 40) / (stored * sizeof(void *))) << "% reduction." << std::endl;
  stored = test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, sample2), sample2);
  std::cout << "Stored " << (stored * sizeof(void *)) << " bytes into 40 bytes, a " << (100.0 * ((stored * sizeof(void *)) - 40) / (stored * sizeof(void *))) << "% reduction." << std::endl;
}

BOOST_AUTO_TEST_CASE(packed_backtrace / random, "Tests that the packed_backtrace works with random input")
{
  char buffer[40];
  const void *bt[5];
  QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand(0);
  auto begin = std::chrono::steady_clock::now();
  do
  {
    auto *_bt = (uint32_t *) bt;  // NOLINT
    for(size_t n = 0; n < sizeof(bt) / sizeof(uint32_t); n++)
    {
      _bt[n] = rand();  // NOLINT
    }
    test_sample(QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, bt), bt);
  } while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - begin).count() < 5);
}

BOOST_AUTO_TEST_CASE(packed_backtrace / benchmark, "Tests that the packed_backtrace has reasonable performance")
{
  char buffer[40];
  size_t count = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now(), end;
  do
  {
    for(size_t n = 0; n < 1024; n++)
    {
      auto bt = QUICKCPPLIB_NAMESPACE::packed_backtrace::make_packed_backtrace(buffer, sample2);
      for(size_t m = 0; m < bt.size(); m++)
      {
        BOOST_CHECK(sample2[m] == bt[m]);
      }
    }
    count += 1024;
    end = std::chrono::steady_clock::now();
  } while(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < 5);
  std::cout << "Can construct and read " << ((uint64_t) count * std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000) << " packed stacktraces/sec" << std::endl;  // NOLINT
}

BOOST_AUTO_TEST_SUITE_END()
