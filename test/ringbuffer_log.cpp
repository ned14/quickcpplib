/* Test the ring buffer log
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (5 commits)


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
#include "../include/ringbuffer_log.hpp"

BOOST_AUTO_TEST_SUITE(ringbuffer_log)

using namespace QUICKCPPLIB_NAMESPACE::ringbuffer_log;

static simple_ringbuffer_log<> simple(level::all);

extern "C" void test_function(bool backtrace)
{
  if(backtrace)
  {
    QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE(simple, level::warn, "test message", 3, 4);
  }
  else
  {
    QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION(simple, level::fatal, "test message", 1, 2);
  }
}

BOOST_AUTO_TEST_CASE(ringbuffer_log / simple / works, "Tests that the simple_ringbuffer_log works as advertised")
{
  BOOST_CHECK(simple.empty());
  BOOST_CHECK(simple.size() == 0);  // NOLINT

  test_function(false);
  BOOST_CHECK(!simple.empty());
  BOOST_CHECK(simple.size() == 1);

  {
    simple_ringbuffer_log<>::value_type &v = simple.front();
    BOOST_CHECK(v.counter == 0ULL);
    BOOST_CHECK(v.level == static_cast<unsigned>(level::fatal));
    BOOST_CHECK(v.using_code64 == 0);
    BOOST_CHECK(v.using_backtrace == 0);
    BOOST_CHECK(!strcmp(v.message, "test message"));
    BOOST_CHECK(v.code32[0] == 1);
    BOOST_CHECK(v.code32[1] == 2);
    BOOST_CHECK(!strcmp(v.function, "test_function:41"));
  }

  test_function(true);
  BOOST_CHECK(!simple.empty());
  BOOST_CHECK(simple.size() == 2);

  {
    simple_ringbuffer_log<>::value_type &v = simple.front();
    BOOST_CHECK(v.counter == 1ULL);
    BOOST_CHECK(v.level == static_cast<unsigned>(level::warn));
    BOOST_CHECK(v.using_code64 == 0);
    BOOST_CHECK(v.using_backtrace == 1);
    BOOST_CHECK(!strcmp(v.message, "test message"));
    BOOST_CHECK(v.code32[0] == 3);
    BOOST_CHECK(v.code32[1] == 4);
    void *bt[16];
    QUICKCPPLIB_NAMESPACE::packed_backtrace::packed_backtrace<> btv(v.backtrace);
    size_t btlen = 0;
    for(auto it = btv.begin(); it != btv.end() && btlen < 16; btlen++, ++it)
    {
      bt[btlen] = *it;
    }
    char **symbols = backtrace_symbols(bt, btlen);  // NOLINT
    BOOST_REQUIRE(symbols != nullptr);              // NOLINT
    BOOST_CHECK(symbols[0] != nullptr);             // NOLINT
    for(size_t n = 0; n < btlen; n++)
    {
      if(symbols[n] != nullptr)  // NOLINT
      {
        BOOST_TEST_MESSAGE(symbols[n]);        // NOLINT
        std::cout << symbols[n] << std::endl;  // NOLINT
      }
      else
      {
        BOOST_TEST_MESSAGE("null");
        std::cout << "null" << std::endl;
      }
    }
    free(symbols);  // NOLINT
  }

  std::cout << "\nDump of log:\n" << simple << std::endl;
}

BOOST_AUTO_TEST_CASE(ringbuffer_log / simple / iterators, "Tests that the simple_ringbuffer_log iterator works as advertised")
{
  BOOST_CHECK(simple.size() == 2);
  simple_ringbuffer_log<>::const_iterator begin = simple.begin();
  simple_ringbuffer_log<>::const_iterator end = simple.end();
  BOOST_CHECK((end - begin) == 2);
  --end;
  BOOST_CHECK(simple.front() == *begin);
  BOOST_CHECK(simple.back() == *end);
  BOOST_CHECK(simple.front() == simple[0]);
  BOOST_CHECK(simple.back() == simple[1]);
}

BOOST_AUTO_TEST_CASE(ringbuffer_log / simple / performance, "Tests that the simple_ringbuffer_log is fast to log to")
{
  test_function(false);
  test_function(false);
  test_function(true);
  test_function(true);
  BOOST_CHECK(simple.size() == 6);
  auto diff_nobacktrace = simple[2].timestamp - simple[3].timestamp;
  auto diff_backtrace = simple[0].timestamp - simple[1].timestamp;
  std::cout << "Nanoseconds to log item without backtrace: " << diff_nobacktrace << std::endl;
  std::cout << "Nanoseconds to log item with    backtrace: " << diff_backtrace << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
