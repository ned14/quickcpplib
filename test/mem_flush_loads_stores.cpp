/* Test ensure_stores
(C) 2018 - 2019 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/mem_flush_loads_stores.hpp"
#include "../include/boost/test/unit_test.hpp"

BOOST_AUTO_TEST_SUITE(mem_flush_loads_stores)

BOOST_AUTO_TEST_CASE(ensure_loads_stores / mem_flush_stores, "Tests that mem_flush_stores() works as advertised")
{
  using QUICKCPPLIB_NAMESPACE::byte::to_byte;
  QUICKCPPLIB_NAMESPACE::byte::byte array[16384];
  array[5] = to_byte(4);
  array[10000] = to_byte(78);
  array[999] = to_byte(99);
  auto ret = QUICKCPPLIB_NAMESPACE::mem_flush_loads_stores::mem_flush_stores(array);
  BOOST_CHECK(ret == QUICKCPPLIB_NAMESPACE::mem_flush_loads_stores::memory_flush_none);
  ret = QUICKCPPLIB_NAMESPACE::mem_flush_loads_stores::mem_flush_stores(array, QUICKCPPLIB_NAMESPACE::mem_flush_loads_stores::memory_flush_retain);
  BOOST_CHECK(ret != QUICKCPPLIB_NAMESPACE::mem_flush_loads_stores::memory_flush_none);
}

BOOST_AUTO_TEST_SUITE_END()
