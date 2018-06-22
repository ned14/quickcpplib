/* Main memory persistence support
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: April 2018


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

#ifndef QUICKCPPLIB_PERSISTENT_HPP
#define QUICKCPPLIB_PERSISTENT_HPP

#include "config.hpp"

#include <atomic>

#ifdef _MSC_VER
#include <intrin.h>
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace persistence
{
  /*! \brief The kinds of cache line flushing which can be performed. */
  enum class memory_flush
  {
    memory_flush_none,    //!< No memory flushing.
    memory_flush_retain,  //!< Flush modified cache line to memory, but retain as unmodified in cache.
    memory_flush_evict    //!< Flush modified cache line to memory, and evict completely from all caches.
  };
  //! No memory flushing.
  constexpr memory_flush memory_flush_none = memory_flush::memory_flush_none;
  //! Flush modified cache line to memory, but retain as unmodified in cache.
  constexpr memory_flush memory_flush_retain = memory_flush::memory_flush_retain;
  //! Flush modified cache line to memory, and evict completely from all caches.
  constexpr memory_flush memory_flush_evict = memory_flush::memory_flush_evict;

  namespace detail
  {
    using flush_impl_type = memory_flush (*)(const void *addr, memory_flush kind);
    inline QUICKCPPLIB_NOINLINE flush_impl_type make_flush_impl() noexcept
    {
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#ifndef _MSC_VER
      static const auto __cpuidex = [](int *cpuInfo, int func1, int func2) { __asm__ __volatile__("cpuid\n\t" : "=a"(cpuInfo[0]), "=b"(cpuInfo[1]), "=c"(cpuInfo[2]), "=d"(cpuInfo[3]) : "a"(func1), "c"(func2)); };  // NOLINT
      // static constexpr auto _mm_clwb = [](const void *addr) { __asm__ __volatile__("clwb (%0)\n\t" : : "r"(addr)); };                                                                                                     // NOLINT
      static const auto _mm_clwb = [](const void *addr) { __asm__ __volatile__(".byte 0x0f, 0xae, 0x30\n\t" : : "a"(addr)); };  // NOLINT
      static const auto _mm_clflushopt = [](const void *addr) { __asm__ __volatile__("clflushopt (%0)\n\t" : : "r"(addr)); };   // NOLINT
      static const auto _mm_clflush = [](const void *addr) { __asm__ __volatile__("clflush (%0)\n\t" : : "r"(addr)); };         // NOLINT
      static const auto _mm_sfence = []() { __asm__ __volatile__("sfence\n\t"); };                                              // NOLINT
#endif
      int nBuff[4];
      __cpuidex(nBuff, 0x7, 0x0);
      if(nBuff[1] & (1 << 24))  // has CLWB instruction
      {
        return [](const void *addr, memory_flush kind) -> memory_flush {
          if(kind == memory_flush_retain)
          {
            _mm_clwb(addr);
            _mm_sfence();
            return memory_flush_retain;
          }
          _mm_clflushopt(addr);
          _mm_sfence();
          return memory_flush_evict;
        };
      }
      if(nBuff[1] & (1 << 23))  // has CLFLUSHOPT instruction
      {
        return [](const void *addr, memory_flush /*unused*/) -> memory_flush {
          _mm_clflushopt(addr);
          _mm_sfence();
          return memory_flush_evict;
        };
      }
      else
      {
        // Use CLFLUSH instruction
        return [](const void *addr, memory_flush /*unused*/) -> memory_flush {
          _mm_clflush(addr);
          return memory_flush_evict;
        };
      }
#elif defined(__aarch64__)
      return [](const void *addr, memory_flush kind) -> memory_flush {
        if(kind == memory_flush_retain)
        {
          __asm__ __volatile__("dc cvac, %0" : : "r"(addr) : "memory");
          __asm__ __volatile__("dmb ish" : : : "memory");
          return memory_flush_retain;
        }
        __asm__ __volatile__("dc civac, %0" : : "r"(addr) : "memory");
        __asm__ __volatile__("dmb ish" : : : "memory");
        return memory_flush_evict;
      };
#else
#error Unsupported platform
#endif
    }
    inline flush_impl_type flush_impl() noexcept
    {
      static flush_impl_type f;
      if(f != nullptr)
        return f;
      f = make_flush_impl();
      return f;
    }
  }

  /*! \class persistent
  \brief Extends `std::atomic<T>` with persistent memory support.

  Persistent memory based storage can be `fdatasync()`ed merely by writing out any
  dirty cache lines in the CPU's caches to main memory/mapped remote memory. This
  class extends `std::atomic<T>` with a `.flush()` member function which calls
  architecture-specific opcodes to cause the CPU to immediately write the dirty cache
  line specified to main memory, marking on completion the cache line as unmodified.
  `.store()`, and all the other member functions which can modify state, will
  call `.flush(memory_flush_retain)` on your behalf if the `memory_order` contains release or sequential
  consistent semantics.

  Note that calling `.flush()` repeatedly on the same cache line will likely produce
  poor performance. You are advised to use non-release semantics to modify the cache
  line, then your final modification to that cache line ought to release. This will
  release the whole cache line at once, avoiding constant read-modify-write cycles
  (unless the latter is exactly what you need of course).

  \warning On older Intel CPUs, due to lack of hardware support, we always execute
  `memory_flush_evict` even if asked for `memory_flush_retain`. This can produce
  some very poor performance. Check the value returned by `.flush()` to see what
  kind of flush was actually performed.
  */
  template <class T> class persistent : protected std::atomic<T>
  {
    using _base = std::atomic<T>;
    bool _needflush(std::memory_order order) const noexcept
    {
      switch(order)
      {
      case std::memory_order_relaxed:
      case std::memory_order_consume:
      case std::memory_order_acquire:
        return false;
      case std::memory_order_release:
      case std::memory_order_acq_rel:
      case std::memory_order_seq_cst:
        return true;
      }
      return false;
    }

  public:
    using _base::_base;
    using _base::operator=;
    using _base::is_lock_free;
    using _base::load;
    using _base::operator T;

    //! \brief If order releases or seqcst, flush the store to main memory.
    void store(T v, std::memory_order order = std::memory_order_seq_cst, memory_flush kind = memory_flush_retain)
    {
      _base::store(v, order);
      if(_needflush(order))
      {
        flush(kind);
      }
    }
    //! \brief If order releases or seqcst, flush the exchange to main memory.
    T exchange(T v, std::memory_order order = std::memory_order_seq_cst, memory_flush kind = memory_flush_retain)
    {
      T r = _base::exchange(v, order);
      if(_needflush(order))
      {
        flush(kind);
      }
      return r;
    }
    //! \brief If the exchange is successful and successful order releases or seqcst, flush the exchange to main memory.
    bool compare_exchange_weak(T &expected, T v, std::memory_order success, std::memory_order failure, memory_flush kind = memory_flush_retain)
    {
      bool r = _base::compare_exchange_weak(expected, v, success, failure);
      if(r && _needflush(success))
      {
        flush(kind);
      }
      return r;
    }
    //! \brief If the exchange is successful and order releases or seqcst, flush the exchange to main memory.
    bool compare_exchange_weak(T &expected, T v, std::memory_order order = std::memory_order_seq_cst, memory_flush kind = memory_flush_retain)
    {
      bool r = _base::compare_exchange_weak(expected, v, order);
      if(r && _needflush(order))
      {
        flush(kind);
      }
      return r;
    }
    //! \brief If the exchange is successful and successful order releases or seqcst, flush the exchange to main memory.
    bool compare_exchange_strong(T &expected, T v, std::memory_order success, std::memory_order failure, memory_flush kind = memory_flush_retain)
    {
      bool r = _base::compare_exchange_strong(expected, v, success, failure);
      if(r && _needflush(success))
      {
        flush(kind);
      }
      return r;
    }
    //! \brief If the exchange is successful and order releases or seqcst, flush the exchange to main memory.
    bool compare_exchange_strong(T &expected, T v, std::memory_order order = std::memory_order_seq_cst, memory_flush kind = memory_flush_retain)
    {
      bool r = _base::compare_exchange_strong(expected, v, order);
      if(r && _needflush(order))
      {
        flush(kind);
      }
      return r;
    }
#define QUICKCPPLIB_PERSISTENT_OP(name)                                                                                                                                                                                                                                                                                        \
  T name(T v, memory_flush kind = memory_flush_retain)                                                                                                                                                                                                                                                                         \
  {                                                                                                                                                                                                                                                                                                                            \
    T r = _base::name(v);                                                                                                                                                                                                                                                                                                      \
    flush(kind);                                                                                                                                                                                                                                                                                                               \
    return r;                                                                                                                                                                                                                                                                                                                  \
  }                                                                                                                                                                                                                                                                                                                            \
  T name##_explicit(T v, std::memory_order order, memory_flush kind = memory_flush_retain)                                                                                                                                                                                                                                     \
  {                                                                                                                                                                                                                                                                                                                            \
    T r = _base::name##_explicit(v, order);                                                                                                                                                                                                                                                                                    \
    if(_needflush(order))                                                                                                                                                                                                                                                                                                      \
      flush(kind);                                                                                                                                                                                                                                                                                                             \
    return r;                                                                                                                                                                                                                                                                                                                  \
  }
    QUICKCPPLIB_PERSISTENT_OP(fetch_add)
    QUICKCPPLIB_PERSISTENT_OP(fetch_sub)
    QUICKCPPLIB_PERSISTENT_OP(fetch_and)
    QUICKCPPLIB_PERSISTENT_OP(fetch_or)
    QUICKCPPLIB_PERSISTENT_OP(fetch_xor)
#undef QUICKCPPLIB_PERSISTENT_OP

    /*! \brief Flush the cache line containing this object instance to main memory,
    optionally evicting it from all caches entirely.
    \return The kind of flush actually performed.
    \param kind What kind of flush to do. See `memory_flush`.
    */
    memory_flush flush(memory_flush kind = memory_flush_retain) const noexcept { return detail::flush_impl()(this, kind); }
  };
}

QUICKCPPLIB_NAMESPACE_END

#endif
