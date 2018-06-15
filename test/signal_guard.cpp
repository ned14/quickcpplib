/* Test the signal guard
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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
#include "../include/signal_guard.hpp"

#include "timing.h"

BOOST_AUTO_TEST_SUITE(signal_guard)

BOOST_AUTO_TEST_CASE(signal_guard / works, "Tests that signal_guard works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  signal_guard_install i(signalc::segmentation_fault | signalc::out_of_memory | signalc::termination);
  {
    int ret = signal_guard(signalc::segmentation_fault,
                           []() -> int {
                             volatile int *a = nullptr;
                             return *a;
                           },
                           [](signalc, const void *, const void *) -> int { return 78; });
    BOOST_CHECK(ret == 78);
    BOOST_CHECK(detail::current_signal_handler() == nullptr);
  }
  {
    int ret = signal_guard(signalc::termination, []() -> int { std::terminate(); }, [](signalc, const void *, const void *) -> int { return 78; });
    BOOST_CHECK(ret == 78);
    BOOST_CHECK(detail::current_signal_handler() == nullptr);
  }
}

BOOST_AUTO_TEST_CASE(signal_guard / performance, "Tests that the signal_guard has reasonable performance")
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  {
    auto begin = nanoclock();
    while(nanoclock() - begin < 1000000000ULL)
      ;
  }
  uint64_t overhead = 99999999;
  {
    for(size_t n = 0; n < 1000; n++)
    {
      volatile uint64_t begin = ticksclock();
      volatile uint64_t end = ticksclock();
      if(end - begin < overhead)
        overhead = end - begin;
    }
  }
  {
    signal_guard_install i(signalc::segmentation_fault);
    uint64_t ticks = 0;
    for(size_t n = 0; n < 128; n++)
    {
      uint64_t begin = ticksclock();
      volatile int ret = signal_guard(signalc::segmentation_fault, []() -> int { return 5; }, [](signalc, const void *, const void *) -> int { return 78; });
      uint64_t end = ticksclock();
      (void) ret;
      // std::cout << (end - begin - overhead) << std::endl;
      ticks += end - begin - overhead;
    }
    std::cout << "It takes " << (ticks / 128) << " CPU ticks to execute successful code (overhead was " << overhead << ")" << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()
