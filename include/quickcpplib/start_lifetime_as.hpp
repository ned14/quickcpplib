/* bless support
(C) 2019 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Oct 2019


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

#ifndef QUICKCPPLIB_START_LIFETIME_AS_HPP
#define QUICKCPPLIB_START_LIFETIME_AS_HPP

#include "config.hpp"

#include <new>

QUICKCPPLIB_NAMESPACE_BEGIN
namespace start_lifetime_as
{
#if defined(QUICKCPPLIB_USE_STD_START_LIFETIME_AS) || _HAS_CXX23 || __cplusplus >= 202300

  using std::launder;
  using std::start_lifetime_as;

#elif(_HAS_CXX17 || __cplusplus >= 201700)

  using std::launder;
  //! Faked `std::start_lifetime_as<T>` from C++ 23
  template <typename T> QUICKCPPLIB_NODISCARD inline T *start_lifetime_as(void *p) { return reinterpret_cast<T *>(p); }

#else

  //! Faked `std::launder<T>` from C++ 17
  template <class T> QUICKCPPLIB_NODISCARD constexpr inline T *launder(T *p) noexcept { return p; }
  //! Faked `std::start_lifetime_as<T>` from C++ 23
  template <typename T> QUICKCPPLIB_NODISCARD inline T *start_lifetime_as(void *p) { return reinterpret_cast<T *>(p); }

#endif
}  // namespace start_lifetime_as

QUICKCPPLIB_NAMESPACE_END

#endif
