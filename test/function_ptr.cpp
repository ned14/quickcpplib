/* Test the function ptr
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
#include "../include/function_ptr.hpp"

BOOST_AUTO_TEST_SUITE(function_ptr)

template <size_t N> struct Callable
{
  int arr[N];
  Callable(int a, float b)
      : arr{(int) (a + b)}
  {
  }
  int operator()(double a, double b) { return (int) (arr[0] + a + b); }
  int operator()(double a, double b) const { return (int) (a - b); }
};

struct NonMoveable
{
  int _v;
  explicit constexpr NonMoveable(int a)
      : _v(a)
  {
  }
  NonMoveable(NonMoveable &&) = delete;
  NonMoveable(const NonMoveable &) = delete;
  int operator()() { return _v; }
};

BOOST_AUTO_TEST_CASE(function_ptr / works, "Tests that function_ptr works as advertised")
{
  auto a = QUICKCPPLIB_NAMESPACE::function_ptr::make_function_ptr<int(double, double)>([](double a, double b) { return (int) (a + b); });
  BOOST_CHECK(a(5, 6) == 11);
  auto b = QUICKCPPLIB_NAMESPACE::function_ptr::emplace_function_ptr<int(double, double), Callable<1>>(1, 2.f);
  BOOST_CHECK(b(5, 6) == 14);
  auto c = QUICKCPPLIB_NAMESPACE::function_ptr::emplace_function_ptr<int(double, double), Callable<100>>(1, 3.f);
  BOOST_CHECK(c(5, 6) == 15);
  auto d = std::move(b);
  BOOST_CHECK(d(5, 6) == 14);
  d = std::move(c);
  BOOST_CHECK(d(5, 6) == 15);
  auto e = QUICKCPPLIB_NAMESPACE::function_ptr::emplace_function_ptr<int(), NonMoveable>(5);
  BOOST_CHECK(e() == 5);
}

BOOST_AUTO_TEST_SUITE_END()
