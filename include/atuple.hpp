/* atuple.hpp
Provides a simple aggregate initialisable heterogenous container.
(C) 2016 Niall Douglas http://www.nedproductions.biz
*/

#ifndef BOOSTLITE_ATUPLE_HPP
#define BOOSTLITE_ATUPLE_HPP

#include <tuple>
#include <type_traits>

namespace boost_lite
{
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
      template <class... Ts, typename std::enable_if<sizeof...(Ts) == sizeof...(Args), bool>::type = true>
      constexpr tuple(Ts &&... ts)
          : std::tuple<Args...>(std::forward<Ts>(ts)...)
      {
      }
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
}  // namespace
#endif
