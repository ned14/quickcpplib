/* Test STL binds
(C) 2014-2017 Niall Douglas <http://www.nedproductions.biz/> (1 commit)


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

#include "../include/bind/stl11/boost/ratio"
#include "../include/bind/stl11/std/ratio"
#include "../include/boost/test/unit_test.hpp"

BOOST_AUTO_TEST_SUITE(bind)

BOOST_AUTO_TEST_CASE(bind / works, "Tests that the STL binds work as advertised")
{
  // In this context use Boost's ratio
  {
    using namespace boost_lite::bind::boost::ratio;
    using two_thirds = ratio<2, 3>;
    using one_sixth = ratio<1, 6>;
    BOOST_CHECK((ratio_less<one_sixth, two_thirds>::value));
  }
  // In this context use STL11's ratio
  {
    using namespace boost_lite::bind::std::ratio;
    using two_thirds = ratio<2, 3>;
    using one_sixth = ratio<1, 6>;
    BOOST_CHECK((ratio_less<one_sixth, two_thirds>::value));
  }
}

BOOST_AUTO_TEST_SUITE_END()
