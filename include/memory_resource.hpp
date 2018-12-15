/* PMR support
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Nov 2018


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

#ifndef QUICKCPPLIB_MEMORY_RESOURCE_HPP
#define QUICKCPPLIB_MEMORY_RESOURCE_HPP

#include "config.hpp"

#ifdef QUICKCPPLIB_USE_STD_MEMORY_RESOURCE

#include <memory_resource>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace pmr = std::pmr;

QUICKCPPLIB_NAMESPACE_END

#elif _HAS_CXX17 || __cplusplus >= 201700

#include <memory_resource>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace pmr = std::pmr;

QUICKCPPLIB_NAMESPACE_END

#else

#include <memory>

QUICKCPPLIB_NAMESPACE_BEGIN

//! The world's worst C++ 14 emulation of polymorphic_allocator, which maps onto std::allocator
namespace pmr
{
  template <class T> using polymorphic_allocator = std::allocator<T>;
}

QUICKCPPLIB_NAMESPACE_END

#endif

#endif
