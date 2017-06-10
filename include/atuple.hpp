/* Provides a simple aggregate initialisable heterogenous container.
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (5 commits)


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

#ifndef QUICKCPPLIB_ATUPLE_HPP
#define QUICKCPPLIB_ATUPLE_HPP

#include "config.hpp"

#include <tuple>
#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace aggregate_tuple
{
  /*! \brief A C++ 14 aggregate initialisable collection of heterogeneous types
  standing in for C++ 17 tuple.
  \tparam Args An arbitrary list of types

  This is a highly incomplete wrapper of std::tuple. It has one major feature the
  C++ 14 std::tuple does not:
  \code
  constexpr tuple<int, double, const char *> fc[] = {{1, 1.0, "hi1"}, {2, 2.0, "hi2"}, ...};
  \endcode
  */
  template <class... Args> struct tuple : std::tuple<Args...>
  {
    constexpr tuple(tuple &&) = default;
    constexpr tuple(const tuple &) = default;
    //! Implicit conversion from the underlying tuple
    constexpr tuple(std::tuple<Args...> &&o)
        : std::tuple<Args...>(std::move(o))
    {
    }
    //! Implicit conversion from the underlying tuple
    constexpr tuple(const std::tuple<Args...> &o)
        : std::tuple<Args...>(o)
    {
    }
    //! Enables list initialisation of the tuple
    constexpr tuple(Args &&... ts)
        : std::tuple<Args...>(std::move(ts)...)
    {
    }
    //! Enables list initialisation of the tuple
    constexpr tuple(const Args &... ts)
        : std::tuple<Args...>(ts...)
    {
    }
#if 0
      //! Enables list initialisation of the tuple
      template <class... Ts, typename std::enable_if<sizeof...(Ts) == sizeof...(Args), bool>::type = true>
      constexpr tuple(Ts &&... ts)
          : std::tuple<Args...>(std::forward<Ts>(ts)...)
      {
      }
#endif
  };

  using std::make_tuple;
  using std::get;

  template <class T> struct tuple_size;
  template <class... Args> struct tuple_size<tuple<Args...>> : std::tuple_size<std::tuple<Args...>>
  {
  };
  template <class... Args> struct tuple_size<std::tuple<Args...>> : std::tuple_size<std::tuple<Args...>>
  {
  };
  template <class T> struct tuple_size<const T> : tuple_size<T>
  {
  };
  template <class T> struct tuple_size<volatile T> : tuple_size<T>
  {
  };
  template <class T> struct tuple_size<const volatile T> : tuple_size<T>
  {
  };

  template <size_t N, class T> struct tuple_element;
  template <size_t N, class... Args> struct tuple_element<N, tuple<Args...>> : std::tuple_element<N, std::tuple<Args...>>
  {
  };
  template <size_t N, class... Args> struct tuple_element<N, std::tuple<Args...>> : std::tuple_element<N, std::tuple<Args...>>
  {
  };
  template <size_t N, class T> struct tuple_element<N, const T> : tuple_element<N, T>
  {
  };
  template <size_t N, class T> struct tuple_element<N, volatile T> : tuple_element<N, T>
  {
  };
  template <size_t N, class T> struct tuple_element<N, const volatile T> : tuple_element<N, T>
  {
  };

}  // namespace

QUICKCPPLIB_NAMESPACE_END

#endif
