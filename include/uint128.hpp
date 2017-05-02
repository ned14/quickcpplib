/* 128 bit integer support
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (4 commits)
File Created: Sept 2016


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

#ifndef BOOSTLITE_UINT128_HPP
#define BOOSTLITE_UINT128_HPP

#include "config.hpp"

#if defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>  // for __m128i on VS2017
#endif

BOOSTLITE_NAMESPACE_BEGIN

namespace integers128
{
  /*! \union uint128
  \brief An unsigned 128 bit value
  */
  union alignas(16) uint128 {
    unsigned char as_bytes[16];
    unsigned short as_shorts[8];
    unsigned int as_ints[4];
    unsigned long long as_longlongs[2];
    // Strongly hint to the compiler what to do here
#if defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    __m128i as_m128i;
#endif
#if defined(__GNUC__) || defined(__clang__)
    typedef unsigned uint32_4_t __attribute__ ((vector_size (16)));
    uint32_4_t as_uint32_4;
#endif
    //! Default constructor, no bits set
    uint128() noexcept {}
    //! All bits zero constructor
    constexpr uint128(std::nullptr_t) noexcept : as_longlongs{0, 0} {}
  private:
    static const uint128 &_allbitszero()
    {
      static uint128 v(nullptr);
      return v;
    }

  public:
    explicit operator bool() const noexcept { return (*this) != _allbitszero(); }
    bool operator!() const noexcept { return (*this) == _allbitszero(); }
    bool operator==(const uint128 &o) const noexcept { return as_longlongs[1] == o.as_longlongs[1] && as_longlongs[0] == o.as_longlongs[0]; }
    bool operator!=(const uint128 &o) const noexcept { return as_longlongs[1] != o.as_longlongs[1] || as_longlongs[0] != o.as_longlongs[0]; }
    bool operator<(const uint128 &o) const noexcept { return as_longlongs[0] < o.as_longlongs[0] || (as_longlongs[0] == o.as_longlongs[0] && as_longlongs[1] < o.as_longlongs[1]); }
    bool operator<=(const uint128 &o) const noexcept { return as_longlongs[0] < o.as_longlongs[0] || (as_longlongs[0] == o.as_longlongs[0] && as_longlongs[1] <= o.as_longlongs[1]); }
    bool operator>(const uint128 &o) const noexcept { return as_longlongs[0] > o.as_longlongs[0] || (as_longlongs[0] == o.as_longlongs[0] && as_longlongs[1] > o.as_longlongs[1]); }
    bool operator>=(const uint128 &o) const noexcept { return as_longlongs[0] > o.as_longlongs[0] || (as_longlongs[0] == o.as_longlongs[0] && as_longlongs[1] >= o.as_longlongs[1]); }
  };
  static_assert(sizeof(uint128) == 16, "uint128 is not 16 bytes long!");
  static_assert(alignof(uint128) == 16, "uint128 is not aligned to 16 byte multiples!");
}

BOOSTLITE_NAMESPACE_END

#endif
