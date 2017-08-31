/* Hash algorithms
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (6 commits)
File Created: Aug 2016


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

#ifndef QUICKCPPLIB_ALGORITHM_HASH_HPP
#define QUICKCPPLIB_ALGORITHM_HASH_HPP

#include "../uint128.hpp"

#include <string.h>  // for memcpy

QUICKCPPLIB_NAMESPACE_BEGIN

namespace algorithm
{
  namespace hash
  {
    //! \brief A STL compatible hash which passes through its input
    template <class T> struct passthru_hash
    {
      size_t operator()(T v) const { return static_cast<size_t>(v); }
    };

    //! \brief A STL compatible hash based on the high quality FNV1 hash algorithm
    template <class T> struct fnv1a_hash
    {
      size_t operator()(T v) const
      {
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__ia64__) || defined(_M_IA64) || defined(__ppc64__)
        static constexpr size_t basis = 14695981039346656037ULL, prime = 1099511628211ULL;
        static_assert(sizeof(size_t) == 8, "size_t is not 64 bit");
#else
        static constexpr size_t basis = 2166136261U, prime = 16777619U;
        static_assert(sizeof(size_t) == 4, "size_t is not 32 bit");
#endif
        const unsigned char *_v = (const unsigned char *) &v;
        size_t ret = basis;
        for(size_t n = 0; n < sizeof(T); n++)
        {
          ret ^= (size_t) _v[n];
          ret *= prime;
        }
        return ret;
      }
    };

    /*! \class fast_hash
    \brief Fast very collision resistant uint128 hash. Currently SpookyHash @ 0.3 cycles/byte.
    */
    class fast_hash
    {
      using uint8 = unsigned char;
      using uint64 = unsigned long long;
      using uint128 = integers128::uint128;

      // number of uint64's in internal state
      static constexpr size_t sc_numVars = 12;

      // size of the internal state
      static constexpr size_t sc_blockSize = sc_numVars * 8;

      // size of buffer of unhashed data, in bytes
      static constexpr size_t sc_bufSize = 2 * sc_blockSize;

      //
      // sc_const: a constant which:
      //  * is not zero
      //  * is odd
      //  * is a not-very-regular mix of 1's and 0's
      //  * does not need any other special mathematical properties
      //
      static constexpr uint64 sc_const = 0xdeadbeefdeadbeefULL;

      uint64 m_data[2 * sc_numVars];  // unhashed data, for partial messages
      uint64 m_state[sc_numVars];     // internal state of the hash
      size_t m_length;                // total length of the input so far
      uint8 m_remainder;              // length of unhashed data stashed in m_data

      static inline void short_(uint128 &hash, const void *data, size_t bytes) noexcept;

    public:
      //! Initialise the hash with an optional seed
      constexpr fast_hash(const uint128 &seed = 0) noexcept : m_data{0}, m_state{seed.as_longlongs[0], seed.as_longlongs[1]}, m_length(0), m_remainder(0) {}

      //! Hash input
      inline void add(const char *data, size_t bytes) noexcept;

      //! Finalise and return hash
      inline uint128 finalise() noexcept;

      //! Single shot hash of a sequence of bytes
      static inline uint128 hash(const char *data, size_t bytes, const uint128 &seed = 0) noexcept;

      //! Single shot hash of a span
      //      template <typename T> static inline uint128 hash(const span::span<T> &str) noexcept { return hash((char *) str.data(), str.size() * sizeof(T)); }
    };

    namespace fash_hash_detail
    {
      using uint8 = unsigned char;
      using uint32 = unsigned int;
      using uint64 = unsigned long long;
      using uint128 = integers128::uint128;
      static constexpr bool ALLOW_UNALIGNED_READS = true;

      static inline QUICKCPPLIB_FORCEINLINE uint64 Rot64(uint64 x, int k) { return (x << k) | (x >> (64 - k)); }
      static inline QUICKCPPLIB_FORCEINLINE void Mix(const uint64 *data, uint64 &s0, uint64 &s1, uint64 &s2, uint64 &s3, uint64 &s4, uint64 &s5, uint64 &s6, uint64 &s7, uint64 &s8, uint64 &s9, uint64 &s10, uint64 &s11)
      {
        s0 += data[0];
        s2 ^= s10;
        s11 ^= s0;
        s0 = Rot64(s0, 11);
        s11 += s1;
        s1 += data[1];
        s3 ^= s11;
        s0 ^= s1;
        s1 = Rot64(s1, 32);
        s0 += s2;
        s2 += data[2];
        s4 ^= s0;
        s1 ^= s2;
        s2 = Rot64(s2, 43);
        s1 += s3;
        s3 += data[3];
        s5 ^= s1;
        s2 ^= s3;
        s3 = Rot64(s3, 31);
        s2 += s4;
        s4 += data[4];
        s6 ^= s2;
        s3 ^= s4;
        s4 = Rot64(s4, 17);
        s3 += s5;
        s5 += data[5];
        s7 ^= s3;
        s4 ^= s5;
        s5 = Rot64(s5, 28);
        s4 += s6;
        s6 += data[6];
        s8 ^= s4;
        s5 ^= s6;
        s6 = Rot64(s6, 39);
        s5 += s7;
        s7 += data[7];
        s9 ^= s5;
        s6 ^= s7;
        s7 = Rot64(s7, 57);
        s6 += s8;
        s8 += data[8];
        s10 ^= s6;
        s7 ^= s8;
        s8 = Rot64(s8, 55);
        s7 += s9;
        s9 += data[9];
        s11 ^= s7;
        s8 ^= s9;
        s9 = Rot64(s9, 54);
        s8 += s10;
        s10 += data[10];
        s0 ^= s8;
        s9 ^= s10;
        s10 = Rot64(s10, 22);
        s9 += s11;
        s11 += data[11];
        s1 ^= s9;
        s10 ^= s11;
        s11 = Rot64(s11, 46);
        s10 += s0;
      }
      static inline QUICKCPPLIB_FORCEINLINE void EndPartial(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3, uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, uint64 &h8, uint64 &h9, uint64 &h10, uint64 &h11)
      {
        h11 += h1;
        h2 ^= h11;
        h1 = Rot64(h1, 44);
        h0 += h2;
        h3 ^= h0;
        h2 = Rot64(h2, 15);
        h1 += h3;
        h4 ^= h1;
        h3 = Rot64(h3, 34);
        h2 += h4;
        h5 ^= h2;
        h4 = Rot64(h4, 21);
        h3 += h5;
        h6 ^= h3;
        h5 = Rot64(h5, 38);
        h4 += h6;
        h7 ^= h4;
        h6 = Rot64(h6, 33);
        h5 += h7;
        h8 ^= h5;
        h7 = Rot64(h7, 10);
        h6 += h8;
        h9 ^= h6;
        h8 = Rot64(h8, 13);
        h7 += h9;
        h10 ^= h7;
        h9 = Rot64(h9, 38);
        h8 += h10;
        h11 ^= h8;
        h10 = Rot64(h10, 53);
        h9 += h11;
        h0 ^= h9;
        h11 = Rot64(h11, 42);
        h10 += h0;
        h1 ^= h10;
        h0 = Rot64(h0, 54);
      }

      static inline QUICKCPPLIB_FORCEINLINE void End(const uint64 *data, uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3, uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, uint64 &h8, uint64 &h9, uint64 &h10, uint64 &h11)
      {
        h0 += data[0];
        h1 += data[1];
        h2 += data[2];
        h3 += data[3];
        h4 += data[4];
        h5 += data[5];
        h6 += data[6];
        h7 += data[7];
        h8 += data[8];
        h9 += data[9];
        h10 += data[10];
        h11 += data[11];
        EndPartial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        EndPartial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        EndPartial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
      }

      static inline QUICKCPPLIB_FORCEINLINE void ShortMix(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
      {
        h2 = Rot64(h2, 50);
        h2 += h3;
        h0 ^= h2;
        h3 = Rot64(h3, 52);
        h3 += h0;
        h1 ^= h3;
        h0 = Rot64(h0, 30);
        h0 += h1;
        h2 ^= h0;
        h1 = Rot64(h1, 41);
        h1 += h2;
        h3 ^= h1;
        h2 = Rot64(h2, 54);
        h2 += h3;
        h0 ^= h2;
        h3 = Rot64(h3, 48);
        h3 += h0;
        h1 ^= h3;
        h0 = Rot64(h0, 38);
        h0 += h1;
        h2 ^= h0;
        h1 = Rot64(h1, 37);
        h1 += h2;
        h3 ^= h1;
        h2 = Rot64(h2, 62);
        h2 += h3;
        h0 ^= h2;
        h3 = Rot64(h3, 34);
        h3 += h0;
        h1 ^= h3;
        h0 = Rot64(h0, 5);
        h0 += h1;
        h2 ^= h0;
        h1 = Rot64(h1, 36);
        h1 += h2;
        h3 ^= h1;
      }

      static inline QUICKCPPLIB_FORCEINLINE void ShortEnd(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
      {
        h3 ^= h2;
        h2 = Rot64(h2, 15);
        h3 += h2;
        h0 ^= h3;
        h3 = Rot64(h3, 52);
        h0 += h3;
        h1 ^= h0;
        h0 = Rot64(h0, 26);
        h1 += h0;
        h2 ^= h1;
        h1 = Rot64(h1, 51);
        h2 += h1;
        h3 ^= h2;
        h2 = Rot64(h2, 28);
        h3 += h2;
        h0 ^= h3;
        h3 = Rot64(h3, 9);
        h0 += h3;
        h1 ^= h0;
        h0 = Rot64(h0, 47);
        h1 += h0;
        h2 ^= h1;
        h1 = Rot64(h1, 54);
        h2 += h1;
        h3 ^= h2;
        h2 = Rot64(h2, 32);
        h3 += h2;
        h0 ^= h3;
        h3 = Rot64(h3, 25);
        h0 += h3;
        h1 ^= h0;
        h0 = Rot64(h0, 63);
        h1 += h0;
      }
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)  // use of ALLOW_UNALIGNED_READS
#endif

    inline void fast_hash::short_(uint128 &hash, const void *message, size_t length) noexcept
    {
      using namespace fash_hash_detail;
      uint64 buf[2 * sc_numVars];
      union {
        const uint8 *p8;
        uint32 *p32;
        uint64 *p64;
        size_t i;
      } u;

      u.p8 = (const uint8 *) message;

      if(!ALLOW_UNALIGNED_READS && (u.i & 0x7))
      {
        memcpy(buf, message, length);
        u.p64 = buf;
      }

      size_t remainder = length % 32;
      uint64 a = hash.as_longlongs[0];
      uint64 b = hash.as_longlongs[1];
      uint64 c = sc_const;
      uint64 d = sc_const;

      if(length > 15)
      {
        const uint64 *end = u.p64 + (length / 32) * 4;

        // handle all complete sets of 32 bytes
        for(; u.p64 < end; u.p64 += 4)
        {
          c += u.p64[0];
          d += u.p64[1];
          ShortMix(a, b, c, d);
          a += u.p64[2];
          b += u.p64[3];
        }

        // Handle the case of 16+ remaining bytes.
        if(remainder >= 16)
        {
          c += u.p64[0];
          d += u.p64[1];
          ShortMix(a, b, c, d);
          u.p64 += 2;
          remainder -= 16;
        }
      }

      // Handle the last 0..15 bytes, and its length
      d += ((uint64) length) << 56;
      switch(remainder)
      {
      case 15:
        d += ((uint64) u.p8[14]) << 48;
      // fallthrough
      case 14:
        d += ((uint64) u.p8[13]) << 40;
      // fallthrough
      case 13:
        d += ((uint64) u.p8[12]) << 32;
      // fallthrough
      case 12:
        d += u.p32[2];
        c += u.p64[0];
        break;
      case 11:
        d += ((uint64) u.p8[10]) << 16;
      // fallthrough
      case 10:
        d += ((uint64) u.p8[9]) << 8;
      // fallthrough
      case 9:
        d += (uint64) u.p8[8];
      // fallthrough
      case 8:
        c += u.p64[0];
        break;
      case 7:
        c += ((uint64) u.p8[6]) << 48;
      // fallthrough
      case 6:
        c += ((uint64) u.p8[5]) << 40;
      // fallthrough
      case 5:
        c += ((uint64) u.p8[4]) << 32;
      // fallthrough
      case 4:
        c += u.p32[0];
        break;
      case 3:
        c += ((uint64) u.p8[2]) << 16;
      // fallthrough
      case 2:
        c += ((uint64) u.p8[1]) << 8;
      // fallthrough
      case 1:
        c += (uint64) u.p8[0];
        break;
      case 0:
        c += sc_const;
        d += sc_const;
      }
      ShortEnd(a, b, c, d);
      hash.as_longlongs[0] = a;
      hash.as_longlongs[1] = b;
    }

    inline void fast_hash::add(const char *message, size_t length) noexcept
    {
      using namespace fash_hash_detail;
      uint64 h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11;
      size_t newLength = length + m_remainder;
      uint8 remainder;
      union {
        const uint8 *p8;
        uint64 *p64;
        size_t i;
      } u;
      const uint64 *end;

      // Is this message fragment too short?  If it is, stuff it away.
      if(newLength < sc_bufSize)
      {
        memcpy(&((uint8 *) m_data)[m_remainder], message, length);
        m_length = length + m_length;
        m_remainder = (uint8) newLength;
        return;
      }

      // init the variables
      if(m_length < sc_bufSize)
      {
        h0 = h3 = h6 = h9 = m_state[0];
        h1 = h4 = h7 = h10 = m_state[1];
        h2 = h5 = h8 = h11 = sc_const;
      }
      else
      {
        h0 = m_state[0];
        h1 = m_state[1];
        h2 = m_state[2];
        h3 = m_state[3];
        h4 = m_state[4];
        h5 = m_state[5];
        h6 = m_state[6];
        h7 = m_state[7];
        h8 = m_state[8];
        h9 = m_state[9];
        h10 = m_state[10];
        h11 = m_state[11];
      }
      m_length = length + m_length;

      // if we've got anything stuffed away, use it now
      if(m_remainder)
      {
        uint8 prefix = sc_bufSize - m_remainder;
        memcpy(&(((uint8 *) m_data)[m_remainder]), message, prefix);
        u.p64 = m_data;
        Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        Mix(&u.p64[sc_numVars], h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        u.p8 = ((const uint8 *) message) + prefix;
        length -= prefix;
      }
      else
      {
        u.p8 = (const uint8 *) message;
      }

      // handle all whole blocks of sc_blockSize bytes
      end = u.p64 + (length / sc_blockSize) * sc_numVars;
      remainder = (uint8)(length - ((const uint8 *) end - u.p8));
      if(ALLOW_UNALIGNED_READS || (u.i & 0x7) == 0)
      {
        while(u.p64 < end)
        {
          Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
          u.p64 += sc_numVars;
        }
      }
      else
      {
        while(u.p64 < end)
        {
          memcpy(m_data, u.p8, sc_blockSize);
          Mix(m_data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
          u.p64 += sc_numVars;
        }
      }

      // stuff away the last few bytes
      m_remainder = remainder;
      memcpy(m_data, end, remainder);

      // stuff away the variables
      m_state[0] = h0;
      m_state[1] = h1;
      m_state[2] = h2;
      m_state[3] = h3;
      m_state[4] = h4;
      m_state[5] = h5;
      m_state[6] = h6;
      m_state[7] = h7;
      m_state[8] = h8;
      m_state[9] = h9;
      m_state[10] = h10;
      m_state[11] = h11;
    }

    inline fast_hash::uint128 fast_hash::finalise() noexcept
    {
      using namespace fash_hash_detail;
      uint128 ret;
      // init the variables
      if(m_length < sc_bufSize)
      {
        ret.as_longlongs[0] = m_state[0];
        ret.as_longlongs[1] = m_state[1];
        short_(ret, m_data, m_length);
        return ret;
      }

      const uint64 *data = (const uint64 *) m_data;
      uint8 remainder = m_remainder;

      uint64 h0 = m_state[0];
      uint64 h1 = m_state[1];
      uint64 h2 = m_state[2];
      uint64 h3 = m_state[3];
      uint64 h4 = m_state[4];
      uint64 h5 = m_state[5];
      uint64 h6 = m_state[6];
      uint64 h7 = m_state[7];
      uint64 h8 = m_state[8];
      uint64 h9 = m_state[9];
      uint64 h10 = m_state[10];
      uint64 h11 = m_state[11];

      if(remainder >= sc_blockSize)
      {
        // m_data can contain two blocks; handle any whole first block
        Mix(data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
        data += sc_numVars;
        remainder -= sc_blockSize;
      }

      // mix in the last partial block, and the length mod sc_blockSize
      memset(&((uint8 *) data)[remainder], 0, (sc_blockSize - remainder));

      ((uint8 *) data)[sc_blockSize - 1] = remainder;

      // do some final mixing
      End(data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);

      ret.as_longlongs[0] = h0;
      ret.as_longlongs[1] = h1;
      return ret;
    }

    inline fast_hash::uint128 fast_hash::hash(const char *message, size_t length, const uint128 &_ret) noexcept
    {
      uint128 ret(_ret);
      using namespace fash_hash_detail;
      if(length < sc_bufSize)
      {
        short_(ret, message, length);
        return ret;
      }

      uint64 h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11;
      uint64 buf[sc_numVars];
      uint64 *end;
      union {
        const uint8 *p8;
        uint64 *p64;
        size_t i;
      } u;
      size_t remainder;

      h0 = h3 = h6 = h9 = ret.as_longlongs[0];
      h1 = h4 = h7 = h10 = ret.as_longlongs[1];
      h2 = h5 = h8 = h11 = sc_const;

      u.p8 = (const uint8 *) message;
      end = u.p64 + (length / sc_blockSize) * sc_numVars;

      // handle all whole sc_blockSize blocks of bytes
      if(ALLOW_UNALIGNED_READS || ((u.i & 0x7) == 0))
      {
        while(u.p64 < end)
        {
          Mix(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
          u.p64 += sc_numVars;
        }
      }
      else
      {
        while(u.p64 < end)
        {
          memcpy(buf, u.p64, sc_blockSize);
          Mix(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
          u.p64 += sc_numVars;
        }
      }

      // handle the last partial block of sc_blockSize bytes
      remainder = (length - ((const uint8 *) end - (const uint8 *) message));
      memcpy(buf, end, remainder);
      memset(((uint8 *) buf) + remainder, 0, sc_blockSize - remainder);
      ((uint8 *) buf)[sc_blockSize - 1] = (uint8) remainder;

      // do some final mixing
      End(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);

      ret.as_longlongs[0] = h0;
      ret.as_longlongs[1] = h1;
      return ret;
    }

#ifdef _MSC_VER
#pragma warning(pop)
#endif
  }
}

QUICKCPPLIB_NAMESPACE_END

#endif
