/* Interleave and deinterleave bits efficiently
(C) 2023 Niall Douglas <http://www.nedproductions.biz/> (7 commits)
File Created: Jun 2023


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

#ifndef QUICKCPPLIB_ALGORITHM_BIT_INTERLEAVE_HPP
#define QUICKCPPLIB_ALGORITHM_BIT_INTERLEAVE_HPP

#include "../config.hpp"

#include <cstdint>
#include <type_traits>

#if 0 //defined(__AVX__) || defined(__SSE4_1__) || defined(__SSSE3__)
#include <tmmintrin.h>  // for _mm_shuffle_epi8
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace algorithm
{
  namespace bit_interleave
  {
    namespace detail
    {
      template <class T> struct next_larger
      {
        static_assert(
        !std::is_same<T, T>::value,
        "T is not a known type from which to deduce the next larger type for R, please supply the appropriate R type.");
      };
      template <> struct next_larger<uint8_t>
      {
        using type = uint16_t;
      };
      template <> struct next_larger<uint16_t>
      {
        using type = uint32_t;
      };
      template <> struct next_larger<uint32_t>
      {
        using type = uint64_t;
      };
      template <class T> struct next_smaller
      {
        static_assert(!std::is_same<T, T>::value,
                      "T is not a known type from which to deduce the next smaller type for "
                      "R, please supply the appropriate R type.");
      };
      template <> struct next_smaller<uint64_t>
      {
        using type = uint32_t;
      };
      template <> struct next_smaller<uint32_t>
      {
        using type = uint16_t;
      };
      template <> struct next_smaller<uint16_t>
      {
        using type = uint8_t;
      };
    }  // namespace detail

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)  // conditional expression is constant
#pragma warning(disable : 4310)  // cast truncates constant value
#pragma warning(disable : 4333)  // right shift by too large amount
#endif
    /*! \brief Interleaves the bits of \emph a and \emph b.

    On my Intel i7-8565u laptop able to boost to 4.6Ghz:

    Straight C edition on MSVC: 4.8052e+08  32-bit to 64-bit interleaves/sec which is 2.08108ns/interleave (9.57 cycles)
     Straight C edition on GCC: 3.39213e+08 32-bit to 64-bit interleaves/sec which is 2.948ns/interleave  (13.56 cycles)
    */
    QUICKCPPLIB_TEMPLATE(class T, class R = typename detail::next_larger<T>::type)
    QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_unsigned<T>::value))
    inline R bit_interleave(T a, T b) noexcept
    {
#if 0 // defined(__AVX__) || defined(__SSE4_1__) || defined(__SSSE3__)
      /* https://lemire.me/blog/2018/01/09/how-fast-can-you-bit-interleave-32-bit-integers-simd-edition/
      says that AVX is considerably faster than the SSSE3 bit interleave if you need to interleave two 128
      bit values into a 256 bit value, but we don't support that here yet.

      PDEP has a 19 cycle latency on most AMD CPUs, the C fallback is considerably quicker.
      */
#else
      // Standard C fallback which processes both halves in parallel to leverage superscalar execution
      R ret1 = R(a), ret2 = R(b);
      constexpr R mask16 = R(0x0000ffff0000ffff), mask8 = R(0x00ff00ff00ff00ff) /* 0000 0000 1111 1111 */,
                  mask4 = R(0x0f0f0f0f0f0f0f0f) /* 0000 1111 */, mask2 = R(0x3333333333333333) /* 0011 0011 */,
                  mask1 = R(0x5555555555555555) /* 0101 0101 */;
      if(sizeof(T) >= 4)
      {
        ret1 = (ret1 ^ (ret1 << 16)) & mask16;
        ret2 = (ret2 ^ (ret2 << 16)) & mask16;
      }
      if(sizeof(T) >= 2)
      {
        ret1 = (ret1 ^ (ret1 << 8)) & mask8;
        ret2 = (ret2 ^ (ret2 << 8)) & mask8;
      }
      ret1 = (ret1 ^ (ret1 << 4)) & mask4;
      ret2 = (ret2 ^ (ret2 << 4)) & mask4;
      ret1 = (ret1 ^ (ret1 << 2)) & mask2;
      ret2 = (ret2 ^ (ret2 << 2)) & mask2;
      ret1 = (ret1 ^ (ret1 << 1)) & mask1;
      ret2 = (ret2 ^ (ret2 << 1)) & mask1;
      return ret1 | (ret2 << 1);
#endif
    }

    template <class T> struct bit_deinterleave_result
    {
      T evens;  //!< The even bits of the input
      T odds;   //!< The odd bits of the input
    };
    //! \brief Deinterleaves the bits in `X` into `evens` and `odds`.
    QUICKCPPLIB_TEMPLATE(class T, class R = typename detail::next_smaller<T>::type)
    QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_unsigned<T>::value))
    inline bit_deinterleave_result<R> bit_deinterleave(T x) noexcept
    {
      constexpr T /* mask32 = T(0x00000000ffffffff), */ mask16 = T(0x0000ffff0000ffff),
                  mask8 = T(0x00ff00ff00ff00ff) /* 0000 0000 1111 1111 */,
                  mask4 = T(0x0f0f0f0f0f0f0f0f) /* 0000 1111 */, mask2 = T(0x3333333333333333) /* 0011 0011 */,
                  mask1 = T(0x5555555555555555) /* 0101 0101 */;
      T ret1 = x & mask1, ret2 = (x >> 1) & mask1;
      ret1 = (ret1 ^ (ret1 >> 1)) & mask2;
      ret2 = (ret2 ^ (ret2 >> 1)) & mask2;
      ret1 = (ret1 ^ (ret1 >> 2)) & mask4;
      ret2 = (ret2 ^ (ret2 >> 2)) & mask4;
      if(sizeof(T) >= 2)
      {
        ret1 = (ret1 ^ (ret1 >> 4)) & mask8;
        ret2 = (ret2 ^ (ret2 >> 4)) & mask8;
      }
      if(sizeof(T) >= 4)
      {
        ret1 = (ret1 ^ (ret1 >> 8)) & mask16;
        ret2 = (ret2 ^ (ret2 >> 8)) & mask16;
      }
      if(sizeof(T) >= 8)
      {
        ret1 = (ret1 ^ (ret1 >> 16)) /*& mask32*/;
        ret2 = (ret2 ^ (ret2 >> 16)) /*& mask32*/;
      }
      return bit_deinterleave_result<R>{R(ret1), R(ret2)};
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  }  // namespace bit_interleave
}  // namespace algorithm

QUICKCPPLIB_NAMESPACE_END

#endif
