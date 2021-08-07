/* Bitwise trie algorithm
(C) 2010-2021 Niall Douglas <http://www.nedproductions.biz/> (7 commits)
File Created: Jun 2016


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

/* This is basically a C++-ification of https://github.com/ned14/nedtries,
and some of the implementation code is lifted from that library.
*/

#ifndef QUICKCPPLIB_ALGORITHM_BITWISE_TRIE_HPP
#define QUICKCPPLIB_ALGORITHM_BITWISE_TRIE_HPP

#define QUICKCPPLIB_ALGORITHM_BITWISE_TRIE_DEBUG 1

#include "../config.hpp"

#include <iterator>
#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace algorithm
{
  namespace bitwise_trie
  {
    namespace detail
    {
      inline unsigned bitscanr(size_t value)
      {
        if(!value)
          return 0;
#if defined(_MSC_VER) && !defined(__cplusplus_cli)
        {
          unsigned long bitpos;
#if defined(_M_IA64) || defined(_M_X64) || defined(WIN64)
          assert(8 == sizeof(size_t));
          _BitScanReverse64(&bitpos, value);
#else
          assert(4 == sizeof(size_t));
          _BitScanReverse(&bitpos, value);
#endif
          return (unsigned) bitpos;
        }
#elif defined(__GNUC__)
        return sizeof(value) * CHAR_BIT - 1 - (unsigned) __builtin_clzl(value);
#else
          /* The following code is illegal C, but it almost certainly will work.
          If not use the legal implementation below */
#if !defined(__cplusplus_cli)
        union
        {
          unsigned asInt[2];
          double asDouble;
        };
        int n;

        asDouble = (double) value + 0.5;
        n = (asInt[0 /*Use 1 if your CPU is big endian!*/] >> 20) - 1023;
#ifdef _MSC_VER
#pragma message(__FILE__ ": WARNING: Make sure you change the line above me if your CPU is big endian!")
#else
#warning Make sure you change the line above me if your CPU is big endian!
#endif
        return (unsigned) n;
#else
#if CHAR_BIT != 8
#error CHAR_BIT is not eight, and therefore this generic bitscan routine will need adjusting!
#endif
        /* This is a generic 32 and 64 bit compatible branch free bitscan right */
        size_t x = value;
        x = x | (x >> 1);
        x = x | (x >> 2);
        x = x | (x >> 4);
        x = x | (x >> 8);
        if(16 < sizeof(x) * CHAR_BIT)
          x = x | (x >> 16);
        if(32 < sizeof(x) * CHAR_BIT)
          x = x | (x >> 32);
        x = ~x;
        x = x - ((x >> 1) & (SIZE_MAX / 3));
        x = (x & (SIZE_MAX / 15 * 3)) + ((x >> 2) & (SIZE_MAX / 15 * 3));
        x = ((x + (x >> 4)) & (SIZE_MAX / UCHAR_MAX * 15)) * (SIZE_MAX / UCHAR_MAX);
        x = (CHAR_BIT * sizeof(x) - 1) - (x >> (CHAR_BIT * (sizeof(x) - 1)));
        return (unsigned) x;
#endif
#endif
      }
      template <int Dir> struct nobble_function_implementation
      {
        template <class T> bool operator()(T &&accessors) const noexcept { return accessors.flip_nobbledir(); }
      };
      template <> struct nobble_function_implementation<-1>
      {
        template <class T> bool operator()(T && /*unused*/) const noexcept { return false; }
      };
      template <> struct nobble_function_implementation<1>
      {
        template <class T> bool operator()(T && /*unused*/) const noexcept { return true; }
      };
    }  // namespace detail

    template <class ItemBaseType> class bitwise_trie_item_accessors;
    template <class Base, class T> class bitwise_trie;

    /*! \struct bitwise_trie_item_accessors
    \brief Default accessor for a bitwise trie item.

    This default accessor requires the following member variables in the trie item type:

    - `ItemFinalType *trie_parent`
    - `ItemFinalType *trie_child[2]`
    - `ItemFinalType *trie_sibling[2]`
    - `KeyType trie_key`
     */
    template <class ItemType> class bitwise_trie_item_accessors
    {
      ItemType *_v;

    public:
      constexpr bitwise_trie_item_accessors(ItemType *v)
          : _v(v)
      {
      }
      constexpr explicit operator bool() const noexcept { return _v != nullptr; }

      constexpr const ItemType *parent() const noexcept
      {
        assert(!parent_is_index());
        return _v->trie_parent;
      }
      constexpr ItemType *parent() noexcept
      {
        assert(!parent_is_index());
        return _v->trie_parent;
      }
      constexpr void set_parent(ItemType *x) noexcept { _v->trie_parent = x; }

      constexpr bool parent_is_index() const noexcept { return ((uintptr_t) _v->trie_parent & 3) == 3; }
      constexpr unsigned bit_index() const noexcept
      {
        assert(parent_is_index());
        return ((unsigned) _v->trie_parent) >> 2;
      }
      constexpr void set_parent_is_index(unsigned bit_index) noexcept { _v->trie_parent = (ItemType *) (((uintptr_t) bit_index << 2) | 3); }

      constexpr const ItemType *child(bool right) const noexcept { return _v->trie_child[right]; }
      constexpr ItemType *child(bool right) noexcept { return _v->trie_child[right]; }
      constexpr void set_child(bool right, ItemType *x) noexcept { _v->trie_child[right] = x; }

      constexpr const ItemType *sibling(bool right) const noexcept { return _v->trie_sibling[right]; }
      constexpr ItemType *sibling(bool right) noexcept { return _v->trie_sibling[right]; }
      constexpr void set_sibling(bool right, ItemType *x) noexcept { v.trie_sibling[right] = x; }

      constexpr auto key() const noexcept { return _v->trie_key; }

      constexpr bool is_primary_sibling() const noexcept { return _v->trie_parent != nullptr; }  // there is exactly one of these ever per key value
      constexpr void set_is_primary_sibling() noexcept { assert(_v->trie_parent != nullptr); }

      constexpr bool is_secondary_sibling() const noexcept { return _v->trie_parent == nullptr; }  // i.e. has same key as primary sibling
      constexpr void set_is_secondary_sibling() noexcept { _v->trie_parent = nullptr; }
    };

    /*! \struct bitwise_trie_head_accessors
    \brief Default accessor for a bitwise trie index head.

    This default accessor requires the following member variables in the trie index head type:

    - `<unsigned type> trie_count`
    - `ItemType *trie_children[8 * sizeof(<unsigned type>)]`
    - `bool trie_nobbledir` (if you use equal nobbling only)
     */
    template <class HeadBaseType, class ItemType> class bitwise_trie_head_accessors
    {
      HeadBaseType *_v;
      using _index_type = decltype(_v->trie_count);
      static_assert(std::is_unsigned<_index_type>::value, "count type must be unsigned");
      static constexpr size_t _index_type_bits = 8 * sizeof(_index_type);
      using _child_array_type = decltype(_v->trie_children);
      static_assert(sizeof(_child_array_type) / sizeof(void *) >= _index_type_bits, "children array is not big enough");

    public:
      constexpr bitwise_trie_head_accessors(HeadBaseType *v)
          : _v(v)
      {
      }
      constexpr explicit operator bool() const noexcept { return _v != nullptr; }

      constexpr _index_type size() const noexcept { return _v->trie_count; }
      constexpr void incr_size() noexcept { ++_v->trie_count; }
      constexpr void decr_size() noexcept { --_v->trie_count; }
      constexpr void set_size(_index_type x) noexcept { _v->trie_count = x; }

      constexpr const ItemType *child(_index_type idx) const noexcept { return _v->trie_children[idx]; }
      constexpr ItemType *child(_index_type idx) noexcept { return _v->trie_children[idx]; }
      constexpr void set_child(_index_type idx, ItemType *x) noexcept { _v->trie_children[idx] = x; }

      constexpr bool flip_nobbledir() noexcept { return (_v->nobbledir = !_v->trie_nobbledir); }
    };

    /*! \struct bitwise_trie
    \brief Bitwise Fredkin trie index head type.

    This uses the bitwise Fredkin trie algorithm to index a collection of items by an unsigned
    integral key, providing identical and constant time insertion, removal, and finds (i.e.
    insert, remove and find all take identical time, and that is constant amount almost
    independent of items in the index). It also provides a bounded time closest fit find,
    which is particularly useful for where you need an item closely matching what you need,
    but you don't care if it's the *closest* matching item. An example of where this is
    super useful to have is in memory allocators, where you need a free block bigger
    than or equal to the size you are allocating.

    There is also a closest rather than closely matching item find, however it can have
    O(log N) worst case complexity, albeit this is rare in well distributed keys.

    The order of the items is *approximately* sorted by key incrementing. Note the
    approximately, this is a nearly-sorted sequence suitable for say bubble sorting
    if you need perfectly sorted.

    Items inserted with the same key preserve order of insertion. Performance is superb
    on all CPUs which have a single cycle opcode for finding the first set bit in a
    key. If your CPU has a slow bitscan opcode, performance is merely good rather than
    superb.

    The index is intrusive, as in, your types must provide the storage needed by the
    index for housekeeping. You can very tightly pack or compress or calculate those
    numbers by defining a specialised `bitwise_trie_head_accessors<Base, ItemType>`
    if you wish to customise storage of housekeeping for the trie index head; and/or
    specialise `bitwise_trie_item_accessors<ItemType>` if you wish to customise
    storage of housekeeping for each item indexed by the trie. The default
    implementations of those accessor types require various member variables prefixed
    with `trie_` in your types, see their documentation for which.

    Most of this implementation is lifted from https://github.com/ned14/nedtries, but
    it has been modernised for current C++ idomatic practice.
    */
    template <class Base, class ItemType, int NobbleDir = 0> class bitwise_trie : public Base
    {
      constexpr bitwise_trie_head_accessors<const Base, const ItemType> head_accessors() const noexcept
      {
        return bitwise_trie_head_accessors<const Base, const ItemType>(this);
      }
      constexpr bitwise_trie_head_accessors<Base, ItemType> head_accessors() noexcept { return bitwise_trie_head_accessors<Base, ItemType>(this); }

      static constexpr bitwise_trie_item_accessors<const ItemType> item_accessors(const ItemType *item) noexcept
      {
        return bitwise_trie_item_accessors<const ItemType>(item);
      }
      static constexpr bitwise_trie_item_accessors<ItemType> item_accessors(ItemType *item) noexcept { return bitwise_trie_item_accessors<ItemType>(item); }

    public:
      //! Key type indexing the items
      using key_type = decltype(item_accessors(nullptr).key());
      //! The type of item indexed
      using mapped_type = ItemType;
      //! The value type
      using value_type = ItemType;
      //! The size type
      using size_type = decltype(static_cast<bitwise_trie *>(nullptr)->head_accessors().size());
      //! The type of a difference between pointers to the type of item indexed
      using difference_type = ptrdiff_t;
      //! A reference to the type of item indexed
      using reference = value_type &;
      //! A const reference to the type of item indexed
      using const_reference = const value_type &;
      //! A pointer to the type of item indexed
      using pointer = value_type *;
      //! A const pointer to the type of item indexed
      using const_pointer = const value_type *;
      //! The direction of nobble configured.
      static constexpr int nobble_direction = (NobbleDir < 0) ? -1 : ((NobbleDir > 0) ? 1 : 0);

    private:
      static_assert(std::is_unsigned<key_type>::value, "key type must be unsigned");
      static_assert(std::is_unsigned<size_type>::value, "head_accessor size type must be unsigned");

      bool _to_nobble() noexcept { detail::nobble_function_implementation<nobble_direction>(head_accessors()); }

      inline void _triecheckvalidity() const noexcept;

      const_pointer _triemin() const noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        const_pointer node = 0, child;
        if(0 == head.size())
        {
          return nullptr;
        }
        for(unsigned bitidx = 0; bitidx < _key_type_bits && nullptr == (node = head.child(bitidx)); bitidx++)
          ;
        assert(node != nullptr);
        return node;
      }
      pointer _triemin() noexcept { return const_cast<pointer>(static_cast<const bitwise_trie *>(this)->_triemin()); }
      const_pointer _triemax() const noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        const_pointer node = 0, child;
        if(0 == head.size())
        {
          return nullptr;
        }
        for(unsigned bitidx = _key_type_bits - 1; bitidx < _key_type_bits && nullptr == (node = head.child(bitidx)); bitidx--)
          ;
        assert(node != nullptr);
        auto nodelink = item_accessors(node);
        while(nullptr != (child = (nodelink.child(true) != nullptr) ? nodelink.child(true) : nodelink.child(false)))
        {
          node = child;
          nodelink = item_accessors(node);
        }
        /* Now go to end leaf */
        if(nodelink.sibling(false) != nullptr)
        {
          return nodelink.sibling(false);
        }
        return node;
      }
      pointer _triemax() noexcept { return const_cast<pointer>(static_cast<const bitwise_trie *>(this)->_triemax()); }

      bool _trieinsert(pointer *r) noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        if(head.size() + 1 < head.size())
        {
          return false;
        }

        const_pointer node = nullptr;
        auto nodelink = item_accessors(node);
        auto rlink = item_accessors(r);
        key_type rkey = rlink.key();

        rlink.set_parent(nullptr);
        rlink.set_child(false, nullptr);
        rlink.set_child(true, nullptr);
        rlink.set_sibling(false, nullptr);
        rlink.set_sibling(true, nullptr);
        unsigned bitidx = detail::bitscanr(rkey);
        /* Avoid unknown bit shifts where possible, their performance can suck */
        key_type keybit = (key_type) 1 << bitidx;
        assert(bitidx < _key_type_bits);
        if(nullptr == (node = head.child(bitidx)))
        { /* Set parent is index flag */
          rlink.set_parent_is_index(bitidx);
          head.set_child(bitidx) = r;
          goto end;
        }
        for(;; node = childnode)
        {
          nodelink = item_accessors(node);
          key_type nodekey = nodelink.key();
          if(nodekey == rkey)
          { /* Insert into end of ring list */
            rlink.set_is_secondary_sibling();
            rlink.set_sibling(true, node);
            if(auto p = nodelink.sibling(false))
            {
              rlink.set_sibling(false, p);
            }
            else
            {
              rlink.set_sibling(false, node);
            }
            nodelink.set_sibling(false, r);
            if(nullptr = nodelink.sibling(true))
            {
              nodelink.set_sibling(true, r);
            }
            break;
          }
          keybit >>= 1;
          const bool keybitset = !!(rkey & keybit);
          auto *childnode = nodelink.child(keybitset);
          if(nullptr == childnode)
          { /* Insert here */
            rlink.set_parent(node);
            nodelink.set_child(keybitset, r);
            break;
          }
        }
      end:
        head.incr_size();
#if QUICKCPPLIB_ALGORITHM_BITWISE_TRIE_DEBUG
        _triecheckvalidity();
#endif
        return true;
      }

      static const_pointer _triebranchprev(const_pointer r, bitwise_trie_item_accessors<const ItemType> *rlinkaddr = nullptr) noexcept
      {
        const_pointer node = nullptr, child = nullptr;
        auto nodelink = item_accessors(node);
        auto rlink = item_accessors(r);

        /* Am I a leaf off the tree? */
        if(rlink.sibling(false) != nullptr && !item_accessors(rlink.sibling(false)).is_primary_sibling())
        {
          return rlink.sibling(false);
        }
        /* Trace up my parents to prev branch */
        while(!rlink.parent_is_index())
        {
          node = rlink.parent();
          nodelink = item_accessors(node);
          /* If I was on child[1] and there is a child[0], go to bottom of child[0] */
          if(nodelink.child(true) == r && nodelink.child(false) != nullptr)
          {
            node = nodelink.child(false);
            nodelink = item_accessors(node);
            /* Follow child[1] preferentially downwards */
            while(nullptr != (child = (nodelink.child(true) != nullptr) ? nodelink.child(1) : nodelink.child(0)))
            {
              node = child;
              nodelink = item_accessors(node);
            }
          }
          /* If I was already on child[0] or there are no more children, return this node */
          /* Now go to end leaf */
          if(nodelink.sibling(false) != nullptr)
          {
            return nodelink.sibling(false);
          }
          return node;
        }
        /* I have reached the top of my trie, no more on this branch */
        if(rlinkaddr != nullptr)
        {
          *rlinkaddr = rlink;
        }
        return nullptr;
      }
      const_pointer _trieprev(const_pointer r) const noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        const_pointer node = nullptr, child = nullptr;
        auto nodelink = item_accessors(node);
        auto rlink = item_accessors(nullptr);

        if((node = _triebranchprev(r, &rlink)) != nullptr || !rlink)
        {
          return node;
        }
        /* I have reached the top of my trie, so on to prev bin */
        unsigned bitidx = rlink.bit_index();
        assert(head.child(bitidx) == r);
        for(bitidx--; bitidx < _key_type_bits && nullptr == (node = head.child(bitidx)); bitidx--)
          ;
        if(bitidx >= _key_type_bits)
        {
          return nullptr;
        }
        nodelink = item_accessors(node);
        /* Follow child[1] preferentially downwards */
        while(nullptr != (child = (nodelink.child(1) != nullptr) ? nodelink.child(1) : nodelink.child(0)))
        {
          node = child;
          nodelink = item_accessors(node);
        }
        /* Now go to end leaf */
        if(nodelink.sibling(false) != nullptr)
        {
          return nodelink.sibling(false);
        }
        return node;
      }
      pointer _trieprev(const_pointer r) noexcept { return const_cast<pointer>(static_cast<const bitwise_trie *>(this)->_trieprev(r)); }

      static const_pointer _triebranchnext(const_pointer r, bitwise_trie_item_accessors<const ItemType> *rlinkaddr = nullptr) noexcept
      {
        const_pointer node = nullptr;
        auto nodelink = item_accessors(node);
        auto rlink = item_accessors(r);

        /* Am I a leaf off the tree? */
        if(rlink.sibling(true) != nullptr && !item_accessors(rlink.sibling(true)).is_primary_sibling())
        {
          return rlink.sibling(true);
        }
        /* If I am the end leaf off a tree, put me back at my tree node */
        while(!rlink.is_primary_sibling())
        {
          r = rlink.sibling(true);
          rlink = item_accessors(r);
        }
        /* Follow my children, preferring child[0] */
        if(nullptr != (node = (rlink.child(false) != nullptr) ? rlink.child(false) : rlink.child(true)))
        {
          nodelink = item_accessors(node);
          assert(nodelink.parent() == r);
          return node;
        }
        /* Trace up my parents to next branch */
        while(!rlink.parent_is_index())
        {
          node = rlink.parent();
          nodelink = item_accessors(node);
          if(nodelink.child(false) == r && nodelink.child(true) != nullptr)
          {
            return nodelink.child(true);
          }
          r = node;
          rlink = nodelink;
        }
        /* I have reached the top of my trie, no more on this branch */
        if(rlinkaddr != nullptr)
        {
          *rlinkaddr = rlink;
        }
        return nullptr;
      }
      const_pointer _trienext(const_pointer r) const noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        const_pointer node = nullptr;
        auto rlink = item_accessors(nullptr);

        if((node = _triebranchnext(r, &rlink)) != nullptr)
        {
          return node;
        }
        /* I have reached the top of my trie, so on to next bin */
        unsigned bitidx = rlink.bit_index();
        for(bitidx++; bitidx < _key_type_bits && nullptr == (node = head.child(bitidx)); bitidx++)
          ;
        if(bitidx >= _key_type_bits)
        {
          return nullptr;
        }
        return node;
      }
      pointer _trienext(const_pointer r) noexcept { return const_cast<pointer>(static_cast<const bitwise_trie *>(this)->_trienext(r)); }

