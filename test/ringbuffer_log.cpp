#include "../include/boost/test/unit_test.hpp"
#include "../include/ringbuffer_log.hpp"

BOOST_AUTO_TEST_SUITE(ringbuffer_log)

using namespace boost_lite::ringbuffer_log;

static simple_ringbuffer_log<> simple(level::all);

extern "C" void test_function(bool backtrace)
{
  if(backtrace)
    BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE(simple, level::warn, "test message", 3, 4);
  else
    BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION(simple, level::fatal, "test message", 1, 2);
}

BOOST_AUTO_TEST_CASE(ringbuffer_log / simple / works, "Tests that the simple_ringbuffer_log works as advertised")
{
  BOOST_CHECK(simple.empty());
  BOOST_CHECK(simple.size() == 0);

  test_function(false);
  BOOST_CHECK(!simple.empty());
  BOOST_CHECK(simple.size() == 1);

  {
    simple_ringbuffer_log<>::value_type &v = simple.front();
    BOOST_CHECK(v.counter == 0);
    BOOST_CHECK(v.level == static_cast<unsigned>(level::fatal));
    BOOST_CHECK(v.using_code64 == 0);
    BOOST_CHECK(v.using_backtrace == 0);
    BOOST_CHECK(!strcmp(v.message, "test message"));
    BOOST_CHECK(v.code32[0] == 1);
    BOOST_CHECK(v.code32[1] == 2);
    BOOST_CHECK(!strcmp(v.function, "test_function:15"));
  }

  test_function(true);
  BOOST_CHECK(!simple.empty());
  BOOST_CHECK(simple.size() == 2);

  {
    simple_ringbuffer_log<>::value_type &v = simple.front();
    BOOST_CHECK(v.counter == 1);
    BOOST_CHECK(v.level == static_cast<unsigned>(level::warn));
    BOOST_CHECK(v.using_code64 == 0);
    BOOST_CHECK(v.using_backtrace == 1);
    BOOST_CHECK(!strcmp(v.message, "test message"));
    BOOST_CHECK(v.code32[0] == 3);
    BOOST_CHECK(v.code32[1] == 4);
    char **symbols = backtrace_symbols((void **) v.backtrace, sizeof(v.backtrace) / sizeof(v.backtrace[0]));
    BOOST_REQUIRE(symbols);
    BOOST_CHECK(symbols[0]);
    for(size_t n = 0; n < sizeof(v.backtrace) / sizeof(v.backtrace[0]); n++)
    {
      if(symbols[n])
      {
        BOOST_TEST_MESSAGE(symbols[n]);
        std::cout << symbols[n] << std::endl;
      }
      else
      {
        BOOST_TEST_MESSAGE("null");
        std::cout << "null" << std::endl;
      }
    }
    free(symbols);
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
