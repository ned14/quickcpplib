/* atuple.hpp
Provides a simple aggregate initialisable heterogenous container.
(C) 2016 Niall Douglas http://www.nedproductions.biz
*/

#ifndef BOOSTLITE_ATUPLE_HPP
#define BOOSTLITE_ATUPLE_HPP

#include <type_traits>
#include <utility>

namespace boost_lite
{
  namespace aggregate_tuple
  {
    /*! \brief A C++ 14 aggregate initialisable collection of heterogeneous types
    standing in for C++ 17 tuple.
    \tparam Args An arbitrary list of types

    \note C++ does not allow aggregate initialisable collections to have non-default
    constructors, so you cannot construct this directly. Use make_tuple().

    This is a highly incomplete and inefficient implementation of std::tuple.
    However it has one major feature the C++ 14 std::tuple does not:
    \code
    constexpr tuple<int, double, const char *> fc[] = {{1, 1.0, "hi1"}, {2, 2.0, "hi2"}, ...};
    \endcode
    Missing major features include tie(), forward_as_tuple() and tuple_cat(), and
    lots of smaller features.
    */
    template <class... Args> struct tuple;
    template <class T, class... Args> struct tuple<T, Args...>
    {
      // Can't be private if it is to be aggregate initialisable
      T _value;
      tuple<Args...> _rest;
//! True only if entire collection is nothrow swappable
#if __cplusplus >= 20170000L
      static constexpr const bool is_nothrow_swappable = std::is_nothrow_swappable<T>::value && tuple<Args...>::is_nothrow_swappable;
#else
      static constexpr const bool is_nothrow_swappable = std::is_nothrow_move_constructible<T>::value && tuple<Args...>::is_nothrow_swappable;
#endif

      //! Default constructor
      constexpr tuple() = default;
      //! Swaps this with another instance. Only available if is_nothrow_swappable.
      template <class U, typename V = typename std::enable_if<std::is_same<U, tuple>::value && is_nothrow_swappable, U>::type> void swap(V &o) noexcept
      {
        std::swap(_value, o._value);
        _rest.swap(o._rest);
      }
    };
    template <> struct tuple<>
    {
      static constexpr const bool is_nothrow_swappable = true;
      void swap(tuple &) noexcept {}
    };

    namespace detail
    {
      template <size_t N, class T, class... Args> struct get
      {
        constexpr auto &&operator()(tuple<T, Args...> &&f) const { return get<N - 1, Args...>()(std::move(f._rest)); }
        constexpr auto &operator()(tuple<T, Args...> &f) const { return get<N - 1, Args...>()(f._rest); }
        constexpr const auto &operator()(const tuple<T, Args...> &f) const { return get<N - 1, Args...>()(f._rest); }
      };
      template <class T, class... Args> struct get<0, T, Args...>
      {
        constexpr T &&operator()(tuple<T, Args...> &&f) const { return std::move(f._value); }
        constexpr T &operator()(tuple<T, Args...> &f) const { return f._value; }
        constexpr const T &operator()(const tuple<T, Args...> &f) const { return f._value; }
      };
    }

/*! \brief Makes an tuple
\tparam Args Deduced from the parameter
\return A tuple
\param args The types and values for the tuple

\note Does not support std::reference_wrapper<>
*/
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
    template <class... Args> constexpr tuple<typename std::decay<Args>::type...> make_tuple(Args &&... args) { return tuple<typename std::decay<Args>::type...>{std::forward<Args>(args)...}; };
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    /*! \brief Accesses the N-th member of the collection
    \tparam N A number 0 to len-1
    \tparam Args Deduced from the parameter
    \return A reference (rvalue, lvalue or const lvalue) to the member
    \param f The collection to access
    */
    template <size_t N, class... Args> constexpr auto &&get(tuple<Args...> &&f) { return detail::get<N, Args...>()(std::move(f)); }
    //! \overload
    template <size_t N, class... Args> constexpr auto &get(tuple<Args...> &f) { return detail::get<N, Args...>()(f); }
    //! \overload
    template <size_t N, class... Args> constexpr const auto &get(const tuple<Args...> &f) { return detail::get<N, Args...>()(f); }

    //! Helper for fetching the size of a tuple
    template <class T> struct tuple_size;
    template <class... Types> struct tuple_size<tuple<Types...>> : public std::integral_constant<std::size_t, sizeof...(Types)>
    {
    };
    template <class T> struct tuple_size<const T> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };
    template <class T> struct tuple_size<volatile T> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };
    template <class T> struct tuple_size<const volatile T> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };

    //! Helper for fetching the type of a tuple member
    template <size_t I, class T> struct tuple_element;
    template <size_t I, class... Types> struct tuple_element<I, tuple<Types...>>
    {
      typedef decltype(detail::get<I, Types...>()(std::declval<tuple<Types...>>())) type;
    };
    template <size_t I, class T> struct tuple_element<I, const T>
    {
      typedef typename std::add_const<typename tuple_element<I, T>::type>::type type;
    };
    template <size_t I, class T> struct tuple_element<I, volatile T>
    {
      typedef typename std::add_volatile<typename tuple_element<I, T>::type>::type type;
    };
    template <size_t I, class T> struct tuple_element<I, const volatile T>
    {
      typedef typename std::add_cv<typename tuple_element<I, T>::type>::type type;
    };
  }  // namespace
}  // namespace
#endif
