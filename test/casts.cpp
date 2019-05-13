/* Test the casting function
(C) 2019 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/boost/test/unit_test.hpp"
#include "../include/erasure_cast.hpp"
#include "../include/type_traits.hpp"

BOOST_AUTO_TEST_SUITE(casts)

BOOST_AUTO_TEST_CASE(is_invocable / works, "Tests that type_traits::is_invocable works as advertised")
{
  struct not_callable
  {
  private:
    void operator()() const {}
  };
  struct callable_void
  {
    void operator()() const {}
  };
  struct callable_int
  {
    void operator()(int) const {}
  };
  struct callable_void_int
  {
    void operator()() const {}
    void operator()(int) const {}
  };
  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<void>::value, "is_invocable<void> is true");
  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<double>::value, "is_invocable<double> is true");

  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<not_callable>::value, "is_invocable<not_callable> is true");

  static_assert(QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_void>::value, "is_invocable<callable_void> not true");
  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_void, int>::value, "is_invocable<callable_void, int> is true");
  static_assert(QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_int, int>::value, "is_invocable<callable_int, int> not true");
  static_assert(QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_int, short>::value, "is_invocable<callable_int, short> not true");
  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_int, callable_void>::value, "is_invocable<callable_int, callable_void> is true");
  static_assert(QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_void_int>::value, "is_invocable<callable_void_int> not true");
  static_assert(QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_void_int, int>::value, "is_invocable<callable_void_int, int> not true");
  static_assert(!QUICKCPPLIB_NAMESPACE::type_traits::is_invocable<callable_void_int, int, int>::value, "is_invocable<callable_void_int, int, int> is true");
}

template <class To, class From,                                                                            //
          typename = decltype(QUICKCPPLIB_NAMESPACE::bit_cast::bit_cast<To, From>(std::declval<From>()))>  //
inline constexpr bool _is_bit_cast_valid(int)
{
  return true;
};
template <class To, class From>  //
inline constexpr bool _is_bit_cast_valid(...)
{
  return false;
};
template <class To, class From>  //
inline constexpr bool is_bit_cast_valid()
{
  return _is_bit_cast_valid<To, From>(5);
};
BOOST_AUTO_TEST_CASE(bit_cast / works, "Tests that bit_cast works as advertised")
{
  enum class Test
  {
    label
  };
  static_assert(is_bit_cast_valid<int, int>(), "bit_cast(int, int) is not callable");
  static_assert(!is_bit_cast_valid<int, char>(), "bit_cast(int, char) is callable");
  static_assert(is_bit_cast_valid<int, Test>(), "bit_cast(int, Test) is not callable");
  static_assert(is_bit_cast_valid<int, float>(), "bit_cast(int, float) is not callable");
  static_assert(!is_bit_cast_valid<int, double>(), "bit_cast(int, double) is callable");

  struct T
  {
    int a[5];
  };
  using chars = char[sizeof(T)];
  using chars1 = char[sizeof(T) + 1];
  static_assert(is_bit_cast_valid<T, chars>(), "bit_cast(T, chars) is not callable");
  //static_assert(is_bit_cast_valid<chars, T>(), "bit_cast(chars, T) is not callable");  // not working yet
  static_assert(!is_bit_cast_valid<T, chars1>(), "bit_cast(T, chars1) is callable");
}


template <class To, class From,                                                                            //
          typename = decltype(QUICKCPPLIB_NAMESPACE::erasure_cast::erasure_cast<To, From>(std::declval<From>()))>  //
inline constexpr bool _is_erasure_cast_valid(int)
{
  return true;
};
template <class To, class From>  //
inline constexpr bool _is_erasure_cast_valid(...)
{
  return false;
};
template <class To, class From>  //
inline constexpr bool is_erasure_cast_valid()
{
  return _is_erasure_cast_valid<To, From>(5);
};
BOOST_AUTO_TEST_CASE(erasure_cast / works, "Tests that erasure_cast works as advertised")
{
  enum class Test
  {
    label
  };
  static_assert(is_erasure_cast_valid<int, int>(), "erasure_cast(int, int) is not callable");
  static_assert(is_erasure_cast_valid<char, int>(), "erasure_cast(int, char) is not callable");
  static_assert(is_erasure_cast_valid<int, char>(), "erasure_cast(int, char) is not callable");
  static_assert(is_erasure_cast_valid<int, Test>(), "erasure_cast(int, Test) is not callable");
  static_assert(is_erasure_cast_valid<int, float>(), "erasure_cast(int, float) is not callable");
  static_assert(is_erasure_cast_valid<int, double>(), "erasure_cast(int, double) is not callable");

  struct T
  {
    int a[5];
  };
  using chars = char[sizeof(T)];
  using chars1 = char[sizeof(T) + 1];
  static_assert(is_erasure_cast_valid<T, chars>(), "erasure_cast(T, chars) is not callable");
  // static_assert(is_erasure_cast_valid<chars, T>(), "erasure_cast(chars, T) is not callable");  // not working yet
  static_assert(is_erasure_cast_valid<T, chars1>(), "erasure_cast(T, chars1) is not callable");
}
BOOST_AUTO_TEST_SUITE_END()
