/* Test the bitwise_trie
(C) 2010 - 2021 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/quickcpplib/algorithm/bitwise_trie.hpp"

#include "../include/quickcpplib/algorithm/hash.hpp"
#include "../include/quickcpplib/algorithm/small_prng.hpp"

#include "../include/quickcpplib/boost/test/unit_test.hpp"

#include "../include/quickcpplib/memory_resource.hpp"

#include "timing.h"

#include <set>
#include <unordered_set>
#include <vector>

BOOST_AUTO_TEST_SUITE(bitwise_trie)

BOOST_AUTO_TEST_CASE(bitwise_trie / works, "Tests that bitwise_trie works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::bitwise_trie;
  // Test converted from nedtries
  struct foo_t
  {
    foo_t *trie_parent;
    foo_t *trie_child[2];
    foo_t *trie_sibling[2];
    uint32_t trie_key{0};

    foo_t() = default;

    foo_t(uint32_t key)
        : trie_key(key)
    {
    }
  };
  struct foo_tree_t
  {
    size_t trie_count;
    bool trie_nobbledir;
    foo_t *trie_children[8 * sizeof(size_t)];
  };

  bitwise_trie<foo_tree_t, foo_t> index;
  {
    foo_t a, b;

    a.trie_key = 2;
    index.insert(&a);
    b.trie_key = 6;
    index.insert(&b);
    auto it = index.find(6);
    BOOST_CHECK(it->trie_key == 6);
    it = index.find_equal_or_next_largest(5);
    BOOST_CHECK(it->trie_key == 6);
    index.erase(2);
    for(auto *i : index)
    {
      std::cout << i << ", " << i->trie_key << "\n";
    }
    std::cout << std::flush;
    auto it1 = it, it2 = it;
    BOOST_CHECK(--it1 == index.end());
    BOOST_CHECK(++it2 == index.end());
    index.triecheckvalidity();
  }

  static constexpr size_t ITEMS_COUNT = 500000;
  std::vector<foo_t> storage;
  storage.reserve(ITEMS_COUNT);
  index.clear();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      storage.emplace_back(v);
      index.insert(&storage.back());
      assert(index.end() != index.find(v));
    }
  }
  BOOST_CHECK(index.size() == ITEMS_COUNT);
  index.triecheckvalidity();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto it = index.find(v);
      BOOST_REQUIRE(it != index.end());
      BOOST_CHECK(it->trie_key == v);
#if 0
      auto count = index.count(it);
      if(count > 1)
      {
        std::cout << "Key " << v << " has count = " << count << std::endl;
      }
#endif
    }
  }
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      if(!(v & 1))
      {
        index.erase(v);
      }
    }
  }
  index.triecheckvalidity();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto it = index.find(v);
      if(!(v & 1))
      {
        BOOST_CHECK(it == index.end());
      }
      else
      {
        BOOST_CHECK(it != index.end());
      }
    }
  }
  index.clear();
  {
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      index.insert(&storage[n]);
    }
  }
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      if(v & 1)
      {
        index.erase(v);
      }
    }
  }
  index.triecheckvalidity();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto it = index.find(v);
      if(v & 1)
      {
        BOOST_CHECK(it == index.end());
      }
      else
      {
        BOOST_CHECK(it != index.end());
      }
    }
  }

  std::multiset<uint32_t> shouldbe;
  index.clear();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      shouldbe.emplace(v);
      index.insert(&storage[n]);
    }
    size_t in_order_count = 0, not_in_order_count = 0;
    for(auto it = index.begin(); it != index.end();)
    {
      auto i = it++;
      if(it == index.end())
      {
        break;
      }
      if(i->trie_key < it->trie_key)
      {
        in_order_count++;
      }
      else
      {
        not_in_order_count++;
      }
    }
    std::cout << "Trie index was in ascending order for " << in_order_count << " items and not for " << not_in_order_count << " items." << std::endl;
  }
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto shouldcount = shouldbe.count(v);
      if(shouldcount > 1)
      {
        BOOST_CHECK(index.count(v) == shouldcount);
        auto it = index.find(v);
        for(size_t x = 0; x < shouldcount - 1; x++)
        {
          auto i = it++;
          BOOST_CHECK(i->trie_key == it->trie_key);
          BOOST_CHECK(*it > *i);  // insertion order preserved
        }
      }
    }
  }
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto it1 = shouldbe.upper_bound(v);
      auto it2 = index.upper_bound(v);
      if(it1 == shouldbe.end())
      {
        BOOST_CHECK(it2 == index.end());
      }
      else if(it2 == index.end())
      {
        BOOST_CHECK(it1 == shouldbe.end());
      }
      else
      {
        BOOST_CHECK(*it1 == it2->trie_key);
        if(*it1 != it2->trie_key)
        {
          std::cout << "upper bound on " << v << " should be " << *it1 << " got instead " << it2->trie_key << std::endl;
          BOOST_CHECK(index.end() != index.find(*it1));
          it2 = index.upper_bound(*it1 - 1);
          BOOST_CHECK(it2->trie_key == *it1);
          it2 = index.upper_bound(v);
        }
      }
    }
  }
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(int rounds = 0; rounds < 20; rounds++)
    {
      uint64_t acc_diff = 0, acc_count = 0;
      for(size_t n = 0; n < ITEMS_COUNT; n++)
      {
        auto v = rand();
        auto it1 = shouldbe.upper_bound(v);
        auto it2 = index.find_equal_or_larger(v + 1, rounds);
        if(it1 == shouldbe.end())
        {
          BOOST_CHECK(it2 == index.end());
        }
        else if(it2 != index.end())
        {
          BOOST_CHECK(it2->trie_key >= *it1);
          acc_diff += it2->trie_key - *it1;
          acc_count++;
        }
      }
      std::cout << "\nFor rounds = " << rounds << " close fit was an average of " << (100.0 * acc_diff / acc_count / UINT32_MAX) << "% from ideal."
                << std::endl;
    }
  }
}

BOOST_AUTO_TEST_CASE(bitwise_trie / benchmark, "Benchmarks bitwise_trie against other algorithms")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm::bitwise_trie;
  namespace pmr = QUICKCPPLIB_NAMESPACE::pmr;
  static constexpr size_t ITEMS_BITSHIFT = 16;  // 26 uses 2Gb of RAM
  static constexpr size_t ITEMS_COUNT = 1 << ITEMS_BITSHIFT;
  static constexpr size_t bytes_per_item = 28;
  std::vector<uint32_t> randoms;
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    randoms.reserve(ITEMS_COUNT);
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      randoms.push_back(rand());
    }
  }
  {
    struct foo_t
    {
      foo_t *trie_parent;
      foo_t *trie_child[2];
      uint32_t trie_key{0};

      foo_t() = default;

      foo_t(uint32_t key)
          : trie_key(key)
      {
      }
    };
    struct foo_tree_t
    {
      size_t trie_count;
      bool trie_nobbledir;
      foo_t *trie_children[8 * sizeof(size_t)];
    };

    bitwise_trie<foo_tree_t, foo_t> index;
    std::vector<foo_t> storage;
    storage.reserve(ITEMS_COUNT);
    std::vector<std::pair<size_t, uint64_t>> clocks;
    clocks.reserve(ITEMS_BITSHIFT + 1);
    size_t n = 0;
    for(size_t x = 0, m = 1; x < ITEMS_BITSHIFT; x++, m <<= 1)
    {
      clocks.emplace_back(n, nanoclock());
      for(; n < m; n++)
      {
        storage.emplace_back(randoms[n]);
        index.insert(&storage.back());
      }
    }
    clocks.emplace_back(n, nanoclock());
    std::cout << "For bitwise_trie:";
    for(n = 1; n < clocks.size(); n++)
    {
      std::cout << "\n   " << clocks[n].first << ": " << ((double) (clocks[n].second - clocks[0].second) / clocks[n].first) << " ns per item insert";
    }
    std::cout << std::endl;
  }
  {
    std::vector<char> buffer;
    buffer.reserve(ITEMS_COUNT * bytes_per_item);
    std::cout << "\nAllocating " << (ITEMS_COUNT * bytes_per_item) << " bytes, " << bytes_per_item << " bytes per item seems acceptable." << std::endl;
    pmr::monotonic_buffer_resource mr(buffer.data(), buffer.capacity());
    std::set<uint32_t, std::less<uint32_t>, pmr::polymorphic_allocator<uint32_t>> cont{pmr::polymorphic_allocator<uint32_t>(&mr)};
    std::vector<std::pair<size_t, uint64_t>> clocks;
    clocks.reserve(ITEMS_BITSHIFT + 1);
    size_t n = 0;
    for(size_t x = 0, m = 1; x < ITEMS_BITSHIFT; x++, m <<= 1)
    {
      clocks.emplace_back(n, nanoclock());
      for(; n < m; n++)
      {
        cont.insert(randoms[n]);
      }
    }
    clocks.emplace_back(n, nanoclock());
    std::cout << "For set:";
    for(n = 1; n < clocks.size(); n++)
    {
      std::cout << "\n   " << clocks[n].first << ": " << ((double) (clocks[n].second - clocks[0].second) / clocks[n].first) << " ns per item insert";
    }
    std::cout << std::endl;
  }
  {
    std::vector<char> buffer;
    buffer.reserve(ITEMS_COUNT * bytes_per_item);
    std::cout << "\nAllocating " << (ITEMS_COUNT * bytes_per_item) << " bytes, " << bytes_per_item << " bytes per item seems acceptable." << std::endl;
    pmr::monotonic_buffer_resource mr(buffer.data(), buffer.capacity());
    std::unordered_set<uint32_t, QUICKCPPLIB_NAMESPACE::algorithm::hash::fnv1a_hash<uint32_t>, std::equal_to<uint32_t>, pmr::polymorphic_allocator<uint32_t>>
    cont{pmr::polymorphic_allocator<uint32_t>(&mr)};
    // cont.reserve(ITEMS_COUNT);
    std::vector<std::pair<size_t, uint64_t>> clocks;
    clocks.reserve(ITEMS_BITSHIFT + 1);
    size_t n = 0;
    for(size_t x = 0, m = 1; x < ITEMS_BITSHIFT; x++, m <<= 1)
    {
      clocks.emplace_back(n, nanoclock());
      for(; n < m; n++)
      {
        cont.insert(randoms[n]);
      }
    }
    clocks.emplace_back(n, nanoclock());
    std::cout << "For unordered_set:";
    for(n = 1; n < clocks.size(); n++)
    {
      std::cout << "\n   " << clocks[n].first << ": " << ((double) (clocks[n].second - clocks[0].second) / clocks[n].first) << " ns per item insert";
    }
    std::cout << std::endl;
  }
}

BOOST_AUTO_TEST_SUITE_END()
