/* Thread related functions
(C) 2016-2022 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Dec 2022


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

#include "../../utils/thread.hpp"

#include "../../algorithm/bitwise_trie.hpp"
#include "../../algorithm/fast_atomic_128.hpp"
#include "../../algorithm/memory.hpp"
#include "../../byte.hpp"
#include "../../optional.hpp"
#include "../../scope.hpp"
#include "../../span.hpp"
#include "../../spinlock.hpp"
#include "../../utils/page_memory.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>  // for memset etc

#ifndef _WIN32
#include <time.h>    // for timers
#include <unistd.h>  // for write()
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace utils
{
  namespace thread
  {
    namespace detail
    {
      template <class T> class page_memory_storage_ptr
      {
        page_memory::page_memory_ptr _mempage;

      public:
        using pointer_type = T *;
        page_memory_storage_ptr() = default;
        page_memory_storage_ptr(page_memory::page_memory_ptr p)
            : _mempage(static_cast<page_memory::page_memory_ptr &&>(p))
        {
        }
        page_memory_storage_ptr(const page_memory_storage_ptr &) = delete;
        page_memory_storage_ptr(page_memory_storage_ptr &&) = default;
        page_memory_storage_ptr &operator=(const page_memory_storage_ptr &) = delete;
        page_memory_storage_ptr &operator=(page_memory_storage_ptr &&) = default;
        ~page_memory_storage_ptr() = default;

        explicit operator bool() const noexcept { return !!_mempage; }
        const T &operator*() const noexcept { return *reinterpret_cast<const T *>(_mempage.get()); }
        T &operator*() noexcept { return *reinterpret_cast<T *>(_mempage.get()); }
        T *operator->() noexcept { return reinterpret_cast<T *>(_mempage.get()); }
        const T *get() const noexcept { return reinterpret_cast<const T *>(_mempage.get()); }
        T *get() noexcept { return reinterpret_cast<T *>(_mempage.get()); }
        void release() noexcept { _mempage.release(); }
        size_t size() const noexcept { return _mempage.size() / sizeof(T); }
        size_t size_bytes() const noexcept { return _mempage.size(); }
        span::span<T> as_span() const noexcept { return {reinterpret_cast<T *>(_mempage.get()), size()}; }
      };
      // This class is a touch UB-ey, I wish we had atomic support for move bitcopying types
      template <class T> class atomic_page_memory_storage_ptr
      {
        struct _trivally_copyable_storage
        {
          byte::byte _storage[sizeof(page_memory_storage_ptr<T>)];
        };
        union _storage_union
        {
#if QUICKCPPLIB_PLATFORM_NATIVE_BITLENGTH > 32
          static_assert(sizeof(_trivally_copyable_storage) == 16, "page_memory_storage_ptr is not 16 bytes long!");
          algorithm::fast_atomic_128::fast_atomic_128<_trivally_copyable_storage> atomic;
#else
          std::atomic<_trivally_copyable_storage> atomic;
#endif
          _trivally_copyable_storage nonatomic;
          page_memory_storage_ptr<T> ptr;
          constexpr _storage_union()
          {
            algorithm::memory::cmemset(nonatomic._storage, byte::byte(0), sizeof(nonatomic._storage));
          }
          ~_storage_union() { ptr.~page_memory_storage_ptr<T>(); }
        } _storage;
        span::span<T> _load() const noexcept
        {
          union _
          {
            _trivally_copyable_storage nonatomic;
            page_memory_storage_ptr<T> ptr;
            _()
                : nonatomic{}
            {
            }
            ~_() {}
          } ret;
          ret.nonatomic = _storage.atomic.load(std::memory_order_acquire);
          return {ret.ptr ? ret.ptr.get() : nullptr, ret.ptr.size()};
        }

      public:
        using value_type = page_memory_storage_ptr<T>;
        atomic_page_memory_storage_ptr()
            : _storage()
        {
        }
        atomic_page_memory_storage_ptr(const atomic_page_memory_storage_ptr &) = delete;
        atomic_page_memory_storage_ptr(atomic_page_memory_storage_ptr &&) = delete;
        atomic_page_memory_storage_ptr &operator=(const atomic_page_memory_storage_ptr &) = delete;
        atomic_page_memory_storage_ptr &operator=(atomic_page_memory_storage_ptr &&) = delete;
        ~atomic_page_memory_storage_ptr() = default;
        explicit operator bool() const noexcept
        {
          auto v = _load();
          return !!v.data();
        }
        const T &operator*() const noexcept
        {
          auto v = _load();
          return *v.data();
        }
        T &operator*() noexcept
        {
          auto v = _load();
          return *v.data();
        }
        T *operator->() noexcept
        {
          auto v = _load();
          return v.data();
        }
        const T *get() const noexcept
        {
          auto v = _load();
          return v.data();
        }
        T *get() noexcept
        {
          auto v = _load();
          return v.data();
        }
        size_t size() const noexcept
        {
          auto v = _load();
          return v.size();
        }
        size_t size_bytes() const noexcept
        {
          auto v = _load();
          return v.size();
        }
        span::span<T> as_span() const noexcept
        {
          auto v = _load();
          return v;
        }
        bool compare_exchange_strong(page_memory_storage_ptr<T> &expected,
                                     page_memory_storage_ptr<T> &&desired) noexcept
        {
          auto &expected_ = reinterpret_cast<_storage_union &>(expected);
          auto &desired_ = reinterpret_cast<_storage_union &>(desired);
          auto ret =
          _storage.atomic.compare_exchange_strong(expected_.nonatomic, desired_.nonatomic, std::memory_order_acq_rel);
          if(ret)
          {
            desired.release();
          }
          return ret;
        }
      };

      // Indexes an async_signal_safe_thread_local_base * to all the storage for that thread local
      // across all the threads. Also indexes a tid to the offset within an instance storage.
      struct instance_storage_segment
      {
        configurable_spinlock::spinlock<uint32_t> lock;
        using lock_guard_type = configurable_spinlock::lock_guard<configurable_spinlock::spinlock<uint32_t>>;

        struct value_type
        {
          value_type *trie_parent{nullptr};
          value_type *trie_child[2];
          union
          {
            const async_signal_safe_thread_local_base *obj;  // if actually a pointer, bottom two bits always clear
            // if actually a tid, the tid is shifted left two bits and the bottom two bits forced set
            uintptr_t trie_key{0};
          };

          page_memory::page_memory_ptr storage;  // WARNING: can relocate in memory as it expands
          union
          {
            size_t storage_bytes_used{0};  // if an object instance node, bytes of storage used
            size_t offset_into_storage;    // if a tid index, the offset into the parent storage
          };

          value_type() = default;

          // Constructor for a value_type referring to an object instance
          value_type(const async_signal_safe_thread_local_base *key, page_memory::page_memory_ptr _storage)
              : obj(key)
              , storage(std::move(_storage))
          {
          }

          // Constructor for a value_type referring to a tid index into object storage
          value_type(uint32_t key, size_t _offset_into_storage)
              : trie_key((uintptr_t(key) << 2) | 3)
              , offset_into_storage(_offset_into_storage)
          {
          }

          bool node_type_is_object_instance_to_storage() const noexcept { return (trie_key & 3) == 0; }
          bool node_type_is_tid_to_storage_offset() const noexcept { return (trie_key & 3) == 3; }
        };
        uint16_t trie_count{0};
        bool trie_nobbledir{false};
        value_type *trie_children[8 * sizeof(async_signal_safe_thread_local_base *)];

        using index_type = algorithm::bitwise_trie::bitwise_trie<instance_storage_segment, value_type>;

        const uint16_t storage_maximum;
        uint16_t storage_allocated{0};
        atomic_page_memory_storage_ptr<index_type> next;  // link to next storage segment

        value_type _storage[1];                           // carries on for reset of pagesize

        instance_storage_segment(size_t pagesize) noexcept
            : storage_maximum((pagesize - offsetof(instance_storage_segment, _storage)) / sizeof(value_type))
        {
        }

        value_type *allocate_value_type() noexcept
        {
          if(storage_allocated == storage_maximum)
          {
            return nullptr;
          }
          return &_storage[storage_allocated++];
        }
        void deallocate_value_type(value_type *v) noexcept
        {
          v->trie_parent = nullptr;
          v->trie_key = 0;
          while(storage_allocated > 0 && &_storage[storage_allocated - 1] == v && v->trie_key == 0)
          {
            v = &_storage[--storage_allocated];
          }
        }
      };
      extern QUICKCPPLIB_SYMBOL_VISIBLE inline atomic_page_memory_storage_ptr<instance_storage_segment::index_type> &
      first_atomic_page_memory_storage_ptr() noexcept
      {
        static atomic_page_memory_storage_ptr<instance_storage_segment::index_type> v;
        return v;
      }
      // If destroy_all is true, this is a request to destroy all associated resources. Returns true if
      // storage was just created for this thread and it needs to be initialised.
      QUICKCPPLIB_UTILS_THREAD_FUNC_DECL bool
      async_signal_safe_thread_local_access(bool destroy_all, void *&out,
                                            const async_signal_safe_thread_local_base *obj, uint32_t tid,
                                            uint16_t bytes, uint16_t align) noexcept
      {
        atomic_page_memory_storage_ptr<typename instance_storage_segment::index_type> *storage = nullptr;
        optional::optional<instance_storage_segment::lock_guard_type> g;
        bool new_object_constructed = false, new_storage_allocated = false;
        auto fill_it = [&storage, &g, &new_object_constructed, &new_storage_allocated, obj, tid, bytes,
                        align](auto &it, bool find_only, uintptr_t key, auto &&initvaluetype)
        {
          storage = &first_atomic_page_memory_storage_ptr();
          while(!it)
          {
            for(typename instance_storage_segment::index_type *ptr = storage->get(); ptr != nullptr;
                storage = &ptr->next, ptr = storage->get())
            {
              g.emplace(ptr->lock);
              auto it_ = ptr->find(key);
              if(it_ != ptr->end())
              {
                it = std::move(it_);
                break;
              }
            }
            if(!it)
            {
              g.reset();
              if(find_only)
              {
                return;
              }
              // We need to allocate a value for this key
              for(;;)
              {
                storage = &first_atomic_page_memory_storage_ptr();
                for(typename instance_storage_segment::index_type *ptr = storage->get(); ptr != nullptr;
                    storage = &ptr->next, ptr = storage->get())
                {
                  g.emplace(ptr->lock);
                  typename instance_storage_segment::value_type *this_new_storage =
                  storage->get()->allocate_value_type();
                  if(this_new_storage != nullptr)
                  {
                    this_new_storage = initvaluetype(this_new_storage, key);
                    it = ptr->insert(this_new_storage);
                    new_object_constructed = true;
                    break;
                  }
                }
                // g is left locking storage
                if(!it)
                {
                  page_memory_storage_ptr<typename instance_storage_segment::index_type> newsegment(
                  page_memory::make_page_memory(std::nothrow, 4096));
                  if(!newsegment)
                  {
                    fprintf(stderr,
                            "FATAL: Could not allocate indexed instance storage segment for "
                            "async_signal_safe_thread_local instance %p "
                            "thread id %u",
                            obj, tid);
                    abort();
                  }
                  new(newsegment.get()) typename instance_storage_segment::index_type(newsegment.size_bytes());
                  page_memory_storage_ptr<typename instance_storage_segment::index_type> empty_segment;
                  storage->compare_exchange_strong(empty_segment, std::move(newsegment));
                  new_storage_allocated = true;
                  continue;
                }
                break;
              }
            }
          }
        };
        optional::optional<instance_storage_segment::index_type::iterator> it_obj_;
        fill_it(it_obj_, destroy_all, uintptr_t(obj),
                [&](instance_storage_segment::value_type *v, uintptr_t)
                {
                  auto storage = page_memory::make_page_memory(std::nothrow, 4096);
                  if(!storage)
                  {
                    fprintf(stderr,
                            "FATAL: Could not allocate instance storage segment for "
                            "async_signal_safe_thread_local instance %p "
                            "thread id %u",
                            obj, tid);
                    abort();
                  }
                  return new(v) instance_storage_segment::value_type(obj, std::move(storage));
                });
        if(destroy_all)
        {
          if(it_obj_)
          {
            auto it_obj = std::move(*it_obj_);
            assert(g);
            assert(storage);
            instance_storage_segment::value_type &v = *it_obj;
            instance_storage_segment::index_type *ptr = storage->get();
            const auto overaligned_size = (bytes + align - 1) & ~(align - 1);
            for(size_t offset = 0; offset < v.storage_bytes_used; offset += overaligned_size)
            {
              auto *addr = (byte::byte *) v.storage.get() + offset;
              const_cast<async_signal_safe_thread_local_base *>(obj)->_destroy(addr);
            }
            v.~value_type();
            ptr->erase(it_obj);
            ptr->deallocate_value_type(&v);
          }
          out = nullptr;
          return false;
        }
        assert(it_obj_);
        auto it_obj = std::move(*it_obj_);
        assert(g);
        assert(storage);
        assert(it_obj->node_type_is_object_instance_to_storage());
        instance_storage_segment::value_type &obj_v = *it_obj;
        optional::optional<instance_storage_segment::index_type::iterator> it_tid;
        new_object_constructed = new_storage_allocated = false;
        bool ret = false;
        fill_it(it_tid, false, 3 | (uintptr_t(tid) << 2),
                [&](instance_storage_segment::value_type *v, uintptr_t)
                {
                  const auto overaligned_size = (bytes + align - 1) & ~(align - 1);
                  if(obj_v.storage_bytes_used + overaligned_size > obj_v.storage.size())
                  {
                    const size_t new_storage_size = obj_v.storage.size() + 4096;
                    if(new_storage_size > UINT16_MAX || !obj_v.storage.try_resize(std::nothrow, new_storage_size))
                    {
                      // TODO FIXME Need to allocate chains of additional storage
                      fprintf(stderr,
                              "FATAL: Could not expand instance storage for "
                              "async_signal_safe_thread_local instance %p "
                              "thread id %u",
                              obj, tid);
                      abort();
                    }
                  }
                  auto offset = obj_v.storage_bytes_used;
                  obj_v.storage_bytes_used += overaligned_size;
                  ret = true;
                  return new(v) instance_storage_segment::value_type(tid, offset);
                });
        assert(it_tid);
        assert((*it_tid)->node_type_is_tid_to_storage_offset());
        out = (byte::byte *) obj_v.storage.get() + (*it_tid)->offset_into_storage;
        return ret;
      }

    }  // namespace detail
  }    // namespace thread
}  // namespace utils

QUICKCPPLIB_NAMESPACE_END
