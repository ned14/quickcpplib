/* Signal guard support
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (4 commits)
File Created: June 2018


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

#ifndef QUICKCPPLIB_SIGNAL_GUARD_HPP
#define QUICKCPPLIB_SIGNAL_GUARD_HPP

#include "bitfield.hpp"
#include "scoped_undo.hpp"

#include <csetjmp>
#include <exception>

#ifdef QUICKCPPLIB_EXPORTS
#define SIGNALGUARD_CLASS_DECL QUICKCPPLIB_SYMBOL_EXPORT
#define SIGNALGUARD_FUNC_DECL extern QUICKCPPLIB_SYMBOL_EXPORT
#define SIGNALGUARD_MEMFUNC_DECL
#else
#if defined(__cplusplus) && (!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#define SIGNALGUARD_CLASS_DECL
#define SIGNALGUARD_FUNC_DECL inline
#define SIGNALGUARD_MEMFUNC_DECL inline
#else
#define SIGNALGUARD_CLASS_DECL QUICKCPPLIB_SYMBOL_IMPORT
#define SIGNALGUARD_FUNC_DECL extern QUICKCPPLIB_SYMBOL_IMPORT
#define SIGNALGUARD_MEMFUNC_DECL
#endif
#endif


QUICKCPPLIB_NAMESPACE_BEGIN

//! \brief The namespace for signal_guard
namespace signal_guard
{
  //! The signals which can be raised
  QUICKCPPLIB_BITFIELD_BEGIN(signalc)
  {
    none = 0,

    abort_process = (1 << 0),            //!< The process is aborting
    undefined_memory_access = (1 << 1),  //!< Attempt to access a memory page which doesn't exist
    illegal_instruction = (1 << 2),      //!< Execution of illegal instruction
    interrupt = (1 << 3),                //!< The process is interrupted
    broken_pipe = (1 << 4),              //!< Reader on a pipe vanished
    segmentation_fault = (1 << 5),       //!< Attempt to access a memory page whose permissions disallow

    floating_point_error = (1 << 8),  //!< Floating point error
    // leave bits 9-15 for individual FP errors

    // C++ handlers
    out_of_memory = (1 << 16),  //!< A call to operator new failed, and a throw is about to occur
    termination = (1 << 17),    //!< A call to std::terminate() was made
  }
  QUICKCPPLIB_BITFIELD_END(signalc)

  namespace detail
  {
    SIGNALGUARD_FUNC_DECL const char *signalc_to_string(signalc code) noexcept;
  }

  /*! On platforms where it is necessary, installs the global signal handlers for
  the signals specified by `guarded`. Each signal installed is threadsafe reference
  counted, so this is safe to call from multiple threads or multiple times.

  If this call does anything at all, it is not fast, plus it serialises on a global mutex.
  */
  class SIGNALGUARD_CLASS_DECL signal_guard_install
  {
    signalc _guarded;

  public:
    SIGNALGUARD_MEMFUNC_DECL explicit signal_guard_install(signalc guarded);
    SIGNALGUARD_MEMFUNC_DECL ~signal_guard_install();
    signal_guard_install(const signal_guard_install &) = delete;
    signal_guard_install(signal_guard_install &&) = delete;
    signal_guard_install &operator=(const signal_guard_install &) = delete;
    signal_guard_install &operator=(signal_guard_install &&) = delete;
  };


  //! Thrown by the default signal handler to abort the current operation
  class signal_raised : public std::exception
  {
    signalc _code;

  public:
    //! Constructor
    signal_raised(signalc code)
        : _code(code)
    {
    }
    virtual const char *what() const noexcept override { return detail::signalc_to_string(_code); }
  };

  namespace detail
  {
    struct signal_handler_info_base;
    SIGNALGUARD_FUNC_DECL signal_handler_info_base *&current_signal_handler() noexcept;
#ifdef _WIN32
    SIGNALGUARD_FUNC_DECL unsigned long win32_exception_filter_function(unsigned long code, _EXCEPTION_POINTERS *pts) noexcept;
#endif
    struct signal_handler_info_base
    {
      jmp_buf buf;
      virtual bool set_siginfo(intptr_t, void *, void *) = 0;
      virtual bool call_continuer() const = 0;
    };
    struct SIGNALGUARD_CLASS_DECL erased_signal_handler_info : public signal_handler_info_base
    {
      SIGNALGUARD_MEMFUNC_DECL explicit erased_signal_handler_info();

      SIGNALGUARD_MEMFUNC_DECL signalc signal() const;
      SIGNALGUARD_MEMFUNC_DECL const void *info() const;
      SIGNALGUARD_MEMFUNC_DECL const void *context() const;

      SIGNALGUARD_MEMFUNC_DECL void acquire(signalc guarded);
      SIGNALGUARD_MEMFUNC_DECL void release(signalc guarded);

    private:
      SIGNALGUARD_MEMFUNC_DECL virtual bool set_siginfo(intptr_t, void *, void *) override final;
      char _erased[1408];
    };
    template <class R> inline R throw_signal_raised(signalc code, const void * /*unused*/, const void * /*unused*/) { throw signal_raised(code); }
    inline bool continue_or_handle(signalc /*unused*/, const void * /*unused*/, const void * /*unused*/) noexcept { return false; }
  }

  /*! Call a callable `f` with signals `guarded` protected for this thread only, returning whatever `f` returns.

  You must have instantiated a `signal_guard_install` for the guarded signals at least once somewhere in
  your process if this signal guard is to work.

  Firstly, how to restore execution to this context is saved, the guarded signals are enabled for the calling
  thread, and `f` is executed, returning whatever `f` returns, and restoring the signal enabled state to what
  it was on entry to this guard before returning. This is mostly inlined code, so it will be relatively fast. No memory
  allocation is performed, though thread local state is allocated on first execution. Approximate overhead on
  an Intel Skylake CPU:

  - Linux: 490 CPU cycles
  - Windows: 85 CPU cycles

  If during the execution of `f`, any one of the signals `guarded` is raised:

  1. `c`, which must have the prototype `bool(signalc, const void *, const void *)`, is called with the signal which
  was raised. The two `void *` are the `siginfo_t *` and `void *` from the `sa_sigaction` handler on POSIX;
  on Windows they are the `PEXCEPTION_RECORD` and `PCONTEXT` from the vectored exception handler. You can fix
  the cause of the signal and return `true` to continue execution, or else return `false` to halt execution.
  Note that the variety of code you can call in `c` is extremely limited, the same restrictions
  as for signal handlers apply.

  2. If `c` returned `false`, the execution of `f` is halted **immediately** without stack unwind, the thread is returned
  to the state just before the calling of `f`, and the callable `g` is called with the specific signal
  which occurred. `g` must have the prototype `R(signalc, const void *, const void *)` where `R` is the return type of `f`,
  and the two `void *` are the same as for the calling of `c`.

  Obviously all state which `f` may have been in the process of doing will be thrown away. You
  should therefore make sure that `f` never causes side effects, including the interruption in the middle
  of some operatiob,mwhich cannot be fixed by the calling of `h`. The default `h` simply throws a `signal_raised`
  C++ exception.
  */
  QUICKCPPLIB_TEMPLATE(class F, class H, class C, class R = decltype(std::declval<F>()()))
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_constructible<R, decltype(std::declval<H>()(signalc::none, static_cast<const void *>(nullptr), static_cast<const void *>(nullptr)))>::value),  //
                        QUICKCPPLIB_TPRED(std::is_constructible<bool, decltype(std::declval<C>()(signalc::none, static_cast<const void *>(nullptr), static_cast<const void *>(nullptr)))>::value))
  inline R signal_guard(signalc guarded, F &&f, H &&h, C &&c)
  {
    struct signal_handler_info_ : public detail::erased_signal_handler_info
    {
      C continuer;  // whether to continue or not
      signal_handler_info_(C &&c)
          : continuer(static_cast<C &&>(c))
      {
      }
      ~signal_handler_info_() = default;

    protected:
      virtual bool call_continuer() const override final { return continuer(this->signal(), this->info(), this->context()); }
    };
    signal_handler_info_ guard(static_cast<C &&>(c));
    // Nothing in the C++ standard says that the compiler cannot reorder reads and writes
    // around a setjmp(), so let's prevent that. This is the weak form affecting the
    // compiler reordering only.
    std::atomic_signal_fence(std::memory_order_seq_cst);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)  // interaction between setjmp() and C++ object destruction is non-portable
