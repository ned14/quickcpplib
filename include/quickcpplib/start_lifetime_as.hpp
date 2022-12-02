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

#include <memory>  // for start_lifetime_as
#include <new>     // for launder
#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN
namespace start_lifetime_as
{
  namespace detail
  {
    using namespace std;
    template <class T> constexpr inline T *launder(T *p, ...) noexcept
    {
      return p;
    }
    template <typename T> inline T *start_lifetime_as(void *p, ...) noexcept
    {
      return reinterpret_cast<T *>(p);
    }
    template <typename T> inline const T *start_lifetime_as(const void *p, ...) noexcept
    {
      return reinterpret_cast<const T *>(p);
    }
    template <typename T> inline T *start_lifetime_as_array(void *p, size_t, ...) noexcept
    {
      return reinterpret_cast<T *>(p);
    }
    template <typename T> inline const T *start_lifetime_as_array(const void *p, size_t, ...) noexcept
    {
      return reinterpret_cast<const T *>(p);
    }

    template <class T> constexpr inline T *_launder(T *p) noexcept
    {
      return launder<T>(p);
    }
    template <class T> struct _start_lifetime_as
    {
      constexpr T *operator()(void *p) const noexcept { return start_lifetime_as<T>(p); }
      constexpr const T *operator()(const void *p) const noexcept { return start_lifetime_as<T>(p); }
      constexpr T *operator()(void *p, size_t n) const noexcept { return start_lifetime_as_array<T>(p, n); }
      constexpr const T *operator()(const void *p, size_t n) const noexcept { return start_lifetime_as_array<T>(p, n); }
    };

#if __cplusplus >= 202600L
    template <class T> using _is_implicit_lifetime = std::is_implicit_lifetime<T>;
#else
    /* Note that this is broken in C++'s without the actual type trait as it's not possible to fully correctly
    implement this. To be specific, aggregates with user provided destructors are reported to have implicit
    lifetime, when they do not. Also, the trait should be true for types with trivial constructors even if
    they're not available to the public, which is undetectable in current C++.
    */
    template <class T> struct _is_implicit_lifetime
    {
      using _type = typename std::remove_cv<T>::type;
      static constexpr bool value = std::is_scalar<_type>::value                                   //
                                    || std::is_array<_type>::value                                 //
#if __cplusplus >= 201700LL || _HAS_CXX17
                                    || std::is_aggregate<_type>::value                             //
#endif
                                    || (std::is_trivially_destructible<_type>::value               //
                                        && (std::is_trivially_default_constructible<_type>::value  //
                                            || std::is_trivially_copy_constructible<_type>::value  //
                                            || std::is_trivially_move_constructible<_type>::value));
    };
#endif
  }  // namespace detail

  //! `std::launder<T>` from C++ 17, or an emulation
  QUICKCPPLIB_TEMPLATE(class T)
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(!std::is_function<T>::value), QUICKCPPLIB_TPRED(!std::is_void<T>::value))
  QUICKCPPLIB_NODISCARD constexpr inline T *launder(T *p) noexcept
  {
    return detail::_launder(p);
  }
  //! `std::start_lifetime_as<T>` from C++ 23, or an emulation
  QUICKCPPLIB_TEMPLATE(class T)
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(detail::_is_implicit_lifetime<T>::value))
  QUICKCPPLIB_NODISCARD constexpr inline auto *start_lifetime_as(void *p) noexcept
  {
    return detail::_start_lifetime_as<T>()(p);
  }
  //! `std::start_lifetime_as<T>` from C++ 23, or an emulation
  QUICKCPPLIB_TEMPLATE(class T)
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(detail::_is_implicit_lifetime<T>::value))
  QUICKCPPLIB_NODISCARD constexpr inline auto *start_lifetime_as(const void *p) noexcept
  {
    return detail::_start_lifetime_as<T>()(p);
  }
  //! `std::start_lifetime_as_array<T>` from C++ 23, or an emulation
  QUICKCPPLIB_TEMPLATE(class T)
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(detail::_is_implicit_lifetime<T>::value))
  QUICKCPPLIB_NODISCARD constexpr inline auto *start_lifetime_as_array(void *p, size_t n) noexcept
  {
    return detail::_start_lifetime_as<T>()(p, n);
  }
  //! `std::start_lifetime_as_array<T>` from C++ 23, or an emulation
  QUICKCPPLIB_TEMPLATE(class T)
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(detail::_is_implicit_lifetime<T>::value))
  QUICKCPPLIB_NODISCARD constexpr inline auto *start_lifetime_as_array(const void *p, size_t n) noexcept
  {
    return detail::_start_lifetime_as<T>()(p, n);
  }

}  // namespace start_lifetime_as

QUICKCPPLIB_NAMESPACE_END

#endif
