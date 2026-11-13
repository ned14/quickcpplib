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

#include "../include/quickcpplib/boost/test/unit_test.hpp"
#include "../include/quickcpplib/ringbuffer_log.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

BOOST_AUTO_TEST_SUITE(ringbuffer_log)

using namespace QUICKCPPLIB_NAMESPACE::ringbuffer_log;

static simple_ringbuffer_log<> simple(level::all);
// The line test_function logs its non-backtrace record from, so the case below can
// check the message's source location without hardcoding a line number which any
// edit above it would falsify.
static int test_function_logged_at = 0;

extern "C" void test_function(bool backtrace)
{
  if(backtrace)
  {
    QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE(simple, level::warn, "test message", 3, 4);
  }
  else
  {
    test_function_logged_at = __LINE__ + 1;
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
    char want_source[64];
    std::snprintf(want_source, sizeof(want_source), "test_function:%d", test_function_logged_at);
    BOOST_CHECK(!strcmp(v.function, want_source));
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

BOOST_AUTO_TEST_CASE(ringbuffer_log / snapshot / hands back whole records,
                     "Tests that snapshot() and snapshot_by_id() copy a whole record out of the ring")
{
  simple_ringbuffer_log<> log_(level::all);
  const auto a = log_.emplace_back(level::warn, "rec A", 0, 0);
  const auto b = log_.emplace_back(level::warn, "rec B", 0, 0);
  const auto c = log_.emplace_back(level::warn, "rec C", 0, 0);
  BOOST_CHECK(a == 0 && b == 1 && c == 2);
  // Positional access counts from the newest
  BOOST_CHECK(!strcmp(log_.snapshot(0).message, "rec C"));
  BOOST_CHECK(!strcmp(log_.snapshot(1).message, "rec B"));
  BOOST_CHECK(!strcmp(log_.snapshot(2).message, "rec A"));
  BOOST_CHECK(log_.snapshot(3).counter == 0 && log_.snapshot(3).message[0] == 0);  // not held
  simple_ringbuffer_log<>::value_type out;
  BOOST_CHECK(log_.try_snapshot(1, out) && !strcmp(out.message, "rec B"));
  BOOST_CHECK(!log_.try_snapshot(3, out));
  // By unique id: note the `operator[](size_type)` overload wins over
  // `operator[](unique_id)` for a plain integer, which is why this has its own name
  BOOST_CHECK(!strcmp(log_.snapshot_by_id(simple_ringbuffer_log<>::unique_id(a)).message, "rec A"));
  BOOST_CHECK(!strcmp(log_.snapshot_by_id(simple_ringbuffer_log<>::unique_id(c)).message, "rec C"));
  // Push until the ring wraps: the first record falls out of it
  for(unsigned i = 0; i < 4096; i++)
    log_.emplace_back(level::warn, "rec X", 0, 0);
  BOOST_CHECK(log_.snapshot_by_id(simple_ringbuffer_log<>::unique_id(a)).counter == 0);
  BOOST_CHECK(log_.snapshot_by_id(simple_ringbuffer_log<>::unique_id(a)).message[0] == 0);
  BOOST_CHECK(log_.snapshot(0).counter != 0);
}

// A ring small enough that concurrent writers wrap it while readers run, but far
// larger than the number of writers (the precondition the class documents).
struct small_policy
{
  using value_type = simple_ringbuffer_log_policy_detail::value_type;
  static constexpr size_t max_items = 64;
  using container_type = std::array<value_type, max_items>;
};

/* Concurrency: the writer path must not lock, and a reader must never be handed a
different record than the one it asked for. The second half is a regression test -
before the id check in _try_snapshot_counter, a slot recycled by a newer record was
returned as if it were the older id, which a lock could not prevent: the ring
overwrites by design.
*/
BOOST_AUTO_TEST_CASE(ringbuffer_log / concurrent / no torn or wrong records, "Tests that concurrent writers and readers agree")
{
  static constexpr unsigned threads = 4, per_thread = 2000;
  ringbuffer_log<small_policy> log(level::all);
  std::atomic<bool> stop{false};
  std::atomic<unsigned> bad{0};
  std::vector<std::vector<size_t>> ids(threads);
  std::vector<std::thread> readers;
  for(unsigned r = 0; r < 2; r++)
  {
    readers.emplace_back([&] {
      size_t next = 0;
      while(!stop.load(std::memory_order_relaxed))
      {
        // Every id the log claims to hold must resolve to a record carrying that id.
        if(log.size() > 2 && next++ % 8 == 0)
        {
          const auto v = log.snapshot(next % log.size());
          if(v.message[0] != 0 && v.counter == (size_t) -1)
          {
            bad++;
          }
        }
        const auto v = log.snapshot(0);
        if(v.message[0] != 0)
        {
          char want[64];
          std::snprintf(want, sizeof(want), "t%u:%u", v.code32[0], v.code32[1]);
          if(strcmp(v.message, want) != 0)
          {
            bad++;  // a copy which is not one write: message and codes disagree
          }
        }
      }
    });
  }
  std::vector<std::thread> writers;
  for(unsigned t = 0; t < threads; t++)
  {
    writers.emplace_back([&, t] {
      char msg[64];
      for(unsigned n = 0; n < per_thread; n++)
      {
        std::snprintf(msg, sizeof(msg), "t%u:%u", t, n);
        ids[t].push_back(log.emplace_back(level::info, msg, t, n, __func__, __LINE__));
      }
    });
  }
  for(auto &w : writers)
  {
    w.join();
  }
  stop = true;
  for(auto &r : readers)
  {
    r.join();
  }
  BOOST_CHECK(bad == 0);
  std::vector<size_t> all;
  for(auto &v : ids)
  {
    all.insert(all.end(), v.begin(), v.end());
  }
  std::sort(all.begin(), all.end());
  BOOST_CHECK(all.size() == size_t(threads) * per_thread);
  BOOST_CHECK(std::unique(all.begin(), all.end()) == all.end());  // no id issued twice
  // Every record still in the ring resolves to the id asked for, and to its own contents
  for(auto it = all.end() - 32; it != all.end(); ++it)
  {
    const auto v = log.snapshot_by_id(ringbuffer_log<small_policy>::unique_id(*it));
    BOOST_CHECK(v.counter == *it);
    BOOST_CHECK(v.message[0] != 0);
    char want[64];
    std::snprintf(want, sizeof(want), "t%u:%u", v.code32[0], v.code32[1]);
    BOOST_CHECK(!strcmp(v.message, want));
  }
}


BOOST_AUTO_TEST_SUITE_END()
