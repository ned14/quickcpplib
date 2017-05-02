/* Test the atuple hack
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (2 commits)


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

#include "../include/atuple.hpp"
#include "../include/boost/test/unit_test.hpp"
#include <typeinfo>

BOOST_AUTO_TEST_SUITE(atuple)

template <class T> struct print_type;

BOOST_AUTO_TEST_CASE(atuple / simple / works, "Tests that the atuple works as advertised")
{
  using namespace boost_lite::aggregate_tuple;
  // Constexpr test
  constexpr tuple<int, double, const char *> ct1 = {1, 1.0, "1"};
  constexpr auto ct2 = make_tuple(1, 1.0, "1");
  constexpr tuple<int, double, const char *> ct3[] = {{1, 1.0, "1"}, {2, 2.0, "2"}, {3, 3.0, "3"}};
  // Container test
  std::vector<tuple<int, double, const char *>> t1 = {{1, 1.0, "1"}};
  auto t2 = make_tuple(1, 1.0, "1");
  std::vector<tuple<int, double, const char *>> t3 = {{1, 1.0, "1"}, {2, 2.0, "2"}, {3, 3.0, "3"}};

  static_assert(tuple_size<decltype(ct1)>() == 3, "ct1 is not three long!");
  static_assert(tuple_size<decltype(ct2)>() == 3, "ct2 is not three long!");
  static_assert(tuple_size<std::decay<decltype(ct3[0])>::type>() == 3, "ct3 is not three long!");

  static_assert(std::is_same<decltype(get<0>(ct1)), const int &>::value, "ct1<0> does not have correct type");
  static_assert(std::is_same<decltype(get<1>(ct1)), const double &>::value, "ct1<1> does not have correct type");
  static_assert(std::is_same<decltype(get<2>(ct1)), const char *const &>::value, "ct1<2> does not have correct type");
  // This should work, but doesn't for some reason I can't figure
  // static_assert(std::is_same<tuple_element<0, decltype(ct1)>::type, const int &>::value, "ct1<0> does not have type int");
  static_assert(std::is_same<decltype(get<0>(ct2)), const int &>::value, "ct2<0> does not have correct type");
  static_assert(std::is_same<decltype(get<1>(ct2)), const double &>::value, "ct2<1> does not have correct type");
  static_assert(std::is_same<decltype(get<2>(ct2)), const char *const &>::value, "ct2<2> does not have correct type");

  static_assert(std::is_same<decltype(get<0>(t1.front())), int &>::value, "t1<0> does not have correct type");
  static_assert(std::is_same<decltype(get<1>(t1.front())), double &>::value, "t1<1> does not have correct type");
  static_assert(std::is_same<decltype(get<2>(t1.front())), const char *&>::value, "t1<2> does not have correct type");
  // This should work, but doesn't for some reason I can't figure
  // static_assert(std::is_same<tuple_element<0, decltype(t2)>::type, int&>::value, "t2<0> does not have type int");
  static_assert(std::is_same<decltype(get<0>(t2)), int &>::value, "t2<0> does not have correct type");
  static_assert(std::is_same<decltype(get<1>(t2)), double &>::value, "t2<1> does not have correct type");
  static_assert(std::is_same<decltype(get<2>(t2)), const char *&>::value, "t2<2> does not have correct type");
  static_assert(std::is_same<decltype(get<0>(std::move(t2))), int &&>::value, "t2<0> does not have correct type");
  static_assert(std::is_same<decltype(get<1>(std::move(t2))), double &&>::value, "t2<1> does not have correct type");
  static_assert(std::is_same<decltype(get<2>(std::move(t2))), const char *&&>::value, "t2<2> does not have correct type");
  BOOST_CHECK(get<0>(ct1) == 1);
  BOOST_CHECK(get<1>(ct1) == 1.0);
  BOOST_CHECK(get<0>(ct3[1]) == 2);
  BOOST_CHECK(get<1>(ct3[1]) == 2.0);
  BOOST_CHECK(get<0>(t3[1]) == 2);
  BOOST_CHECK(get<1>(t3[1]) == 2.0);
}

BOOST_AUTO_TEST_SUITE_END()
