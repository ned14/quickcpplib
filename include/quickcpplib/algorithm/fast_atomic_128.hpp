/* Fast 128 bit atomics
(C) 2023 Niall Douglas <http://www.nedproductions.biz/> (7 commits)
File Created: Feb 2023


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

#ifndef QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_HPP
#define QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_HPP

#include "../config.hpp"

#include <atomic>
#include <cstddef>

#if defined(__AVX__)
#include <emmintrin.h>  // for __m128i
#endif


QUICKCPPLIB_NAMESPACE_BEGIN

namespace algorithm
{
  //! Defines a `std::atomic` specialisation for 128 bit trivially copyable types
  namespace fast_atomic_128
  {
    /*! \class fast_atomic_128
    \brief Defines a single CPU opcode `std::atomic` specialisation for 128 bit trivially copyable types.

    Usage is easy: declare your `std::atomic<T>` specialisation inheriting from `fast_atomic_128<T>`,
    and the appropriate CPU specific opcodes will be generated to do 128 bit atomic loads and stores
    **if** your compiler says your CPU is capable of doing so. Otherwise, it falls back onto the normal
    `__atomic_compare_exchange_16` etc runtime implementation of your compiler which does CPUID checking.

    Note that we take advantage on x64 of 16 byte MOV instructions being guaranteed atomic if and only
    if the CPU implements AVX. This is true for Intel and AMD, but may not be true for other x64
    implementations.

    (Why compilers don't already use the 128 bit atomic instructions is beyond me, but apart from clang
    they don't. See https://godbolt.org/z/EnbeeW4az to see what I mean)
    */
    QUICKCPPLIB_TEMPLATE(class T)
    QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_trivially_copyable<T>::value && sizeof(T) == 16))
    class alignas(16) fast_atomic_128
    {
      union _storage
      {
        T v{};
        uint64_t uints[2];
#if defined(__GNUC__) || defined(__clang__)
        std::atomic<unsigned __int128> u128atomic;
        unsigned __int128 u128;
#endif
#if defined(__AVX__)
        __m128i ssereg;
#endif
        constexpr _storage() {}
        constexpr _storage(T x)
            : v(x)
        {
        }
      } _state;

    public:
      fast_atomic_128() = default;
      constexpr fast_atomic_128(T v)
          : _state(v)
      {
      }
      fast_atomic_128(const fast_atomic_128 &) = delete;
      fast_atomic_128(fast_atomic_128 &&) = delete;
      fast_atomic_128 &operator=(const fast_atomic_128 &) = delete;
      fast_atomic_128 &operator=(fast_atomic_128 &&) = delete;
      ~fast_atomic_128() = default;

      T load(std::memory_order ord) const noexcept
      {
        (void) ord;
        if((((uintptr_t) this) & 15) != 0)
        {
          abort();  // CMPXCHG16B requires 16 byte aligned storage
        }
#if defined(__AVX__)
        _storage c;
        std::atomic_thread_fence(ord);
        c.ssereg = _state.ssereg;
        std::atomic_thread_fence(ord);
        return c.v;
#elif defined(_MSC_VER)
        _storage c, s;
        s.v = c.v = _state.v;
        c.uints[0] = ~c.uints[0];  // try hard to ensure comparator never matches
        _InterlockedCompareExchange128((long long *) _state.uints, s.uints[1], s.uints[0], (long long *) c.uints);
        // If this succeeds, we've overwritten the value written by another thread with an older value, otherwise
        // comparator is atomically loaded with current _state.v
        return c.v;
#elif defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
        _storage c, s;
        s.v = c.v = _state.v;
        c.uints[0] = ~c.uints[0];          // try hard to ensure comparator never matches
        __asm__("lock cmpxchg16b %0"
                : "+m"(*(T *) &_state.v),  // memory to compare
                  "+d"(c.uints[1]),        // comparand high
                  "+a"(c.uints[0])         // comparand low
                : "c"(s.uints[1]),         // replace high
                  "b"(s.uints[0])          // replace low
                : "memory", "cc");         // as memory and flags get modified
        // Comparator is atomically loaded with current _state.v
        return c.v;
#elif defined(__aarch64__) || defined(_M_ARM64)
        // Verified on godbolt that this "does the right thing" https://godbolt.org/z/Kz36d9en4
        _storage c;
        c.u128 = _state.u128atomic.load(ord);
        return c.v;
#else
        // Note that some toolchains are broken in auto linking in __atomic_compare_exchange_16 :(
#define QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_NEEDS_LIBATOMIC 1
        _storage c, s;
        s.v = c.v = _state.v;
        c.uints[0] = ~c.uints[0];  // try hard to ensure comparator never matches
        const_cast<fast_atomic_128 *>(this)->_state.u128atomic.compare_exchange_strong(c.u128, s.u128, ord,
                                                                                       std::memory_order_relaxed);
        return c.v;
#endif
      }
      void store(T with, std::memory_order ord) noexcept
      {
        (void) ord;
        if((((uintptr_t) this) & 15) != 0)
        {
          abort();  // CMPXCHG16B requires 16 byte aligned storage
        }
#if defined(__AVX__)
        _storage s(with);
        std::atomic_thread_fence(ord);
        _state.ssereg = s.ssereg;
        std::atomic_thread_fence(ord);
#elif defined(_MSC_VER)
        _storage c, s;
        s.v = with;
        int done;
        do
        {
          c.v = _state.v;
          done =
          _InterlockedCompareExchange128((long long *) _state.uints, s.uints[1], s.uints[0], (long long *) c.uints);
        } while(!done);
#elif defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
        _storage c, s;
        s.v = with;
        int done;
        do
        {
          c.v = _state.v;
          __asm__("lock cmpxchg16b %0"
                  : "+m"(_state.v),    // memory to compare
                    "=@ccz"(done),     // ZF set if replace occurred, ZF cleared if comparand replaced with memory
                    "+d"(c.uints[1]),  // comparand high
                    "+a"(c.uints[0])   // comparand low
                  : "c"(s.uints[1]),   // replace high
                    "b"(s.uints[0])    // replace low
                  : "memory");         // as memory gets modified
        } while(!done);
#elif defined(__aarch64__) || defined(_M_ARM64)
        // Verified on godbolt that this "does the right thing" https://godbolt.org/z/Kz36d9en4
        _storage s(with);
        _state.u128atomic.store(s.u128, ord);
#else
#define QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_NEEDS_LIBATOMIC 1
        // Note that some toolchains are broken in auto linking in __atomic_compare_exchange_16 :(
        _storage c, s;
        s.v = with;
        do
        {
          c.v = _state.v;
        } while(!_state.u128atomic.compare_exchange_weak(c.u128, s.u128, ord, std::memory_order_relaxed));
#endif
      }
      bool compare_exchange_weak(T &expected, T desired, std::memory_order success, std::memory_order failure) noexcept
      {
        if((((uintptr_t) this) & 15) != 0)
        {
          abort();  // CMPXCHG16B requires 16 byte aligned storage
        }
#ifdef _MSC_VER
        _storage c, s;
        c.v = expected;
        s.v = desired;
        return _InterlockedCompareExchange128((long long *) _state.uints, s.uints[1], s.uints[0],
                                              (long long *) c.uints) != 0;
#elif defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
        _storage c, s;
        c.v = expected;
        s.v = desired;
        int done;
        __asm__("lock cmpxchg16b %0"
                : "+m"(_state.v),    // memory to compare
                  "=@ccz"(done),     // ZF set if replace occurred, ZF cleared if comparand replaced with memory
                  "+d"(c.uints[1]),  // comparand high
                  "+a"(c.uints[0])   // comparand low
                : "c"(s.uints[1]),   // replace high
                  "b"(s.uints[0])    // replace low
                : "memory");         // as memory gets modified
        if(done != 0)
        {
          expected = c.v;
          return true;
        }
        return false;
#elif defined(__aarch64__) || defined(_M_ARM64)
        // Verified on godbolt that this "does the right thing" https://godbolt.org/z/Kz36d9en4
        _storage c, s;
        c.v = expected;
        s.v = desired;
        if(_state.u128atomic.compare_exchange_weak(c.u128, s.u128, success, failure))
        {
          expected = c.v;
          return true;
        }
        return false;
#else
#define QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_NEEDS_LIBATOMIC 1
        // Note that some toolchains are broken in auto linking in __atomic_compare_exchange_16 :(
        _storage c, s;
        c.v = expected;
        s.v = desired;
        if(_state.u128atomic.compare_exchange_weak(c.u128, s.u128, success, failure))
        {
          expected = c.v;
          return true;
        }
        return false;
#endif
      }
      bool compare_exchange_weak(T &expected, T desired, std::memory_order ord = std::memory_order_seq_cst) noexcept
      {
        std::memory_order failure = ord;
        switch(ord)
        {
        case std::memory_order_acq_rel:
          failure = std::memory_order_acquire;
          break;
        case std::memory_order_release:
          failure = std::memory_order_relaxed;
          break;
        default:
          break;
        }
        return compare_exchange_weak(expected, desired, ord, failure);
      }
      bool compare_exchange_strong(T &expected, T desired, std::memory_order success,
                                   std::memory_order failure) noexcept
      {
        if((((uintptr_t) this) & 15) != 0)
        {
          abort();  // CMPXCHG16B requires 16 byte aligned storage
        }
#ifdef _MSC_VER
        (void) success;
        (void) failure;
        _storage c, s;
        c.v = expected;
        s.v = desired;
        return _InterlockedCompareExchange128((long long *) _state.uints, s.uints[1], s.uints[0],
                                              (long long *) c.uints) != 0;
#elif defined(__x86_64__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16)
        _storage c, s;
        c.v = expected;
        s.v = desired;
        int done;
        __asm__("lock cmpxchg16b %0"
                : "+m"(_state.v),    // memory to compare
                  "=@ccz"(done),     // ZF set if replace occurred, ZF cleared if comparand replaced with memory
                  "+d"(c.uints[1]),  // comparand high
                  "+a"(c.uints[0])   // comparand low
                : "c"(s.uints[1]),   // replace high
                  "b"(s.uints[0])    // replace low
                : "memory");         // as memory gets modified
        if(done != 0)
        {
          expected = c.v;
          return true;
        }
        return false;
#elif defined(__aarch64__) || defined(_M_ARM64)
        // Verified on godbolt that this "does the right thing" https://godbolt.org/z/Kz36d9en4
        _storage c, s;
        c.v = expected;
        s.v = desired;
        if(_state.u128atomic.compare_exchange_strong(c.u128, s.u128, success, failure))
        {
          expected = c.v;
          return true;
        }
        return false;
#else
#define QUICKCPPLIB_ALGORITHM_FAST_ATOMIC_128_NEEDS_LIBATOMIC 1
        // Note that some toolchains are broken in auto linking in __atomic_compare_exchange_16 :(
        _storage c, s;
        c.v = expected;
        s.v = desired;
        if(_state.u128atomic.compare_exchange_strong(c.u128, s.u128, success, failure))
        {
          expected = c.v;
          return true;
        }
        return false;
#endif
      }
      bool compare_exchange_strong(T &expected, T desired, std::memory_order ord = std::memory_order_seq_cst) noexcept
      {
        std::memory_order failure = ord;
        switch(ord)
        {
        case std::memory_order_acq_rel:
          failure = std::memory_order_acquire;
          break;
        case std::memory_order_release:
          failure = std::memory_order_relaxed;
          break;
        default:
          break;
        }
        return compare_exchange_strong(expected, desired, ord, failure);
      }
    };
  }  // namespace fast_atomic_128
}  // namespace algorithm

QUICKCPPLIB_NAMESPACE_END

#endif