#ifndef NDEBUG
      struct _trie_validity_state
      {
        size_type count, tops, lefts, rights, leafs;
        key_type smallestkey, largestkey;
      };

      static void _triecheckvaliditybranch(const_pointer node, key_type bitidx, _trie_validity_state &state) noexcept
      {
        const_pointer child;
        auto nodelink = item_accessors(node);
        key_type nodekey = nodelink.key();

        if(nodekey < state.smallestkey)
          state.smallestkey = nodekey;
        if(nodekey > state.largestkey)
          state.largestkey = nodekey;
        assert(nodelink.parent() != nullptr);
        auto *child = nodelink.parent();
        auto childlink = item_accessors(child);
        assert(childlink.child(0) == node || childlink.child(1) == node);
        assert(node == childlink.child(!!(nodekey & ((size_t) 1 << bitidx))));
        while((child = nodelink.sibling(true)).is_secondary_sibling())
        {
          state.leafs++;
          childlink = item_accessors(child);
          assert(nullptr == childlink.parent());
          assert(nullptr == childlink.child(0));
          assert(nullptr == childlink.child(1));
          assert(child == item_accessors(child.sibling(false)).sibling(true));
          assert(child == item_accessors(child.sibling(true)).sibling(false));
          nodelink = childlink;
          state.count++;
        }
        nodelink = item_accessors(node);
        state.count++;
        if(nodelink.child(0) != nullptr)
        {
          state.lefts++;
          _triecheckvaliditybranch(nodelink.child(0), bitidx - 1, state);
        }
        if(nodelink.child(1) != nullptr)
        {
          state.rights++;
          _triecheckvaliditybranch(nodelink.child(1), bitidx - 1, state);
        }
      }
