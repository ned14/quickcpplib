/* Test the unsigned 128 bit integer
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
#include "../include/quickcpplib/utils/thread.hpp"

#include <chrono>

BOOST_AUTO_TEST_SUITE(async_signal_safe_thread_local)

BOOST_AUTO_TEST_CASE(async_signal_safe_thread_local / works,
                     "Tests that async_signal_safe_thread_local works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::utils::thread;
  struct Shared
  {
    size_t constructed{0}, called{0}, destructed{0};
  };
  struct Test
  {
    Shared *shared;
    Test(Shared *s)
        : shared(s)
    {
      shared->constructed++;
    }
    ~Test() { shared->destructed++; }
    void call() { shared->called++; }
  };
  struct MakeTest
  {
    Shared *shared;
    MakeTest(Shared *s)
        : shared(s)
    {
    }
    Test *operator()(void *addr) const { return new(addr) Test(shared); }
  };
  {
    Shared s;
    {
      async_signal_safe_thread_local<Test, MakeTest> v{MakeTest(&s)};
      v->call();
    }
    BOOST_CHECK(s.constructed == 1);
    BOOST_CHECK(s.called == 1);
    BOOST_CHECK(s.destructed == 1);
  }
}

BOOST_AUTO_TEST_CASE(async_signal_safe_thread_local / performance,
                     "Tests that the async_signal_safe_thread_local has reasonable performance")
{
#if 0
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
#endif
}

BOOST_AUTO_TEST_SUITE_END()
