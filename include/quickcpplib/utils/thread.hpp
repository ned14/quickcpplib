/* Thread related functions
(C) 2016-2022 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: Jun 2016


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

#ifndef QUICKCPPLIB_UTILS_THREAD_HPP
#define QUICKCPPLIB_UTILS_THREAD_HPP

#include "../config.hpp"
#include "../declval.hpp"

#include <cstdint>
#include <new>
#include <type_traits>

#ifdef __linux__
#include <sys/syscall.h>  // for SYS_gettid
#include <unistd.h>       // for syscall()
#endif

#if defined(__APPLE__)
#include <mach/mach_init.h>  // for mach_thread_self
#include <mach/mach_port.h>  // for mach_port_deallocate
#endif

#ifdef __FreeBSD__
#include <pthread_np.h>  // for pthread_getthreadid_np
#endif

#ifdef QUICKCPPLIB_EXPORTS
#define QUICKCPPLIB_UTILS_THREAD_CLASS_DECL QUICKCPPLIB_SYMBOL_EXPORT
#define QUICKCPPLIB_UTILS_THREAD_FUNC_DECL extern QUICKCPPLIB_SYMBOL_EXPORT
#define QUICKCPPLIB_UTILS_THREAD_MEMFUNC_DECL
#else
#if defined(__cplusplus) && (!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) &&                   \
!defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_UTILS_THREAD_CLASS_DECL
#define QUICKCPPLIB_UTILS_THREAD_FUNC_DECL extern inline
#define QUICKCPPLIB_UTILS_THREAD_MEMFUNC_DECL inline
#elif defined(QUICKCPPLIB_DYN_LINK) && !defined(QUICKCPPLIB_STATIC_LINK)
#define QUICKCPPLIB_UTILS_THREAD_CLASS_DECL QUICKCPPLIB_SYMBOL_IMPORT
#define QUICKCPPLIB_UTILS_THREAD_FUNC_DECL extern QUICKCPPLIB_SYMBOL_IMPORT
#define QUICKCPPLIB_UTILS_THREAD_MEMFUNC_DECL
#else
#define QUICKCPPLIB_UTILS_THREAD_CLASS_DECL
#define QUICKCPPLIB_UTILS_THREAD_FUNC_DECL extern
#define QUICKCPPLIB_UTILS_THREAD_MEMFUNC_DECL
#endif
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace utils
{
  namespace thread
  {
#ifdef _WIN32
    namespace win32
    {
      extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
    }
#endif
    //! The thread id of the calling thread
    inline uint32_t this_thread_id() noexcept
    {
#ifdef _WIN32
      return (uint32_t) win32::GetCurrentThreadId();
#elif defined(__linux__)
      return (uint32_t) syscall(SYS_gettid);
#elif defined(__APPLE__)
      thread_port_t tid = mach_thread_self();
      mach_port_deallocate(mach_task_self(), tid);
      return (uint32_t) tid;
#else
      return (uint32_t) pthread_getthreadid_np();
#endif
    }

    namespace detail
    {
      struct async_signal_safe_thread_local_base
      {
        virtual ~async_signal_safe_thread_local_base() {}
        virtual void _destroy(void *addr) noexcept = 0;
      };
      QUICKCPPLIB_UTILS_THREAD_FUNC_DECL bool
      async_signal_safe_thread_local_access(bool destroy_all, void *&out,
                                            const async_signal_safe_thread_local_base *obj, uint32_t tid,
                                            uint16_t bytes, uint16_t align) noexcept;
      template <class T> struct default_constructor
      {
        T *operator()(void *addr) const { return new(addr) T; }
      };
    }  // namespace detail
    template <class T, class U> class async_signal_safe_thread_local;
    template <class T, class U = detail::default_constructor<T>>
    inline async_signal_safe_thread_local<T, U> make_async_signal_safe_thread_local(U &&constructor = U{});

    /*! \class async_signal_safe_thread_local
    \brief A `thread_local` implementation which is safe to call from within signal handlers.
    \tparam T The type of the thread local object
    \tparam U The type of a callable which will initialise the thread local object
    upon first use by a thread.

    Unfortunately, C++ 11's `thread_local` is not guaranteed to be safe to call
    from within signal handlers, and to be more specific, recent versions of glibc
    are _specifically_ unsafe to call from within signal handlers, as they now
    are implemented using a lazily called `malloc` which is a big no-no from within
    a signal handler.

    Similarly, `boost::thread_specific_ptr` is implemented using `malloc`, and
    `atexit()` almost invariably can use `malloc`. On some platforms e.g. glibc,
    not even `__attribute__((thread_local))` is async signal safe (why?
    https://www.akkadia.org/drepper/tls.pdf says that `_tls_get_addr()` may call
    malloc. Joy.)

    So we were left with little choice but to reimplement thread local storage
    using async signal safe only mechanisms.

    The object is not relocatable in memory after construction. This is because
    `this_thread_id()` and the address of this object are used to form a key
    which looks up an internal hash table. Generally one therefore ought to
    use static storage for these objects.

    The internal implementation is a linked list of memory pages, each containing
    a hash table. Each hash table is searched in turn as needed. If a new
    `address + thread id` is needed and no free slot can be found in any of the
    existing allocations, a new memory page will need to be allocated which can
    be very slow. Only async signal safe platform syscalls
    are used to expand storage, so table expansion is async signal safe. You
    should cache the pointer returned by this function, to avoid
    unnecessary hash table lookups.

    The storage algorithm is a bit dumb -- it keeps a bunch of kernel memory
    pages per `this_thread_id()` all of which get memory copied upon expansion --
    so storing large objects using this facility would be unwise. We therefore
    limit object size to 64Kb.
    */
    template <class T, class U = detail::default_constructor<T>>
    class async_signal_safe_thread_local : public detail::async_signal_safe_thread_local_base
    {
      static_assert(sizeof(T) + alignof(U) < 65536 + sizeof(void *),
                    "Size plus alignment of type must be less than 64Kb!");
      static_assert(std::is_same<std::decay_t<decltype(declval<U>()((void *) nullptr))>, T *>::value,
                    "The supplied initialiser of T does not return a T *!");
      template <class T_, class U_>
      inline async_signal_safe_thread_local<T_, U_> make_async_signal_safe_thread_local(U_ &&constructor);

      U _constructor;

      virtual void _destroy(void *addr) noexcept override
      {
        auto *p = reinterpret_cast<T *>(addr);
        p->~T();
      }
      T *_get() const
      {
        void *out = nullptr;
        if(detail::async_signal_safe_thread_local_access(false, out, this, this_thread_id(), sizeof(T), alignof(T)))
        {
          return _constructor(out);
        }
        return reinterpret_cast<T *>(out);
      }

    public:
      constexpr async_signal_safe_thread_local(U constructor)
          : _constructor(static_cast<U &&>(constructor))
      {
      }

      async_signal_safe_thread_local(const async_signal_safe_thread_local &) = delete;
      async_signal_safe_thread_local(async_signal_safe_thread_local &&) = delete;
      async_signal_safe_thread_local &operator=(const async_signal_safe_thread_local &) = delete;
      async_signal_safe_thread_local &operator=(async_signal_safe_thread_local &&) = delete;
      ~async_signal_safe_thread_local()
      {
        void *out = nullptr;
        detail::async_signal_safe_thread_local_access(true, out, this, this_thread_id(), sizeof(T), alignof(T));
      }

      //! Access the thread local value, constructing a new instance if necessary
      T &value() & { return *_get(); }
      T &&value() && { return *_get(); }
      const T &value() const & { return *_get(); }
      const T &&value() const && { return *_get(); }
      //! Access the thread local value, constructing a new instance if necessary
      T &operator*() & { return *_get(); }
      T &&operator*() && { return *_get(); }
      const T &operator*() const & { return *_get(); }
      const T &&operator*() const && { return *_get(); }
      //! Access the thread local value, constructing a new instance if necessary
      T *operator->() { return _get(); }
      const T *operator->() const { return _get(); }
    };
    //! Construct an instance of `async_signal_safe_thread_local`, where `T` will be constructed by `constructor(addr)`
    //! on first thread access. If your `T` is default constructible, you need supply nothing here.
    //! Note this is only available on C++ 17, as it requires guaranteed copy elision.
    template <class T, class U>
    inline async_signal_safe_thread_local<T, U> make_async_signal_safe_thread_local(U &&constructor)
    {
      return async_signal_safe_thread_local<T, U>(static_cast<U &&>(constructor));
    }
  }  // namespace thread
}  // namespace utils

QUICKCPPLIB_NAMESPACE_END

#if(!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_INCLUDED_BY_HEADER 1
#include "../detail/impl/thread.ipp"
#undef QUICKCPPLIB_INCLUDED_BY_HEADER
#endif

#endif
