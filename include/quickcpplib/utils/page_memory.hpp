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

#ifndef QUICKCPPLIB_UTILS_PAGE_MEMORY_HPP
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_HPP

#include "../config.hpp"

#include <cstddef>
#include <cstring>  // for memcpy
#ifdef __cpp_exceptions
#include <exception>
#endif
#include <new>

#ifdef QUICKCPPLIB_EXPORTS
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_CLASS_DECL QUICKCPPLIB_SYMBOL_EXPORT
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL extern QUICKCPPLIB_SYMBOL_EXPORT
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_MEMFUNC_DECL
#else
#if defined(__cplusplus) && (!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) &&                   \
!defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_CLASS_DECL
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL extern inline
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_MEMFUNC_DECL inline
#elif defined(QUICKCPPLIB_DYN_LINK) && !defined(QUICKCPPLIB_STATIC_LINK)
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_CLASS_DECL QUICKCPPLIB_SYMBOL_IMPORT
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL extern QUICKCPPLIB_SYMBOL_IMPORT
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_MEMFUNC_DECL
#else
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_CLASS_DECL
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL extern
#define QUICKCPPLIB_UTILS_PAGE_MEMORY_MEMFUNC_DECL
#endif
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace utils
{
  namespace page_memory
  {
    namespace detail
    {
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *allocate(size_t &bytes) noexcept;
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void free(void *p, size_t bytes) noexcept;
      QUICKCPPLIB_UTILS_PAGE_MEMORY_FUNC_DECL void *resize(void *p, size_t &newbytes, size_t oldbytes,
                                                           bool can_relocate) noexcept;
    }  // namespace detail
    class page_memory_ptr;
    inline page_memory_ptr make_page_memory(size_t bytes);
    inline page_memory_ptr make_page_memory(std::nothrow_t, size_t bytes) noexcept;

    //! A unique ptr to page memory
    class page_memory_ptr
    {
      friend inline page_memory_ptr make_page_memory(size_t bytes);
      friend inline page_memory_ptr make_page_memory(std::nothrow_t, size_t bytes) noexcept;

      void *_p{nullptr};
      size_t _bytes{0};

      constexpr page_memory_ptr(void *p, size_t bytes) noexcept
          : _p(p)
          , _bytes(bytes)
      {
      }

    public:
      page_memory_ptr() = default;
      page_memory_ptr(const page_memory_ptr &) = delete;
      page_memory_ptr &operator=(const page_memory_ptr &) = delete;
      page_memory_ptr(page_memory_ptr &&o) noexcept
          : _p(o._p)
          , _bytes(o._bytes)
      {
        o._p = nullptr;
        o._bytes = 0;
      }
      page_memory_ptr &operator=(page_memory_ptr &&o) noexcept
      {
        if(&o != this)
        {
          this->~page_memory_ptr();
          new(this) page_memory_ptr(static_cast<page_memory_ptr &&>(o));
        }
        return *this;
      }
      ~page_memory_ptr() { reset(); }

      explicit operator bool() const noexcept { return _p != nullptr; }
      void reset()
      {
        if(_p != nullptr)
        {
          detail::free(_p, _bytes);
          _p = nullptr;
          _bytes = 0;
        }
      }
      void release()
      {
        _p = nullptr;
        _bytes = 0;
      }
      void swap(page_memory_ptr &o) noexcept
      {
        void *tp = _p;
        size_t tbytes = _bytes;
        _p = o._p;
        _bytes = o._bytes;
        o._p = tp;
        o._bytes = tbytes;
      }
      void *get() noexcept { return _p; }
      const void *get() const noexcept { return _p; }
      size_t size() const noexcept { return _bytes; }
      size_t try_resize(std::nothrow_t, size_t newbytes) noexcept
      {
        if(_bytes == 0)
        {
          _p = detail::allocate(newbytes);
          if(_p == nullptr)
          {
            return 0;
          }
          _bytes = newbytes;
          return newbytes;
        }
        auto *p = detail::resize(_p, newbytes, _bytes, false);
        if(p == nullptr)
        {
          return 0;
        }
        _p = p;
        _bytes = newbytes;
        return newbytes;
      }
#ifdef __cpp_exceptions
      size_t resize(size_t newbytes)
      {
        if(_bytes == 0)
        {
          _p = detail::allocate(newbytes);
          if(_p == nullptr)
          {
            throw std::bad_alloc();
          }
          _bytes = newbytes;
          return newbytes;
        }
        auto *p = detail::resize(_p, newbytes, _bytes, true);
        if(p == nullptr)
        {
          p = detail::allocate(newbytes);
          if(p == nullptr)
          {
            throw std::bad_alloc();
          }
          const auto tocopy = (newbytes < _bytes) ? newbytes : _bytes;
          memcpy(p, _p, tocopy);
          detail::free(_p, _bytes);
        }
        _p = p;
        _bytes = newbytes;
        return newbytes;
      }
#endif
      size_t resize(std::nothrow_t, size_t newbytes) noexcept
      {
        if(_bytes == 0)
        {
          _p = detail::allocate(newbytes);
          if(_p == nullptr)
          {
            return 0;
          }
          _bytes = newbytes;
          return newbytes;
        }
        auto *p = detail::resize(_p, newbytes, _bytes, true);
        if(p == nullptr)
        {
          p = detail::allocate(newbytes);
          if(p == nullptr)
          {
            return 0;
          }
          const auto tocopy = (newbytes < _bytes) ? newbytes : _bytes;
          memcpy(p, _p, tocopy);
          detail::free(_p, _bytes);
        }
        _p = p;
        _bytes = newbytes;
        return newbytes;
      }
    };
#ifdef __cpp_exceptions
    inline page_memory_ptr make_page_memory(size_t bytes)
    {
      auto *p = detail::allocate(bytes);
      if(p == nullptr)
      {
        throw std::bad_alloc();
      }
      return page_memory_ptr(p, bytes);
    }
#endif
    inline page_memory_ptr make_page_memory(std::nothrow_t, size_t bytes) noexcept
    {
      auto *p = detail::allocate(bytes);
      if(p == nullptr)
      {
        return {};
      }
      return page_memory_ptr(p, bytes);
    }
  }  // namespace page_memory
}  // namespace utils

QUICKCPPLIB_NAMESPACE_END

#if(!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_INCLUDED_BY_HEADER 1
#include "../detail/impl/page_memory.ipp"
#undef QUICKCPPLIB_INCLUDED_BY_HEADER
#endif

#endif
