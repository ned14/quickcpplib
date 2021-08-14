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

#include "../include/quickcpplib/algorithm/small_prng.hpp"

#include "../include/quickcpplib/boost/test/unit_test.hpp"

#include <set>
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
    it = index.nearest_find(5);
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

  static constexpr size_t ITEMS_COUNT = 1000000;
  std::set<uint32_t> shouldbe;
  std::vector<foo_t> storage;
  storage.reserve(ITEMS_COUNT);
  index.clear();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      shouldbe.insert(v);
      storage.emplace_back(v);
      index.insert(&storage.back());
      assert(index.end() != index.find(v));
    }
  }
  index.triecheckvalidity();
  {
    QUICKCPPLIB_NAMESPACE::algorithm::small_prng::small_prng rand;
    for(size_t n = 0; n < ITEMS_COUNT; n++)
    {
      auto v = rand();
      auto it = index.find(v);
      BOOST_REQUIRE(it != index.end());
      BOOST_CHECK(it->trie_key == v);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
