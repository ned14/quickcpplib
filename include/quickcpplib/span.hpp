/* span support
(C) 2016-2023 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
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

#ifndef QUICKCPPLIB_SPAN_HPP
#define QUICKCPPLIB_SPAN_HPP

// `_HAS_CXX20` is defined in `<vcruntime.h>`, i.e. we need to drag it in (indirectly).
// `<cstddef>` is inexpensive to include and would be included for `std::size_t` anyways.
// It is also guaranteed to include the MS STL versioning machinery for `std::byte`.
#include <cstddef>

#include "config.hpp"

#if !defined(QUICKCPPLIB_USE_STD_SPAN)
#if(_HAS_CXX20 || __cplusplus >= 202002L) && __cpp_lib_span >= 202002L
#define QUICKCPPLIB_USE_STD_SPAN 1
#else
#define QUICKCPPLIB_USE_STD_SPAN 0
#endif
#endif

#if QUICKCPPLIB_USE_STD_SPAN

#include "declval.hpp"

#include <span>
#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace span
{
#if QUICKCPPLIB_USE_UNPATCHED_STD_SPAN
  template <class T, size_t Extent = std::dynamic_extent> using span = std::span<T, Extent>;
#else
  template <class T, size_t Extent = std::dynamic_extent> class span : public std::span<T, Extent>
  {
    using _base = std::span<T, Extent>;
    using _const_base = std::span<const T, Extent>;

  public:
    using _base::_base;
    constexpr span(_base v) noexcept
        : _base(v)
    {
    }

    // libc++ incorrectly makes the range consuming constructor explicit which breaks all implicit
    // construction of spans from vectors. Let's add a constructor to fix that, even though it'll
    // break double implicit conversions :(
    template <class U>
      requires(
      !std::is_same_v<std::decay_t<U>, span> && !std::is_same_v<std::decay_t<U>, _base> &&
      !std::is_same_v<std::decay_t<U>, _const_base> &&
      std::is_convertible<typename std::decay_t<U>::pointer, typename _base::pointer>::value &&
      requires { declval<U>().data(); } && requires { declval<U>().size(); })
    constexpr span(U &&v) noexcept
        : _base(v.data(), v.size())
    {
    }
  };
#endif
}  // namespace span

#undef QUICKCPPLIB_USE_STD_SPAN
#define QUICKCPPLIB_USE_STD_SPAN 1

QUICKCPPLIB_NAMESPACE_END

#else  // ^^^ QUICKCPPLIB_USE_STD_SPAN / not QUICKCPPLIB_USE_STD_SPAN vvv

#include <exception>  // for std::terminate()

#if QUICKCPPLIB_USE_SYSTEM_SPAN_LITE
#include <nonstd/span.hpp>
#else
#include "span-lite/include/nonstd/span.hpp"
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace span
{
  template <class T> using span = nonstd::span<T>;
}

QUICKCPPLIB_NAMESPACE_END

#endif  // ^^^ not QUICKCPPLIB_USE_STD_SPAN ^^^

#endif
