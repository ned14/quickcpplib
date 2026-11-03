/* byte support
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Apr 2018


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

#ifndef QUICKCPPLIB_BYTE_HPP
#define QUICKCPPLIB_BYTE_HPP

#include "config.hpp"

// std::byte is C++17, and there is no pre-C++17 substitute any more: the bundled
// byte-lite copy was deleted when C++17 became quickcpplib's default. C++14 is
// still an explicit opt-in mode (QUICKCPPLIB_MIN_CXX_STANDARD=14) for consumers
// such as Outcome, but those consumers do not use std::byte. So rather than
// silently omit the type in that mode, fail here with one actionable message.
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L

#include <cstddef>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace byte
{
  using std::byte;
  template <class IntegerType> inline constexpr byte to_byte(IntegerType v) noexcept { return static_cast<byte>(v); }
}

QUICKCPPLIB_NAMESPACE_END

#else

#error "quickcpplib/byte.hpp requires C++17 (std::byte). quickcpplib defaults to C++17 and the pre-C++17 byte polyfill has been removed. If you configured the opt-in C++14 mode with QUICKCPPLIB_MIN_CXX_STANDARD=14, do not include this header: mem_flush_loads_stores.hpp and detach_cast.hpp are unavailable in that mode."

#endif

#endif
