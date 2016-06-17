/* Extended type traits
(C) 2012-2016 Niall Douglas http://www.nedproductions.biz/
Created: May 2016
*/

#ifndef BOOSTLITE_TYPE_TRAITS_HPP
#define BOOSTLITE_TYPE_TRAITS_HPP

#include <type_traits>

namespace boost_lite
{
  //! Gets the compiler to error out printing a type
  template <class T> struct print_type
  {
  private:
    print_type() {}
  };

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
      template <class T> inline auto is_sequence_impl(T) -> decltype(*std::begin(std::declval<T>()), *std::end(std::declval<T>()), bool()) { return true; }
      inline int is_sequence_impl(...) { return 0; }
    }
    //! True if type T is a STL compliant sequence (does it have begin() and end()?)
    template <class T, typename = decltype(detail::is_sequence_impl(std::declval<T>()))> struct is_sequence : std::false_type
    {
    };
    template <class T> struct is_sequence<T, bool> : std::true_type
    {
      typedef decltype(*std::begin(*((typename std::remove_reference<T>::type *) nullptr))) raw_type;  //!< The raw type (probably a (const) lvalue ref) returned by *it
      typedef typename detail::decay_preserving_cv<raw_type>::type type;                               //!< The type held by the container, still potentially const if container does not permit write access
    };


#if 0
    // Disabled until I find the time to get it working
    namespace detail
    {
      template <size_t N> struct Char
      {
        char foo[N];
      };
      // Overload only available if a default constructed T has a constexpr-available size()
      template <class T, size_t N = static_cast<T *>(nullptr)->size() + 1> constexpr inline Char<N> constexpr_size(const T &) { return Char<N>(); }
      template <class T> constexpr inline Char<1> constexpr_size(...) { return Char<1>(); }
    }
    /*! Returns true if the instance of v has a constexpr size()
    /note This is too overly conservative, it does not correctly return true for constexpr input.
    */
    template <class T> constexpr inline bool has_constexpr_size(const T &v) { return sizeof(detail::constexpr_size<typename std::decay<T>::type>(std::move(v))) > 1; }
    //! \overload
    template <class T> constexpr inline bool has_constexpr_size() { return sizeof(detail::constexpr_size<typename std::decay<T>::type>(std::declval<T>())) > 1; }

    static_assert(has_constexpr_size<std::array<std::string, 2>>(), "failed");

#if 0
      // Non-constexpr array (always has a constexpr size())
      auto ca = std::array<int, 2>();
      // Constexpr initializer_list (has constexpr size()). Note fails to compile on
      // VS2015 as its initializer_list isn't constexpr constructible yet
#ifndef _MSC_VER
      constexpr std::initializer_list<int> cil{ 1, 2 };
#endif
      // Non-constexpr initializer_list (does not have constexpr size())
      std::initializer_list<int> il{ 1, 2 };
      // Non-constexpr vector (never has constexpr size())
      std::vector<int> vec{ 1, 2 };

      // Correct on GCC 4.9 and clang 3.8 and VS2015
      static_assert(ca.size(), "non-constexpr array size constexpr");
      // Correct on GCC 4.9 and clang 3.8.
#ifndef _MSC_VER
      static_assert(cil.size(), "constexpr il size constexpr");
#endif
      // Fails as you'd expect everywhere with non-constexpr il error
      // static_assert(il.size(), "non-constexpr il size constexpr");

      // Correct on GCC 4.9 and clang 3.8 and VS2015
      static_assert(has_constexpr_size(ca), "ca");
      // Incorrect on GCC 4.9 and clang 3.8 and VS2015
#ifndef _MSC_VER
      static_assert(!has_constexpr_size(cil), "cil");  // INCORRECT!
#endif
                                                       // Correct on GCC 4.9 and clang 3.8 and VS2015
      static_assert(!has_constexpr_size(il), "il");
      // Correct on GCC 4.9 and clang 3.8 and VS2015
      static_assert(!has_constexpr_size(vec), "vec");

      constexpr bool test_ca() {
        return has_constexpr_size(std::array<int, 2>{1, 2});
      }
      constexpr bool testca = test_ca();
      // Correct on GCC 4.9 and clang 3.8 and VS2015
      static_assert(testca, "testca()");

      constexpr bool test_cil() {
        return has_constexpr_size(std::initializer_list<int>{1, 2});
      }
      constexpr bool testcil = test_cil();
      // Incorrect on GCC 4.9 and clang 3.8 and VS2015
      static_assert(!testcil, "testcil()");          // INCORRECT!
#endif
#endif
  }
}

#endif
