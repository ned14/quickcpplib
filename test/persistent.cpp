/* Test persistent
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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
#include "../include/persistent.hpp"

BOOST_AUTO_TEST_SUITE(persistence)

BOOST_AUTO_TEST_CASE(persistence / works, "Tests that persistent<T> works as advertised")
{
  QUICKCPPLIB_NAMESPACE::persistence::persistent<int> v(5);
  v.store(6);
  BOOST_CHECK(v == 6);
}

BOOST_AUTO_TEST_SUITE_END()
