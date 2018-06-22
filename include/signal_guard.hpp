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

    // Signal install flags
    early_global_signals = (1 << 28)  //!< CAUTION: Enable signals globally at install time, not at guard time. This is dangerous, see documentation.
  }
  QUICKCPPLIB_BITFIELD_END(signalc)

  namespace detail
  {
    SIGNALGUARD_FUNC_DECL const char *signalc_to_string(signalc code) noexcept;
    SIGNALGUARD_FUNC_DECL signalc &signal_guard_installed() noexcept;
  }

  /*! On platforms where it is necessary (POSIX), installs, and potentially enables, the global signal handlers for
  the signals specified by `guarded`. Each signal installed is threadsafe reference
  counted, so this is safe to call from multiple threads or multiple times.

  If this call does anything at all, it is not fast, plus it serialises on a global mutex.

  ## POSIX only

  Changing the signal mask for a process involves a kernel transition, which costs perhaps
  500 CPU cycles. The default implementation enables the guarded signals for the local thread
  just before executing the guarded section of code, and restores the previous thread local
  signal mask on exiting the guarded section of code. This, inevitably, adds at least 1,000
  CPU cycles to each guarded code invocation, but it comes with the big advantage of predictability.

  One can set the `signalc::early_global_signals` flag during signal install which profoundly
  changes these semantics. When we install the handlers, we use `SA_NODEFER` to prevent the
  blocking of the raised signal during the execution of the signal handler. We enable the guarded
  signals immediately, and globally.

  These differences mean that `signal_guard` no longer needs to touch the signal mask during
  execution, and thus avoid all kernel transitions. Performance is enormously improved. The
  cost however is that signal handling becomes much less predictable. If the installed signal
  handlers cause the raising of a signal, an infinite loop occurs. Signal handlers are executed
  in all threads in the process, not just in the guarded code sections.

  Because of these profound differences, you cannot mix different types of signal install in
  the same process. An attempt to instantiate a second install with differing `signalc::early_global_signals`
  will throw an exception.
  */
  class SIGNALGUARD_CLASS_DECL signal_guard_install
  {
    signalc _guarded;
#ifndef _WIN32
    sigset_t _former;  // used only if signalc::early_global_signals
#endif

  public:
    SIGNALGUARD_MEMFUNC_DECL explicit signal_guard_install(signalc guarded);
    SIGNALGUARD_MEMFUNC_DECL ~signal_guard_install();
    signal_guard_install(const signal_guard_install &) = delete;
    signal_guard_install(signal_guard_install &&) = delete;
    signal_guard_install &operator=(const signal_guard_install &) = delete;
    signal_guard_install &operator=(signal_guard_install &&) = delete;
  };

  /*! Call the currently installed signal handler for a signal (POSIX), or raise a Win32 structured
  exception (Windows), returning false if no handler was called due to the currently
  installed handler being `SIG_IGN` (POSIX).

  Note that on POSIX, we fetch the currently installed signal handler and try to call it directly.
  This allows us to supply custom `info` and `context`, and we do all the things which the signal
  handler flags tell us to do beforehand [1]. If the current handler has been defaulted, we
  enable the signal and execute `pthread_kill(pthread_self(), signo)` in order to invoke the
  default handling.

  Note that on Windows, `context` is ignored as there is no way to override the context thrown
  with a Win32 structured exception.

  [1]: We currently do not implement alternative stack switching. If a handler requests that, we
  simply abort the process. Code donations implementing support are welcome.
  */
  SIGNALGUARD_FUNC_DECL bool thread_local_raise_signal(signalc signo, void *info = nullptr, void *context = nullptr);


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

  /*! \class raised_signal_info
  \brief Portable information about a raised signal.
  */
  class raised_signal_info
  {
  protected:
    raised_signal_info() = default;

  public:
    jmp_buf _buf;

    raised_signal_info(const raised_signal_info &) = delete;
    raised_signal_info(raised_signal_info &&) = delete;
    raised_signal_info &operator=(const raised_signal_info &) = delete;
    raised_signal_info &operator=(raised_signal_info &&) = delete;

    //! The signal raised
    virtual signalc signal() const = 0;
    //! The faulting address for `signalc::undefined_memory_access`, `signalc::segmentation_fault`, `signalc::illegal_instruction` and `signalc::floating_point_error`.
    virtual void *address() const = 0;
//! The system specific error code for this signal, the `si_errno` code (POSIX) or `NTSTATUS` code (Windows)
#ifdef _WIN32
    virtual long error_code() const = 0;
#else
    virtual int error_code() const = 0;
#endif

    //! The OS specific `siginfo_t *` (POSIX) or `PEXCEPTION_RECORD` (Windows)
    virtual void *raw_info() = 0;
    //! The OS specific `ucontext_t` (POSIX) or `PCONTEXT` (Windows)
    virtual void *raw_context() = 0;

    virtual bool _set_siginfo(intptr_t, void *, void *) = 0;
    virtual bool _call_continuer() const = 0;
  };

  namespace detail
  {
    SIGNALGUARD_FUNC_DECL raised_signal_info *&current_signal_handler() noexcept;
#ifdef _WIN32
    SIGNALGUARD_FUNC_DECL unsigned long win32_exception_filter_function(unsigned long code, _EXCEPTION_POINTERS *pts) noexcept;
#endif
    struct SIGNALGUARD_CLASS_DECL erased_signal_handler_info : public raised_signal_info
    {
      SIGNALGUARD_MEMFUNC_DECL erased_signal_handler_info();

      SIGNALGUARD_MEMFUNC_DECL signalc signal() const override final;
      SIGNALGUARD_MEMFUNC_DECL void *address() const override final;
#ifdef _WIN32
      SIGNALGUARD_MEMFUNC_DECL long error_code() const override final;
#else
      SIGNALGUARD_MEMFUNC_DECL int error_code() const override final;
#endif
      SIGNALGUARD_MEMFUNC_DECL void *raw_info() override final;
      SIGNALGUARD_MEMFUNC_DECL void *raw_context() override final;

      SIGNALGUARD_MEMFUNC_DECL void acquire(signalc guarded);
      SIGNALGUARD_MEMFUNC_DECL void release(signalc guarded);

    private:
      SIGNALGUARD_MEMFUNC_DECL virtual bool _set_siginfo(intptr_t, void *, void *) override final;
      char _erased[1424];
    };
    template <class R> inline R throw_signal_raised(const raised_signal_info &i) { throw signal_raised(i.signal()); }
    inline bool continue_or_handle(const raised_signal_info & /*unused*/) noexcept { return false; }
  }

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

  /*! Call a callable `f` with signals `guarded` protected for this thread only, returning whatever `f` returns.

  Note that on POSIX, if a `signal_guard_install` is not already instanced, one is temporarily installed, which is
  not quick. You are therefore very strongly recommended, when on POSIX, to call this function
  with a `signal_guard_install` already installed.

  Firstly, how to restore execution to this context is saved, if thread locally configured the guarded signals are enabled for the calling
  thread, and `f` is executed, returning whatever `f` returns, and restoring the signal enabled state to what
  it was on entry to this guard before returning. This is mostly inlined code, so it will be relatively fast. No memory
  allocation is performed if a `signal_guard_install` is already instanced. Approximate overhead on
  an Intel CPU:

  - Linux (thread local): 1450 CPU cycles (mostly the two syscalls to enable and disable the guarded signals)
  - Linux (early global): 52 CPU cycles
  - Windows: 85 CPU cycles

  If during the execution of `f`, any one of the signals `guarded` is raised:

  1. `c`, which must have the prototype `bool(const raised_signal_info &)`, is called with the signal which
  was raised. You can fix
  the cause of the signal and return `true` to continue execution, or else return `false` to halt execution.
  Note that the variety of code you can call in `c` is extremely limited, the same restrictions
  as for signal handlers apply.

  2. If `c` returned `false`, the execution of `f` is halted **immediately** without stack unwind, the thread is returned
  to the state just before the calling of `f`, and the callable `g` is called with the specific signal
  which occurred. `g` must have the prototype `R(const raised_signal_info &)` where `R` is the return type of `f`.
  `g` is called with this signal guard
  removed, though a signal guard higher in the call chain may instead be active.

  Obviously all state which `f` may have been in the process of doing will be thrown away. You
  should therefore make sure that `f` never causes side effects, including the interruption in the middle
  of some operation, which cannot be fixed by the calling of `h`. The default `h` simply throws a `signal_raised`
  C++ exception.
  */
  QUICKCPPLIB_TEMPLATE(class F, class H, class C, class R = decltype(std::declval<F>()()))
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_constructible<R, decltype(std::declval<H>()(std::declval<const raised_signal_info>()))>::value),  //
                        QUICKCPPLIB_TPRED(std::is_constructible<bool, decltype(std::declval<C>()(std::declval<const raised_signal_info>()))>::value))
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
      virtual bool _call_continuer() const override final { return continuer(*this); }
    };
    signal_handler_info_ guard(static_cast<C &&>(c));
    signal_guard_install *sgi = nullptr;
