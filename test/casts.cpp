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

#include "../include/quickcpplib/boost/test/unit_test.hpp"
#include "../include/quickcpplib/detach_cast.hpp"
#include "../include/quickcpplib/erasure_cast.hpp"
#include "../include/quickcpplib/in_place_detach_attach.hpp"

#include <array>

struct DetachCastTest
{
  std::string s;
};
QUICKCPPLIB_NAMESPACE_BEGIN
namespace detach_cast
{
  namespace traits
  {
    template <> struct enable_reinterpret_detach_cast<DetachCastTest> : public std::true_type
    {
    };
    template <> struct enable_reinterpret_attach_cast<DetachCastTest> : public std::true_type
    {
    };
  }  // namespace traits
}  // namespace detach_cast
QUICKCPPLIB_NAMESPACE_END

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

BOOST_AUTO_TEST_CASE(bit_cast / works, "Tests that bit_cast works as advertised")
{
  using QUICKCPPLIB_NAMESPACE::detach_cast::detail::is_bit_cast_valid;
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
  using Carray = char[sizeof(T)];
  using Carray1 = char[sizeof(T) + 1];
  using STDarray = std::array<char, sizeof(T)>;
  static_assert(is_bit_cast_valid<T, Carray>(), "bit_cast(T, Carray) is not callable");
  static_assert(!is_bit_cast_valid<Carray, T>(), "bit_cast(Carray, T) is callable");  // Functions cannot return C arrays by value
  static_assert(!is_bit_cast_valid<T, Carray1>(), "bit_cast(T, Carray1) is callable");
  static_assert(is_bit_cast_valid<T, STDarray>(), "bit_cast(T, STDarray) is not callable");
  static_assert(is_bit_cast_valid<STDarray, T>(), "bit_cast(STDarray, T) is not callable");
}


template <class To, class From,                                                                                    //
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
  using Carray = char[sizeof(T)];
  using Carray1 = char[sizeof(T) + 1];
  using STDarray = std::array<char, sizeof(T)>;
  static_assert(is_erasure_cast_valid<T, Carray>(), "erasure_cast(T, Carray) is not callable");
  static_assert(!is_erasure_cast_valid<Carray, T>(), "erasure_cast(Carray, T) is callable");  // Functions cannot return C arrays by value
  static_assert(is_erasure_cast_valid<T, Carray1>(), "erasure_cast(T, Carray1) is not callable");
  static_assert(is_erasure_cast_valid<T, STDarray>(), "erasure_cast(T, STDarray) is not callable");
  static_assert(is_erasure_cast_valid<STDarray, T>(), "erasure_cast(STDarray, T) is not callable");
}


BOOST_AUTO_TEST_CASE(detach_attach_cast / works, "Tests that detach_cast and attach_cast works as advertised")
{
  using QUICKCPPLIB_NAMESPACE::detach_cast::attach_cast;
  using QUICKCPPLIB_NAMESPACE::detach_cast::detach_cast;
  {
    enum class Test
    {
      label1,
      label2
    } a(Test::label2);
    char a_copy[sizeof(a)];
    memcpy(a_copy, &a, sizeof(a));
    auto &b = detach_cast(a);
    using output_type = std::remove_reference<decltype(b)>::type;
    static_assert(sizeof(b) == sizeof(a), "Detached representation is not same size as input object");
    static_assert(!std::is_const<output_type>::value, "Output type is const");
    BOOST_CHECK((void *) &a == (void *) &b);
    BOOST_CHECK(0 == memcmp(a_copy, &b, sizeof(b)));

    Test &c = attach_cast<Test>(b);
    BOOST_CHECK((void *) &a == (void *) &c);
    BOOST_CHECK(0 == memcmp(a_copy, &c, sizeof(b)));
  }
  {
    const enum class Test { label1, label2 } a(Test::label2);
    char a_copy[sizeof(a)];
    memcpy(a_copy, &a, sizeof(a));
    auto &b = detach_cast(a);
    using output_type = std::remove_reference<decltype(b)>::type;
    static_assert(sizeof(b) == sizeof(a), "Detached representation is not same size as input object");
    static_assert(std::is_const<output_type>::value, "Output type is not const");
    BOOST_CHECK((void *) &a == (void *) &b);
    BOOST_CHECK(0 == memcmp(a_copy, &b, sizeof(b)));

    const Test &c = attach_cast<const Test>(b);
    BOOST_CHECK((void *) &a == (void *) &c);
    BOOST_CHECK(0 == memcmp(a_copy, &c, sizeof(b)));
  }

  {
    // The following is completely UB in C++ 20
    DetachCastTest c{"test string"};
    auto &d = detach_cast(c);
    // If this proposal were accepted, c's destructor would not run now, but its storage would be
    // rebound into a byte[sizeof(DetachCastTest)].
    static_assert(sizeof(d) == sizeof(c), "Detached representation is not same size as input object");
    BOOST_CHECK((void *) &c == (void *) &d);

    DetachCastTest &e = attach_cast<DetachCastTest>(d);
    BOOST_CHECK((void *) &c == (void *) &e);
  }
}

BOOST_AUTO_TEST_CASE(in_place_detach_attach / works, "Tests that in_place_detach and in_place_attach works as advertised")
{
  {
    using QUICKCPPLIB_NAMESPACE::in_place_attach_detach::in_place_attach;
    using QUICKCPPLIB_NAMESPACE::in_place_attach_detach::in_place_detach;
    float v = 5.0f;
    QUICKCPPLIB_NAMESPACE::span::span<QUICKCPPLIB_NAMESPACE::byte::byte> b = in_place_detach(QUICKCPPLIB_NAMESPACE::span::span<float>{&v, 1});
    QUICKCPPLIB_NAMESPACE::span::span<float> a = in_place_attach<float>(b);
    BOOST_CHECK(&v == a.data());
    BOOST_CHECK(v == a[0]);
  }
  union attachable_test {
    double v;
    QUICKCPPLIB_NAMESPACE::byte::byte b[sizeof(double)];
  } test;
  test.v = 78.2;
  {
    QUICKCPPLIB_NAMESPACE::in_place_attach_detach::attached<double> a(test.b);  // attaches
    BOOST_CHECK(a.size() == 1);
    BOOST_CHECK(a.data() == &test.v);
    BOOST_CHECK(*a.begin() == 78.2);
    *a.begin() = 99.8;
    // detaches on destruction
  }
  BOOST_CHECK(test.v == 99.8);
}

BOOST_AUTO_TEST_SUITE_END()
