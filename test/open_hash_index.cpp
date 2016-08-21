#include "../include/boost/test/unit_test.hpp"
#include "../include/open_hash_index.hpp"

#include <array>

#ifdef _MSC_VER
#pragma warning(disable : 4503)  // decorated name length exceeded
#endif

BOOST_AUTO_TEST_SUITE(open_hash_index)

template <class OpenHashIndex> void do_test()
{
  OpenHashIndex cont;
  typename OpenHashIndex::iterator dit;
  BOOST_CHECK(cont.empty());
  BOOST_CHECK(cont.size() == 0);
  BOOST_CHECK(cont.begin() == cont.end());
  {
    auto it = cont.insert(std::make_pair(1, 78));
    BOOST_CHECK(it.first != cont.end());
    BOOST_CHECK(it.first->first == 1);
    BOOST_CHECK(it.first->second == 78);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 1);
  }
  {
    auto it = cont.insert(std::make_pair(2, 79));
    BOOST_CHECK(it.first != cont.end());
    BOOST_CHECK(it.first->first == 2);
    BOOST_CHECK(it.first->second == 79);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 2);
  }
  {
    auto it = cont.insert(std::make_pair(3, -78));
    BOOST_CHECK(it.first != cont.end());
    BOOST_CHECK(it.first->first == 3);
    BOOST_CHECK(it.first->second == -78);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 3);
  }
  {
    dit = cont.insert(std::make_pair(4, 0)).first;
    BOOST_CHECK(dit != cont.end());
    BOOST_CHECK(dit->first == 4);
    BOOST_CHECK(dit->second == 0);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 4);
  }
  {
    auto it = cont.insert(std::make_pair(5, 4));
    BOOST_CHECK(it.first != cont.end());
    BOOST_CHECK(it.first->first == 5);
    BOOST_CHECK(it.first->second == 4);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 5);
  }
  {
    typename OpenHashIndex::const_iterator it(std::move(dit));
    cont.erase(std::move(it));
    BOOST_CHECK(cont.size() == 4);
  }
  {
    auto it = cont.insert(std::make_pair(6, 65));
    BOOST_CHECK(it.first != cont.end());
    BOOST_CHECK(it.first->first == 6);
    BOOST_CHECK(it.first->second == 65);
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 5);
  }
  {
    auto it = cont.insert(std::make_pair(7, 4));
    BOOST_CHECK(it.first == cont.end());
    BOOST_CHECK(!cont.empty());
    BOOST_CHECK(cont.size() == 5);
  }
  {
    auto it = cont.find(0);
    BOOST_CHECK(it == cont.end());
    it = cont.find(2);
    BOOST_CHECK(it != cont.end());
    BOOST_CHECK(it->first == 2);
    BOOST_CHECK(it->second == 79);
  }
  {
    auto it = cont.find(7);
    BOOST_CHECK(it == cont.end());
    it = cont.find(6);
    BOOST_CHECK(it != cont.end());
    BOOST_CHECK(it->first == 6);
    BOOST_CHECK(it->second == 65);
    BOOST_CHECK(cont.size() == 5);
  }
  {
    typename OpenHashIndex::const_iterator begin = cont.begin();
    typename OpenHashIndex::const_iterator end = cont.end();
    BOOST_CHECK(end > begin);
    BOOST_CHECK(begin < end);
    --end;
    BOOST_CHECK(end > begin);
    BOOST_CHECK(begin < end);
    BOOST_CHECK(cont.front() == *begin);
    BOOST_CHECK(cont.back() == *end);
  }
  {
    cont.erase(1);
    BOOST_CHECK(cont.size() == 4);
  }
  {
    cont.erase(1);
    BOOST_CHECK(cont.size() == 4);
  }
  {
    cont.erase(3);
    BOOST_CHECK(cont.size() == 3);
  }
  {
    cont.erase(6);
    BOOST_CHECK(cont.size() == 2);
  }
  {
    cont.erase(2);
    BOOST_CHECK(cont.size() == 1);
  }
  {
    cont.erase(5);
    BOOST_CHECK(cont.empty());
    BOOST_CHECK(cont.size() == 0);
  }
}

template <class T> using array5 = std::array<T, 5>;

BOOST_AUTO_TEST_CASE(open_hash_index / linear_memory_policy / works, "Tests that the open_hash_index<linear_memory_policy> works as advertised")
{
  using namespace boost_lite::open_hash_index;
  do_test<basic_open_hash_index<linear_memory_policy<size_t, int>, array5>>();
}

BOOST_AUTO_TEST_CASE(open_hash_index / atomic_linear_memory_policy / works, "Tests that the open_hash_index<atomic_linear_memory_policy> works as advertised")
{
  using namespace boost_lite::open_hash_index;
  do_test<basic_open_hash_index<atomic_linear_memory_policy<size_t, int>, array5, true>>();
}

#if 0
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
#endif

BOOST_AUTO_TEST_SUITE_END()
