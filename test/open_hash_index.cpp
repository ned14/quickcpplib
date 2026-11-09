/* Test the open hash index
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

// These tests are built Release, whose flags include -DNDEBUG, so every assert()
// in this file used to compile to nothing: the assertions below could never fail
// and were never running. Undefining NDEBUG here (before the first include, so it
// also covers any <cassert> pulled in by a header) keeps them live.
#undef NDEBUG

#include "../include/quickcpplib/algorithm/open_hash_index.hpp"
#include "../include/quickcpplib/boost/test/unit_test.hpp"

#include <array>
#include <random>
#include <unordered_map>

#ifdef _MSC_VER
#pragma warning(disable : 4503)  // decorated name length exceeded
#endif

BOOST_AUTO_TEST_SUITE(open_hash_index)

template <class OpenHashIndex> void do_test()
{
  OpenHashIndex cont;
  typename OpenHashIndex::iterator dit;
  BOOST_CHECK(cont.empty());
  BOOST_CHECK(cont.size() == 0);  // NOLINT
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
    cont.erase(std::move(dit));
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
    BOOST_CHECK(cont.size() == 0);  // NOLINT
  }
}

template <class T> using array5 = std::array<T, 5>;
template <class T> using array8192 = std::array<T, 8192>;

void test_traits()
{
  using namespace QUICKCPPLIB_NAMESPACE::configurable_spinlock;
  static_assert(!QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index::detail::is_shared_mutex<spinlock<bool>>::value, "");
  static_assert(QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index::detail::is_shared_mutex<shared_spinlock<bool>>::value, "");
}

BOOST_AUTO_TEST_CASE(open_hash_index / linear_memory_policy / works, "Tests that the open_hash_index<linear_memory_policy> works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index;
  do_test<basic_open_hash_index<linear_memory_policy<size_t, int, 5>, array5>>();
}

BOOST_AUTO_TEST_CASE(open_hash_index / atomic_linear_memory_policy / works / single, "Tests that the open_hash_index<atomic_linear_memory_policy> works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index;
  do_test<basic_open_hash_index<atomic_linear_memory_policy<size_t, int, 5>, array5, true>>();
}

static constexpr size_t ITEMS = 0x4000000;
alignas(4096) static std::array<unsigned, ITEMS> input;

// The keys are `input[n] >> 1` and the input is masked to 13 bits, so every key
// is < 4096 and the whole key space can be walked afterwards.
static constexpr size_t KEYSPACE = 4096;

template <class OpenHashIndex> void do_threaded_test(const char *desc)
{
  OpenHashIndex cont;
  // Thread 0 below is the only mutator, so it can keep an authoritative record of
  // which keys are present. Before this existed the two "works as advertised"
  // cases ran five seconds of concurrent traffic and printed a throughput
  // figure: a lost update, a wrong find() or a corrupt erase() was
  // indistinguishable from a pass, because nothing was ever asserted (0 checks,
  // 0 failures, "passed").
  std::vector<std::atomic<unsigned char>> present(KEYSPACE);
  for(auto &i : present)
  {
    i.store(0, std::memory_order_relaxed);
  }
  std::cout << "\nTesting " << desc << " under concurrency ..." << std::endl;
  for(size_t n = 0; n < 4096; n++)
  {
    cont.insert(std::make_pair(input[n] >> 1, input[n]));
    present[input[n] >> 1].store(1, std::memory_order_relaxed);
  }

  std::vector<std::thread> threads;
  std::atomic<int> done(-int(std::thread::hardware_concurrency() + 1)), updates(0), finds(0);
  for(size_t i = 0; i < std::thread::hardware_concurrency(); i++)
  {
    threads.push_back(std::thread([&, i] {
      ++done;
      while(done)
      {
        std::this_thread::yield();
      }
      while(!done)
      {
        for(size_t n = 0; n < ITEMS; n++)
        {
          if(!i)
          {
            const auto key = input[n] >> 1;
            if(!(input[n] & 3))
            {
              cont.erase(key);
              present[key].store(0, std::memory_order_relaxed);
            }
            else
            {
              cont.insert(std::make_pair(key, input[n]));
              present[key].store(1, std::memory_order_relaxed);
            }
            ++updates;
          }
          else
          {
            if(cont.find(input[n] >> 1))
            {
              ++finds;
            }
          }
        }
      }
      ++done;
    }));
  }
  while(done < -1)
  {
    std::this_thread::yield();
  }
  ++done;
  auto begin = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  ++done;
  while(done < int(std::thread::hardware_concurrency() + 1))
  {
    std::this_thread::yield();
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0;
  for(auto &i : threads)
  {
    i.join();
  }
  std::cout << "   I saw " << (unsigned long) (updates / diff) << " updates/sec and " << (unsigned long) (finds / diff) << " finds/sec" << std::endl;  // NOLINT

  // Verify the container against what the single writer recorded, once the
  // threads are quiescent. The check is done here rather than inside a reader on
  // purpose: find() hands out a pointer into live storage, so a reader can only
  // look at an element while another thread may be rewriting it, which would
  // assert about a race rather than about the container. Per key, so this catches
  // a lost update, a phantom entry, a wrong find() result and a corrupt erase(),
  // not just a wrong count.
  size_t live = 0, mismatches = 0;
  for(size_t k = 0; k < KEYSPACE; k++)
  {
    const bool expected = present[k].load(std::memory_order_relaxed) != 0;
    auto p = cont.find(k);
    const bool found = static_cast<bool>(p);
    if(expected)
    {
      ++live;
    }
    if(found != expected)
    {
      ++mismatches;
    }
    else if(found && (p->first != k || (p->second >> 1) != k))
    {
      ++mismatches;
    }
  }
  if(mismatches != 0)
  {
    std::cout << "   " << mismatches << " of " << KEYSPACE << " keys disagree with the writer's record" << std::endl;
  }
  BOOST_CHECK_EQUAL(mismatches, 0u);
  BOOST_CHECK_EQUAL(cont.size(), live);
}

BOOST_AUTO_TEST_CASE(open_hash_index / atomic_linear_memory_policy / works / concurrent / exclusive, "Tests that the open_hash_index<atomic_linear_memory_policy> works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index;
  std::mt19937 randomness;
  std::cout << "\nPreparing randomness ..." << std::endl;
  for(auto &i : input)
  {
    i = randomness() & 8191;
  }
  do_threaded_test<basic_open_hash_index<atomic_linear_memory_policy<unsigned, unsigned>, array8192>>("basic_open_hash_index<atomic_linear_memory_policy, spinlock>");
}

BOOST_AUTO_TEST_CASE(open_hash_index / atomic_linear_memory_policy / works / concurrent / shared, "Tests that the open_hash_index<atomic_linear_memory_policy> works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index;
  do_threaded_test<basic_open_hash_index<atomic_linear_memory_policy<unsigned, unsigned>, array8192>>("basic_open_hash_index<atomic_linear_memory_policy, shared_spinlock>");
}


template <class MapType> void do_insert_erase_performance(MapType &cont, const char *desc)
{
  std::cout << "\nTesting map " << desc << " for single threaded insert/erase ..." << std::endl;
  // Fill a bit
  for(size_t n = 0; n < 4096; n++)
  {
    cont.insert(std::make_pair(input[n] >> 1, input[n]));
  }
  // Randomly add and remove values
  auto begin = std::chrono::high_resolution_clock::now();
  size_t i = 0;
  do
  {
    for(size_t n = 0; n < ITEMS; n++)
    {
      if(input[n] & 1)
      {
        cont.insert(std::make_pair(input[n] >> 1, input[n]));
      }
      else
      {
        cont.erase(input[n] >> 1);
      }
    }
    ++i;
  } while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - begin).count() < 3);
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "   Map " << desc << " performed " << (unsigned long long) (i * ITEMS * 1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) << "/sec" << std::endl;  // NOLINT
}

template <class MapType> void do_find_performance(const MapType &cont, const char *desc)
{
  std::cout << "\nTesting map " << desc << " for single threaded lookup ..." << std::endl;
  auto begin = std::chrono::high_resolution_clock::now();
  size_t i = 0, found = 0;
  do
  {
    for(size_t n = 0; n < ITEMS; n++)
    {
      if(cont.end() != cont.find(input[n] >> 1))
      {
        ++found;
      }
    }
    ++i;
  } while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - begin).count() < 3);
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "   Map " << desc << " performed " << (unsigned long long) (i * ITEMS * 1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) << "/sec. Items were found " << (100.0 * found / (i * ITEMS)) << "% of the time." << std::endl;  // NOLINT
}

BOOST_AUTO_TEST_CASE(open_hash_index / linear_memory_policy / performance, "Tests that the open_hash_index<linear_memory_policy> is fast")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::open_hash_index;
  {
    std::unordered_map<unsigned, unsigned> cont;
    do_insert_erase_performance(cont, "unordered_map");
    do_find_performance(cont, "unordered_map");
  }
  {
    alignas(4096) basic_open_hash_index<linear_memory_policy<unsigned, unsigned, 1, arithmetic_modulus<unsigned>>, array8192> cont;
    do_insert_erase_performance(cont, "basic_open_hash_index<linear_memory_policy, arithmetic_modulus>");
    do_find_performance(cont, "basic_open_hash_index<linear_memory_policy, arithmetic_modulus>");
  }
  {
    alignas(4096) basic_open_hash_index<linear_memory_policy<unsigned, unsigned, 1, twos_power_modulus<unsigned>>, array8192> cont;
    do_insert_erase_performance(cont, "basic_open_hash_index<linear_memory_policy, twos_power_modulus>");
    do_find_performance(cont, "basic_open_hash_index<linear_memory_policy, twos_power_modulus>");
  }
  {
    alignas(4096) basic_open_hash_index<atomic_linear_memory_policy<unsigned, unsigned>, array8192> cont;
    do_insert_erase_performance(cont, "basic_open_hash_index<atomic_linear_memory_policy, spinlock>");
    do_find_performance(cont, "basic_open_hash_index<atomic_linear_memory_policy, spinlock>");
  }
  {
    alignas(4096) basic_open_hash_index<atomic_linear_memory_policy<unsigned, unsigned, 1, QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<unsigned>>, array8192> cont;
    do_insert_erase_performance(cont, "basic_open_hash_index<atomic_linear_memory_policy, shared_spinlock>");
    do_find_performance(cont, "basic_open_hash_index<atomic_linear_memory_policy, shared_spinlock>");
  }
}

BOOST_AUTO_TEST_SUITE_END()