#endif
      void _triecheckvalidity() const noexcept
      {
#ifndef NDEBUG
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        const_pointer node = nullptr, child = nullptr;
        _trie_validity_state state;
        memset(&state, 0, sizeof(state));
        for(unsigned n = 0; n < _key_type_bits; n++)
        {
          if((node = head.child(n)) != nullptr)
          {
            auto nodelink = item_accessors(node);
            key_type nodekey = nodelink.key();
            state.tops++;
            auto bitidx = nodelink.bit_index();
            assert(bitidx == n);
            assert(head.child(bitidx) == node);
            assert(0 == nodekey || ((((size_t) -1) << bitidx) & nodekey) == ((size_t) 1 << bitidx));
            while((child = nodelink.sibling(true)).is_secondary_sibling())
            {
              state.leafs++;
              auto childlink = item_accessors(child);
              assert(nullptr == childlink.parent());
              assert(nullptr == childlink.child(0));
              assert(nullptr == childlink.child(1));
              assert(child == item_accessors(child.sibling(false)).sibling(true));
              assert(child == item_accessors(child.sibling(true)).sibling(false));
              nodelink = childlink;
              state.count++;
            }
            nodelink = item_accessors(node);
            state.count++;
            if(nodelink.child(0) != nullptr)
            {
              state.lefts++;
              state.smallestkey = (size_t) -1;
              state.largestkey = 0;
              _triecheckvaliditybranch(nodelink.child(0), bitidx - 1, state);
              assert(0 == state.smallestkey || state.smallestkey >= (size_t) 1 << bitidx);
              assert(state.largestkey < (size_t) 1 << (bitidx + 1));
            }
            if(nodelink.child(1) != nullptr)
            {
              state.rights++;
              state.smallestkey = (size_t) -1;
              state.largestkey = 0;
              _triecheckvaliditybranch(nodelink.child(1), bitidx - 1, state);
              assert(state.smallestkey >= (size_t) 1 << bitidx);
              assert(state.largestkey < (size_t) 1 << (bitidx + 1));
            }
          }
        }
        assert(state.count == head.size());
        for(state.count = 0, node = _triemin(); node != nullptr; (node = _trienext(node)), state.count++)
#if 0
      printf("%p\n", node)
#endif
          ;
        if(state.count != head.size())
        {
          assert(state.count == head.size());
        }
#if 0
    printf("\n");
#endif
        for(state.count = 0, node = _triemax(); node != nullptr; (node = _trieprev(node)), state.count++)
#if 0
      printf("%p\n", node)
#endif
          ;
        if(state.count != head.size())
        {
          assert(state.count == head.size());
        }
#if 0
    printf("\n");
#endif
#if !defined(NDEBUG) && 0
        if(count > 50)
          printf("Of count %u, tops %.2lf%%, lefts %.2lf%%, rights %.2lf%%, leafs %.2lf%%\n", count, 100.0 * tops / count, 100.0 * lefts / count,
                 100.0 * rights / count, 100.0 * leafs / count);
#endif
#endif /* !NDEBUG */
      }

    private:
      template <bool is_const, class Parent, class Pointer, class Reference> class iterator_
      {
        friend class bitwise_trie;
        template <bool is_const, class Parent, class Pointer, class Reference> friend class iterator_;
        Parent *_parent{nullptr};
        Pointer _p{nullptr};

        static_assert(is_const == std::is_const<typename std::remove_reference<decltype(*_p)>::type>::value, "");

        iterator_ &_inc() noexcept
        {
          if(_parent != nullptr && _p != nullptr)
          {
            _p = _parent->_trienext(_p);
          }
          return *this;
        }
        iterator_ &_dec() noexcept
        {
          if(_parent != nullptr)
          {
            if(_p == nullptr)
            {
              _p = _parent->_triemax();
            }
            else
            {
              _p = _parent->_trieprev(_p);
            }
          }
          return *this;
        }

        constexpr iterator_(Parent *parent, Pointer &&p) noexcept
            : _parent(parent)
            , _p(std::move(p))
        {
        }
        constexpr iterator_(Parent *parent) noexcept
            : _parent(parent)
            , _p(nullptr)
        {
        }

      public:
        constexpr iterator_() noexcept
            : _parent(nullptr)
            , _p(nullptr)
        {
        }
        constexpr iterator_(const iterator_ &) = default;
        constexpr iterator_(iterator_ &&) noexcept = default;
        constexpr iterator_ &operator=(const iterator_ &) = default;
        constexpr iterator_ &operator=(iterator_ &&) noexcept = default;
        // Non-const to const iterator
        template <class _Parent, class _Pointer, class _Reference,
                  typename = typename std::enable_if<std::is_same<_Parent, _Parent>::value && is_const, _Parent>::type>
        constexpr iterator_(const iterator_<false, _Parent, _Pointer, _Reference> &o) noexcept
            : _parent(o._parent)
            , _p(o._p)
        {
        }
        template <class _Parent, class _Pointer, class _Reference,
                  typename = typename std::enable_if<std::is_same<_Parent, _Parent>::value && is_const, _Parent>::type>
        constexpr iterator_(iterator_<false, _Parent, _Pointer, _Reference> &&o) noexcept
            : _parent(std::move(o._parent))
            , _p(std::move(o._p))
        {
        }
        void swap(iterator_ &o) noexcept
        {
          std::swap(_parent, o._parent);
          std::swap(_p, o._p);
        }

        explicit operator bool() const noexcept { return _parent != nullptr && _p != nullptr; }
        bool operator!() const noexcept { return _parent == nullptr || _p == nullptr; }
        underlying_pointer_type operator->() noexcept { return _p; }
        const_underlying_pointer_type operator->() const noexcept { return _p; }
        bool operator==(const iterator_ &o) const noexcept { return _parent == o._parent && _p == o._p; }
        bool operator!=(const iterator_ &o) const noexcept { return _parent != o._parent || _p != o._p; }
        Reference operator*() noexcept
        {
          if(_parent == nullptr || _p == nullptr)
          {
            abort();
          }
          return *_p;
        }
        const Reference operator*() const noexcept
        {
          if(_parent == nullptr || _p == nullptr)
          {
            abort();
          }
          return *_p;
        }
        iterator_ &operator++() noexcept { return _inc(); }
        iterator_ operator++(int) noexcept
        {
          iterator_ ret(*this);
          ++*this;
          return ret;
        }
        iterator_ &operator--() noexcept { return _dec(); }
        iterator_ operator--(int) noexcept
        {
          iterator_ ret(*this);
          --*this;
          return ret;
        }
        bool operator<(const iterator_ &o) const noexcept
        {
          if(_parent != o._parent)
          {
            abort();
          }
          return _p != nullptr && (o._p == nullptr || _p < o._p);
        }
        bool operator>(const iterator_ &o) const noexcept
        {
          if(_parent != o._parent)
          {
            abort();
          }
          return o._p != nullptr && (_p == nullptr || _p > o._p);
        }
        bool operator<=(const iterator_ &o) const noexcept
        {
          if(_parent != o._parent)
          {
            abort();
          }
          return !(o > *this);
        }
        bool operator>=(const iterator_ &o) const noexcept
        {
          if(_parent != o._parent)
          {
            abort();
          }
          return !(o < *this);
        }
      };

    public:
      //! The iterator type
      using iterator = iterator_<false, bitwise_trie, pointer, reference>;
      //! The const iterator type
      using const_iterator = iterator_<true, const bitwise_trie, const_pointer, const_reference>;
      //! The reverse iterator type
      using reverse_iterator = std::reverse_iterator<iterator>;
      //! The const reverse iterator type
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;

      constexpr bitwise_trie() { clear(); }
      bitwise_trie(const bitwise_trie &o) noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto myhead = head_accessors();
        auto ohead = o.head_accessors();
        for(unsigned n = 0; n < _key_type_bits; n++)
        {
          head.set_child(n, ohead.child(n));
        }
        head.set_size(ohead.size());
      }
      bitwise_trie &operator=(const bitwise_trie &o) noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto myhead = head_accessors();
        auto ohead = o.head_accessors();
        for(unsigned n = 0; n < _key_type_bits; n++)
        {
          head.set_child(n, ohead.child(n));
        }
        head.set_size(ohead.size());
        return *this;
      }

      //! Swaps the contents of the index
      void swap(bitwise_trie &o) noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto myhead = head_accessors();
        auto ohead = o.head_accessors();
        for(unsigned n = 0; n < _key_type_bits; n++)
        {
          auto t = head.child(n);
          head.set_child(n, ohead.child(n));
          ohead.set_child(n, t);
        }
        auto t = head.size();
        head.set_size(ohead.size());
        ohead.set_size(t);
      }

      //! True if the bitwise trie is empty
      constexpr QUICKCPPLIB_NODISCARD empty() const noexcept { return size() == 0; }
      //! Returns the number of items in the bitwise trie
      constexpr size_type size() const noexcept { return head_accessors().size(); }
      //! Returns the maximum number of items in the index
      constexpr size_type max_size() const noexcept { return (size_type) -1; }

      //! Returns the front of the index.
      reference front() noexcept
      {
        if(auto p = _triemin())
        {
          return *p;
        }
        abort();
      }
      //! Returns the front of the index.
      const_reference front() const noexcept
      {
        if(auto p = _triemin())
        {
          return *p;
        }
        abort();
      }
      //! Returns the back of the index.
      reference back() noexcept
      {
        if(auto p = _triemax())
        {
          return *p;
        }
        abort();
      }
      //! Returns the back of the index.
      const_reference back() const noexcept
      {
        if(auto p = _triemax())
        {
          return *p;
        }
        abort();
      }

      //! Returns an iterator to the first item in the index.
      iterator begin() noexcept
      {
        if(auto p = _triemin())
        {
          return iterator(this, p);
        }
        return iterator(this);
      }
      //! Returns an iterator to the first item in the index.
      const_iterator begin() const noexcept
      {
        if(auto p = _triemin())
        {
          return const_iterator(this, p);
        }
        return const_iterator(this);
      }
      //! Returns an iterator to the first item in the index.
      const_iterator cbegin() const noexcept { return begin(); }
      //! Returns an iterator to the item after the last in the index.
      iterator end() noexcept { return iterator(this); }
      //! Returns an iterator to the item after the last in the index.
      const_iterator end() const noexcept { return const_iterator(this); }
      //! Returns an iterator to the item after the last in the index.
      const_iterator cend() const noexcept { return const_iterator(this); }

      //! Clears the index.
      constexpr void clear() noexcept
      {
        static constexpr unsigned _key_type_bits = (unsigned) (8 * sizeof(key_type));
        auto head = head_accessors();
        for(unsigned n = 0; n < _key_type_bits; n++)
        {
          head.set_child(n, nullptr);
        }
        head.set_size(0);
      }
      //! Return how many items with key there are.
      size_type count(key_type k) const noexcept
      {
        if(auto p = _triefind(k))
        {
          size_type ret = 1;
          auto plink = item_accessors(p);
          for(auto *i = plink.sibling(true); i != nullptr && i != p; i = item_accessors(i).sibling(true))
          {
            ret++;
          }
          return ret;
        }
        return 0;
      }
      //! Inserts a new item, returning an iterator to the new item. If the returned iterator is invalid,
      //! there is no more space.
      iterator insert(pointer p) noexcept
      {
        if(size() < max_size() && _trieinsert(p))
        {
          return { iterator(this, p); };
        }
        return end();
      }
      //! Erases an item.
      iterator erase(const_iterator it) noexcept
      {
        auto ret(it);
        ++ret;
        _trieremove(const_cast<pointer>(it._p));
        return it;
      }
      //! Erases an item.
      iterator erase(key_type k) noexcept { return erase(find(k)); }
      //! Finds an item
      iterator find(key_type k) const noexcept
      {
        if(auto p = _triefind(k))
        {
          return { iterator(this, p); };
        }
        return end();
      }
      //! True if the index contains the key
      bool contains(key_type k) const noexcept { return nullptr != _triefind(k); }
      //! Returns a reference to the specified element, aborting if key not found.
      reference operator[](key_type k) noexcept
      {
        if(auto p = _triefind(k))
        {
          return *p;
        }
        abort();
      }
      //! Returns a reference to the specified element, aborting if key not found.
      const_reference operator[](const key_type &k) const noexcept
      {
        if(auto p = _triefind(k))
        {
          return *p;
        }
        abort();
      }
    };
  }  // namespace bitwise_trie
}  // namespace algorithm

QUICKCPPLIB_NAMESPACE_END

#endif