#ifndef _WIN32
    char _sgi[sizeof(signal_guard_install)];
    if(detail::signal_guard_installed() == 0)
    {
      sgi = reinterpret_cast<signal_guard_install *>(_sgi);
      new(sgi) signal_guard_install(guarded);
    }
#endif
    // Nothing in the C++ standard says that the compiler cannot reorder reads and writes
    // around a setjmp(), so let's prevent that. This is the weak form affecting the
    // compiler reordering only.
    std::atomic_signal_fence(std::memory_order_seq_cst);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)  // interaction between setjmp() and C++ object destruction is non-portable
#endif
    if(setjmp(guard._buf))
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    {
      // returning from longjmp, so unset the TLS and call failure handler
      guard.release(guarded);
      if(sgi != nullptr)
        sgi->~signal_guard_install();
      return h(guard);
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
      longjmp(guard._buf, 1);
    }
#else
    // Set the TLS, enable the signals
    guard.acquire(guarded);
    auto release = scoped_undo::undoer([&] {
      guard.release(guarded);
      if(sgi != nullptr)
        sgi->~signal_guard_install();
    });
    return f();
#endif
  }
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 6
#pragma GCC diagnostic pop
#endif
  //! \overload
  template <class F, class R = decltype(std::declval<F>()())> inline R signal_guard(signalc guarded, F &&f) { return signal_guard(guarded, static_cast<F &&>(f), detail::throw_signal_raised<R>, detail::continue_or_handle); }
  //! \overload
  QUICKCPPLIB_TEMPLATE(class F, class H, class R = decltype(std::declval<F>()()))
  QUICKCPPLIB_TREQUIRES(QUICKCPPLIB_TPRED(std::is_constructible<R, decltype(std::declval<H>()(std::declval<const raised_signal_info>()))>::value))
  inline auto signal_guard(signalc guarded, F &&f, H &&h) { return signal_guard(guarded, static_cast<F &&>(f), static_cast<H &&>(h), detail::continue_or_handle); }
}

QUICKCPPLIB_NAMESPACE_END

#if(!defined(QUICKCPPLIB_HEADERS_ONLY) || QUICKCPPLIB_HEADERS_ONLY == 1) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#define QUICKCPPLIB_INCLUDED_BY_HEADER 1
#include "../src/signal_guard.cpp"
#undef QUICKCPPLIB_INCLUDED_BY_HEADER
#endif


#endif
