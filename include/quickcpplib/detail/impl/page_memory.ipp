/* Page memory related functions
(C) 2022 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Dec 2022


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

#include "../../utils/page_memory.hpp"

#ifdef _WIN32
#include <memoryapi.h>
#else
#include <sys/mman.h>
#include <unistd.h>  // for getpagesize()
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace utils
{
  namespace page_memory
  {
    namespace detail
    {
#ifdef _WIN32
      inline size_t getpagesize() noexcept
      {
        static size_t ret;
        if(ret == 0u)
        {
          SYSTEM_INFO si{};
          memset(&si, 0, sizeof(si));
          GetSystemInfo(&si);
          ret = si.dwPageSize;
        }
        return ret;
      }
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *allocate(size_t &bytes) noexcept
      {
        auto *ret = ::VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(ret == nullptr)
        {
          return nullptr;
        }
        const auto ps = getpagesize();
        bytes = (bytes + ps - 1) & ~(ps - 1);
        return ret;
      }
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void free(void *p, size_t bytes) noexcept
      {
        if(!::VirtualFree(p, 0, MEM_RELEASE))
        {
          abort();
        }
      }
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *resize(void *p, size_t &newbytes, size_t oldbytes) noexcept
      {
        (void) p;
        (void) newbytes;
        (void) oldbytes;
        return nullptr;
      }
#else
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *allocate(size_t &bytes) noexcept
      {
        auto *ret = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(ret == MAP_FAILED)
        {
          return nullptr;
        }
        const auto ps = ::getpagesize();
        bytes = (bytes + ps - 1) & ~(ps - 1);
        return ret;
      }
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void free(void *p, size_t bytes) noexcept
      {
        if(-1 == ::munmap(p, bytes))
        {
          abort();
        }
      }
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *resize(void *p, size_t &newbytes, size_t oldbytes,
                                                           bool can_relocate) noexcept
      {
        (void) p;
        (void) newbytes;
        (void) oldbytes;
#ifdef __linux__
        auto *ret = ::mremap(p, oldbytes, newbytes, can_relocate ? MREMAP_MAYMOVE : 0);
        if(ret == MAP_FAILED)
        {
          return nullptr;
        }
        const auto ps = ::getpagesize();
        newbytes = (newbytes + ps - 1) & ~(ps - 1);
        return ret;
#else
        return nullptr;
#endif
      }
#endif
    }  // namespace detail
  }    // namespace page_memory
}  // namespace utils

QUICKCPPLIB_NAMESPACE_END