#endif
    if(setjmp(guard.buf))
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    {
      // returning from longjmp, so unset the TLS and call failure handler
      guard.release(guarded);
      return h(guard.signal(), guard.info(), guard.context());
    }
    std::atomic_signal_fence(std::memory_order_seq_cst);
#ifdef _WIN32
    __try
    {
      return [&] {
        guard.acquire(guarded);
        auto release = scoped_undo::undoer([&] { guard.release(guarded); });
        return f();
      }();
    }
    __except(detail::win32_exception_filter_function(GetExceptionCode(), GetExceptionInformation()))
    {
      longjmp(guard.buf, 1);
    }
#else
    // Set the TLS, enable the signals
    guard.acquire(guarded);
    auto release = scoped_undo::undoer([&] { guard.release(guarded); });
    return f();
#endif
  }
  //! \overload
  template <class F, class R = decltype(std::declval<F>()())> inline R signal_guard(signalc guarded, F &&f) { return signal_guard(guarded, static_cast<F &&>(f), detail::throw_signal_raised<R>, detail::continue_or_handle); }
  //! \overload
  QUICKCPPLIB_TEMPLATE(class F, class H, class R = decltype(std::declval<F>()()))
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_constructible<R, decltype(std::declval<H>()(signalc::none, static_cast<const void *>(nullptr), static_cast<const void *>(nullptr)))>::value))
  inline auto signal_guard(signalc guarded, F &&f, H &&h) { return signal_guard(guarded, static_cast<F &&>(f), static_cast<H &&>(h), detail::continue_or_handle); }
}

QUICKCPPLIB_NAMESPACE_END

#if(!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_INCLUDED_BY_HEADER 1
#include "../src/signal_guard.cpp"
#undef QUICKCPPLIB_INCLUDED_BY_HEADER
#endif


#endif
