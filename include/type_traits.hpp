/* Extended type traits
(C) 2012-2016 Niall Douglas http://www.nedproductions.biz/
Created: May 2016
*/

#ifndef BOOSTLITE_TYPE_TRAITS_HPP
#define BOOSTLITE_TYPE_TRAITS_HPP

#include <type_traits>

namespace boost_lite
{
  namespace type_traits
  {
    namespace detail
    {
      template <class T> struct decay_preserving_cv
      {
        typedef typename std::remove_reference<T>::type U;
        typedef typename std::conditional<std::is_array<U>::value, typename std::remove_extent<U>::type *, typename std::conditional<std::is_function<U>::value, typename std::add_pointer<U>::type, U>::type>::type type;
      };
      // Support for SFINAE detection of iterator/pointer ranges (Can it dereference? Can it increment?)
      //    template<class T, typename = void> struct is_rangeable : std::false_type { };
      //    template<class T> struct is_rangeable<T, decltype(*std::declval<T&>(), ++std::declval<T&>(), void())> : std::true_type { };
      // Support for SFINAE detection of containers (does it have begin() and end()?), made considerably more complex by needing MSVC to work.
      template <class T> inline auto is_container_impl(T) -> decltype(*std::begin(std::declval<T>()), *std::end(std::declval<T>()), bool()) { return true; }
      inline int is_container_impl(...) { return 0; }
    }
    template <class T, typename = decltype(detail::is_container_impl(std::declval<T>()))> struct is_container : std::false_type
    {
    };
    template <class T> struct is_container<T, bool> : std::true_type
    {
      typedef decltype(*std::begin(*((typename std::remove_reference<T>::type *) nullptr))) raw_type;  //!< The raw type (probably a (const) lvalue ref) returned by *it
      typedef typename detail::decay_preserving_cv<raw_type>::type type;                               //!< The type held by the container, still potentially const if container does not permit write access
    };
  }
}

#endif
