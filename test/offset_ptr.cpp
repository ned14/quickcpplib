/* Test the offset pointer
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
#include "../include/offset_ptr.hpp"

#include "timing.h"

BOOST_AUTO_TEST_SUITE(offset_ptr)

BOOST_AUTO_TEST_CASE(offset_ptr / works, "Tests that offset_ptr works as advertised")
{
  static std::pair<volatile void *, QUICKCPPLIB_NAMESPACE::offset_ptr::offset_ptr<volatile void>> testdata[1000000];

  for(auto &i : testdata)
  {
    i.first = (void *) (uintptr_t) rand();
    i.second = i.first;
  }
  for(auto &i : testdata)
  {
    BOOST_CHECK(i.first == i.second);
  }
}

BOOST_AUTO_TEST_CASE(offset_ptr / performance, "Tests that the offset_ptr has reasonable performance")
{
  static std::pair<volatile void *, QUICKCPPLIB_NAMESPACE::offset_ptr::offset_ptr<volatile void>> testdata[1000000];

  size_t count = 0;
  {
    auto begin = nanoclock();
    while(nanoclock() - begin < 1000000000ULL)
      ;
  }
  for(auto &i : testdata)
  {
    i.first = (void *) (uintptr_t) rand();
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
    uint64_t ticks = 0;
    for(auto &i : testdata)
    {
      volatile uint64_t begin = ticksclock();
      i.second = i.first;
      volatile uint64_t end = ticksclock();
      ticks += end - begin - overhead;
    }
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / count) << " CPU ticks latency per set" << std::endl;
  }
  {
    uint64_t ticks = 0;
    for(auto &i : testdata)
    {
      volatile uint64_t begin = ticksclock();
      (void) i.second;
      volatile uint64_t end = ticksclock();
      ticks += end - begin - overhead;
    }
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / count) << " CPU ticks latency per get" << std::endl;
  }
  {
    for(size_t n = 0; n < 64; n++)
    {
      testdata[n].second = testdata[n].first;
    }
    uint64_t ticks = 0;
    volatile uint64_t begin = ticksclock();
    for(size_t n = 0; n < 64; n++)
    {
      testdata[n].second = testdata[n].first;
    }
    volatile uint64_t end = ticksclock();
    ticks += end - begin - overhead;
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / 64) << " CPU ticks throughput per set" << std::endl;
  }
  {
    for(size_t n = 0; n < 64; n++)
    {
      (void) testdata[n].second;
    }
    uint64_t ticks = 0;
    volatile uint64_t begin = ticksclock();
    for(size_t n = 0; n < 64; n++)
    {
      (void) testdata[n].second;
    }
    volatile uint64_t end = ticksclock();
    ticks += end - begin - overhead;
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / 64) << " CPU ticks throughput per get" << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(atomic_offset_ptr / performance, "Tests that the atomic_offset_ptr has reasonable performance")
{
  static std::pair<void *, QUICKCPPLIB_NAMESPACE::offset_ptr::atomic_offset_ptr<void>> testdata[1000000];
  size_t count = 0;
  {
    auto begin = nanoclock();
    while(nanoclock() - begin < 1000000000ULL)
      ;
  }
  for(auto &i : testdata)
  {
    i.first = (void *) (uintptr_t) rand();
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
    uint64_t ticks = 0;
    for(auto &i : testdata)
    {
      volatile uint64_t begin = ticksclock();
      i.second.set(i.first);
      volatile uint64_t end = ticksclock();
      ticks += end - begin - overhead;
    }
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / count) << " CPU ticks latency per set" << std::endl;
  }
  {
    uint64_t ticks = 0;
    for(auto &i : testdata)
    {
      volatile uint64_t begin = ticksclock();
      (void) i.second.get();
      volatile uint64_t end = ticksclock();
      ticks += end - begin - overhead;
    }
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / count) << " CPU ticks latency per get" << std::endl;
  }
  {
    for(size_t n = 0; n < 64; n++)
    {
      testdata[n].second = testdata[n].first;
    }
    uint64_t ticks = 0;
    volatile uint64_t begin = ticksclock();
    for(size_t n = 0; n < 64; n++)
    {
      testdata[n].second.set(testdata[n].first);
    }
    volatile uint64_t end = ticksclock();
    ticks += end - begin - overhead;
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / 64) << " CPU ticks throughput per set" << std::endl;
  }
  {
    for(size_t n = 0; n < 64; n++)
    {
      (void) testdata[n].second;
    }
    uint64_t ticks = 0;
    volatile uint64_t begin = ticksclock();
    for(size_t n = 0; n < 64; n++)
    {
      (void) testdata[n].second.get();
    }
    volatile uint64_t end = ticksclock();
    ticks += end - begin - overhead;
    count = sizeof(testdata) / sizeof(testdata[0]);
    std::cout << "It takes " << (ticks / 64) << " CPU ticks throughput per get" << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()
