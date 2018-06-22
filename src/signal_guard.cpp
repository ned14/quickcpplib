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

#include "../include/signal_guard.hpp"

#include "../include/spinlock.hpp"

#include <csignal>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace signal_guard
{
  namespace detail
  {
    SIGNALGUARD_FUNC_DECL const char *signalc_to_string(signalc code) noexcept
    {
      static constexpr const char *strings[] = {"Signal abort process", "Signal undefined memory access", "Signal illegal instruction", "Signal interrupt", "Signal broken pipe", "Signal segmentation fault", nullptr, nullptr, "Signal floating point error"};
      if(code == signalc::none)
        return "none";
      for(size_t n = 0; n < sizeof(strings) / sizeof(strings[0]); n++)
      {
        if((static_cast<unsigned>(code) & (1 << n)) != 0)
          return strings[n];
      }
      if((static_cast<unsigned>(code) & static_cast<unsigned>(signalc::out_of_memory)) != 0)
        return "C++ out of memory";
      if((static_cast<unsigned>(code) & static_cast<unsigned>(signalc::termination)) != 0)
        return "C++ termination";
      return "unknown";
    }

    SIGNALGUARD_FUNC_DECL raised_signal_info *&current_signal_handler() noexcept
    {
      static QUICKCPPLIB_THREAD_LOCAL raised_signal_info *v;
      return v;
    }
#ifndef _WIN32
    SIGNALGUARD_FUNC_DECL signalc &signal_guard_installed() noexcept
    {
      static QUICKCPPLIB_THREAD_LOCAL signalc v;
      return v;
    }
#endif

    static configurable_spinlock::spinlock<bool> lock;
    static unsigned new_handler_count, terminate_handler_count;
    static std::new_handler new_handler_old;
    static std::terminate_handler terminate_handler_old;

    inline void new_handler()
    {
      auto *shi = current_signal_handler();
      if(shi != nullptr)
      {
        if(shi->_set_siginfo(signalc::out_of_memory, nullptr, nullptr))
        {
          if(!shi->_call_continuer())
          {
            longjmp(shi->_buf, 1);
          }
        }
      }
      if(new_handler_old != nullptr)
      {
        new_handler_old();
      }
      else
      {
        throw std::bad_alloc();
      }
    }
    inline void terminate_handler()
    {
      auto *shi = current_signal_handler();
      if(shi != nullptr)
      {
        if(shi->_set_siginfo(signalc::termination, nullptr, nullptr))
        {
          if(!shi->_call_continuer())
          {
            longjmp(shi->_buf, 1);
          }
        }
      }
      if(terminate_handler_old != nullptr)
      {
        terminate_handler_old();
      }
      else
      {
        std::abort();
      }
    }

#ifdef _WIN32
    inline DWORD signo_from_signalc(signalc c)
    {
      switch(static_cast<unsigned>(c))
      {
      case signalc::abort_process:
        return EXCEPTION_NONCONTINUABLE_EXCEPTION;
      case signalc::undefined_memory_access:
        return EXCEPTION_IN_PAGE_ERROR;
      case signalc::illegal_instruction:
        return EXCEPTION_ILLEGAL_INSTRUCTION;
      // case signalc::interrupt:
      //  return SIGINT;
      // case signalc::broken_pipe:
      //  return SIGPIPE;
      case signalc::segmentation_fault:
        return EXCEPTION_ACCESS_VIOLATION;
      case signalc::floating_point_error:
        return EXCEPTION_FLT_INVALID_OPERATION;
      }
      return (DWORD) -1;
    }
    inline signalc signalc_from_signo(intptr_t c)
    {
      switch(c)
      {
      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return signalc::abort_process;
      case EXCEPTION_IN_PAGE_ERROR:
        return signalc::undefined_memory_access;
      case EXCEPTION_ILLEGAL_INSTRUCTION:
        return signalc::illegal_instruction;
      // case SIGINT:
      //  return signalc::interrupt;
      // case SIGPIPE:
      //  return signalc::broken_pipe;
      case EXCEPTION_ACCESS_VIOLATION:
        return signalc::segmentation_fault;
      case EXCEPTION_FLT_DENORMAL_OPERAND:
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      case EXCEPTION_FLT_INEXACT_RESULT:
      case EXCEPTION_FLT_INVALID_OPERATION:
      case EXCEPTION_FLT_OVERFLOW:
      case EXCEPTION_FLT_STACK_CHECK:
      case EXCEPTION_FLT_UNDERFLOW:
        return signalc::floating_point_error;
      case EXCEPTION_STACK_OVERFLOW:
        return signalc::out_of_memory;
      default:
        return signalc::none;
      }
    }
    SIGNALGUARD_FUNC_DECL unsigned long win32_exception_filter_function(unsigned long code, _EXCEPTION_POINTERS *ptrs) noexcept
    {
      auto *shi = current_signal_handler();
      if(shi != nullptr)
      {
        if(shi->_set_siginfo(code, ptrs->ExceptionRecord, ptrs->ContextRecord))
        {
          if(!shi->_call_continuer())
          {
            // invoke longjmp
            return EXCEPTION_EXECUTE_HANDLER;
          }
        }
      }
      return EXCEPTION_CONTINUE_SEARCH;
    }
#else
    inline int signo_from_signalc(signalc c)
    {
      switch(static_cast<unsigned>(c))
      {
      case signalc::abort_process:
        return SIGABRT;
      case signalc::undefined_memory_access:
        return SIGBUS;
      case signalc::illegal_instruction:
        return SIGILL;
      case signalc::interrupt:
        return SIGINT;
      case signalc::broken_pipe:
        return SIGPIPE;
      case signalc::segmentation_fault:
        return SIGSEGV;
      case signalc::floating_point_error:
        return SIGFPE;
      }
      return -1;
    }
    inline signalc signalc_from_signo(intptr_t c)
    {
      switch(c)
      {
      case SIGABRT:
        return signalc::abort_process;
      case SIGBUS:
        return signalc::undefined_memory_access;
      case SIGILL:
        return signalc::illegal_instruction;
      case SIGINT:
        return signalc::interrupt;
      case SIGPIPE:
        return signalc::broken_pipe;
      case SIGSEGV:
        return signalc::segmentation_fault;
      case SIGFPE:
        return signalc::floating_point_error;
      }
      return signalc::none;
    }
    struct installed_signal_handler
    {
      int signo;
      unsigned count;
      struct sigaction former;
    };
    static installed_signal_handler handler_counts[16];

    // Simulate the raising of a signal
    inline bool do_raise_signal(int signo, struct sigaction &sa, siginfo_t *_info, void *_context)
    {
      void (*h1)(int signo, siginfo_t *info, void *context) = nullptr;
      void (*h2)(int signo) = nullptr;
      siginfo_t info;
      ucontext_t context;
      // std::cout << "do_raise_signal(" << signo << ", " << (void *) sa.sa_handler << ", " << info << ", " << context << ")" << std::endl;
      if(sa.sa_handler == SIG_IGN)
      {
        // std::cout << "ignore" << std::endl;
        return false;
      }
      else if(sa.sa_handler == SIG_DFL)
      {
        // std::cout << "default" << std::endl;
        // Default action for these is to ignore
        if(signo == SIGCHLD || signo == SIGURG)
          return false;
        // Ok, need to invoke the default handler
        struct sigaction dfl, myformer;
        memset(&dfl, 0, sizeof(dfl));
        dfl.sa_handler = SIG_DFL;
        sigaction(signo, &dfl, &myformer);
        sigset_t myformer2;
        sigset_t def2;
        sigemptyset(&def2);
        sigaddset(&def2, signo);
        pthread_sigmask(SIG_UNBLOCK, &def2, &myformer2);
        pthread_kill(pthread_self(), signo);  // Very likely never returns
        sigaction(signo, &myformer, nullptr);
        return true;
      }
      else if(sa.sa_flags & SA_SIGINFO)
      {
        h1 = sa.sa_sigaction;
        info = *_info;
        context = *(const ucontext_t *) _context;
      }
      else
      {
        h2 = sa.sa_handler;
      }
      // std::cout << "handler" << std::endl;
      if(sa.sa_flags & SA_ONSTACK)
      {
        // Not implemented yet
        abort();
      }
      if(sa.sa_flags & SA_RESETHAND)
      {
        struct sigaction dfl;
        memset(&dfl, 0, sizeof(dfl));
        dfl.sa_handler = SIG_DFL;
        sigaction(signo, &dfl, nullptr);
      }
      if(!(sa.sa_flags & (SA_RESETHAND | SA_NODEFER)))
      {
        sigaddset(&sa.sa_mask, signo);
      }
      sigset_t oldset;
      pthread_sigmask(SIG_BLOCK, &sa.sa_mask, &oldset);
      if(h1 != nullptr)
        h1(signo, &info, &context);
      else
        h2(signo);
      pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
      return true;
    }

    // Called by POSIX to handle a raised signal
    inline void raw_signal_handler(int signo, siginfo_t *info, void *context)
    {
      auto *shi = current_signal_handler();
      // std::cout << "raw_signal_handler(" << signo << ", " << info << ", " << context << ") shi=" << shi << std::endl;
      if(shi != nullptr)
      {
        if(shi->_set_siginfo(signo, info, context))
        {
          if(!shi->_call_continuer())
          {
            longjmp(shi->_buf, 1);
          }
        }
      }
      // Otherwise, call the previous signal handler
      for(const auto &i : handler_counts)
      {
        if(i.signo == signo)
        {
          struct sigaction sa;
          lock.lock();
          sa = i.former;
          lock.unlock();
          do_raise_signal(signo, sa, info, context);
        }
      }
    }

#endif
  }

  SIGNALGUARD_MEMFUNC_DECL signal_guard_install::signal_guard_install(signalc guarded)
      : _guarded(guarded)
  {
#ifndef _WIN32
    {
      signalc alreadyinstalled = detail::signal_guard_installed();
      if(alreadyinstalled)
      {
        if((alreadyinstalled & signalc::early_global_signals) != (guarded & signalc::early_global_signals))
        {
          throw std::runtime_error("Cannot install signal guards with differing signalc::early_global_signals");
        }
      }
    }
    sigset_t set;
    sigemptyset(&set);
    for(size_t n = 0; n < 16; n++)
    {
      if((static_cast<unsigned>(guarded) & (1 << n)) != 0)
      {
        int signo = detail::signo_from_signalc(1 << n);
        if(signo != -1)
        {
          int ret = 0;
          detail::lock.lock();
          detail::handler_counts[n].signo = signo;
          if(!detail::handler_counts[n].count++)
          {
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_sigaction = detail::raw_signal_handler;
            sa.sa_flags = SA_SIGINFO;
            if(guarded & signalc::early_global_signals)
            {
              sa.sa_flags |= SA_NODEFER;
            }
            ret = sigaction(signo, &sa, &detail::handler_counts[n].former);
          }
          detail::lock.unlock();
          if(ret == -1)
          {
            throw std::system_error(errno, std::system_category());
          }
          sigaddset(&set, signo);
        }
      }
    }
    if(guarded & signalc::early_global_signals)
    {
      if(-1 == sigprocmask(SIG_UNBLOCK, &set, &_former))
      {
        throw std::system_error(errno, std::system_category());
      }
    }
    detail::signal_guard_installed() |= guarded;
#endif
    if((guarded & signalc::out_of_memory) || (guarded & signalc::termination))
    {
      detail::lock.lock();
      if(guarded & signalc::out_of_memory)
      {
        if(!detail::new_handler_count++)
        {
          detail::new_handler_old = std::set_new_handler(detail::new_handler);
        }
      }
      if(guarded & signalc::termination)
      {
        if(!detail::terminate_handler_count++)
        {
          detail::terminate_handler_old = std::set_terminate(detail::terminate_handler);
        }
      }
      detail::lock.unlock();
    }
  }
  SIGNALGUARD_MEMFUNC_DECL signal_guard_install::~signal_guard_install()
  {
    if((_guarded & signalc::out_of_memory) || (_guarded & signalc::termination))
    {
      detail::lock.lock();
      if(_guarded & signalc::out_of_memory)
      {
        if(!--detail::new_handler_count)
        {
          std::set_new_handler(detail::new_handler_old);
        }
      }
      if(_guarded & signalc::termination)
      {
        if(!--detail::terminate_handler_count)
        {
          std::set_terminate(detail::terminate_handler_old);
        }
      }
      detail::lock.unlock();
    }
#ifndef _WIN32
    if(_guarded & signalc::early_global_signals)
    {
      sigprocmask(SIG_SETMASK, &_former, nullptr);
    }
    bool deinstalled = true;
    for(size_t n = 0; n < 16; n++)
    {
      if((static_cast<unsigned>(_guarded) & (1 << n)) != 0)
      {
        int signo = detail::signo_from_signalc(1 << n);
        if(signo != -1)
        {
          int ret = 0;
          detail::lock.lock();
          if(!--detail::handler_counts[n].count)
          {
            ret = sigaction(signo, &detail::handler_counts[n].former, nullptr);
          }
          else
          {
            deinstalled = false;
          }
          detail::lock.unlock();
          if(ret == -1)
          {
            abort();
          }
        }
      }
    }
    if(deinstalled)
    {
      detail::lock.lock();
      for(size_t n = 0; n < 16; n++)
      {
        if(detail::handler_counts[n].count != 0)
        {
          deinstalled = false;
        }
      }
      if(deinstalled)
      {
        detail::signal_guard_installed() = signalc::none;
      }
      detail::lock.unlock();
    }
#endif
  }

  SIGNALGUARD_FUNC_DECL bool thread_local_raise_signal(signalc _signo, void *_info, void *_context)
  {
    if(_signo == signalc::out_of_memory)
    {
      if(std::get_new_handler() == nullptr)
      {
        std::terminate();
      }
      std::get_new_handler()();
      return true;
    }
    else if(_signo == signalc::termination)
    {
      std::terminate();
    }
    auto signo = detail::signo_from_signalc(_signo);
#ifdef _WIN32
    if((DWORD) -1 == signo)
      throw std::runtime_error("Unknown signal");
    (void) _context;
    const auto *info = (const _EXCEPTION_RECORD *) _info;
    // info->ExceptionInformation[0] = 0=read 1=write 8=DEP
    // info->ExceptionInformation[1] = causing address
    // info->ExceptionInformation[2] = NTSTATUS causing exception
    if(info != nullptr)
    {
      RaiseException(signo, info->ExceptionFlags, info->NumberParameters, info->ExceptionInformation);
    }
    else
    {
      RaiseException(signo, 0, 0, nullptr);
    }
    return false;
#else
    if(-1 == signo)
      throw std::runtime_error("Unknown signal");
    auto *info = (siginfo_t *) _info;
    // Fetch the current handler, and simulate the raise
    struct sigaction sa;
    sigaction(signo, nullptr, &sa);
    return detail::do_raise_signal(signo, sa, info, _context);
#endif
  }

  namespace detail
  {
    struct signal_handler_info
    {
      raised_signal_info *next{nullptr};  // any previously installed info on this thread
      signalc guarded{signalc::none};     // handlers used

      signalc signal{signalc::none};  // the signal which occurred
      intptr_t signo{-1};             // what the signal handler was called with
      bool have_info{false}, have_context{false};
#ifdef _WIN32
      _EXCEPTION_RECORD info;  // what the signal handler was called with
      CONTEXT context;
#else
      sigset_t former;  // former sigmask
      siginfo_t info;   // what the signal handler was called with
      ucontext_t context;
#endif
    };
    static_assert(sizeof(signal_handler_info) <= 1424, "signal_handler_info is too big for erased storage");

    SIGNALGUARD_MEMFUNC_DECL erased_signal_handler_info::erased_signal_handler_info()
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      new(p) signal_handler_info;
    }

    SIGNALGUARD_MEMFUNC_DECL signalc erased_signal_handler_info::signal() const
    {
      auto *p = reinterpret_cast<const signal_handler_info *>(_erased);
      return p->signal;
    }
    SIGNALGUARD_MEMFUNC_DECL void *erased_signal_handler_info::address() const
    {
      auto *p = reinterpret_cast<const signal_handler_info *>(_erased);
#ifdef _WIN32
      return p->have_info ? (void *) p->info.ExceptionInformation[1] : nullptr;
#else
      return p->have_info ? p->info.si_addr : nullptr;
#endif
    }
#ifdef _WIN32
    SIGNALGUARD_MEMFUNC_DECL long erased_signal_handler_info::error_code() const
    {
      auto *p = reinterpret_cast<const signal_handler_info *>(_erased);
      return p->have_info ? (long) p->info.ExceptionInformation[2] : 0;
    }
#else
    SIGNALGUARD_MEMFUNC_DECL int erased_signal_handler_info::error_code() const
    {
      auto *p = reinterpret_cast<const signal_handler_info *>(_erased);
      return p->have_info ? p->info.si_errno : 0;
    }
#endif
    SIGNALGUARD_MEMFUNC_DECL void *erased_signal_handler_info::raw_info()
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      return p->have_info ? static_cast<void *>(&p->info) : nullptr;
    }
    SIGNALGUARD_MEMFUNC_DECL void *erased_signal_handler_info::raw_context()
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      return p->have_context ? static_cast<void *>(&p->context) : nullptr;
    }

    SIGNALGUARD_MEMFUNC_DECL void erased_signal_handler_info::acquire(signalc guarded)
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      // Set me to current handler
      p->next = current_signal_handler();
      p->guarded = guarded;
      std::atomic_signal_fence(std::memory_order_seq_cst);
      current_signal_handler() = this;
#ifndef _WIN32
      if(!(signal_guard_installed() & signalc::early_global_signals))
      {
        // Enable my guarded signals for this thread
        sigset_t set;
        sigemptyset(&set);
#if 1
        for(size_t n = 0; n < 16; n++)
        {
          if((static_cast<unsigned>(guarded) & (1 << n)) != 0)
          {
            int signo = detail::signo_from_signalc(1 << n);
            if(signo != -1)
            {
              sigaddset(&set, signo);
            }
          }
        }
#else
        unsigned g = static_cast<unsigned>(guarded);
        for(size_t n = __builtin_ffs(g); n != 0; g ^= 1 << (n - 1), n = __builtin_ffs(g))
        {
          int signo = detail::signo_from_signalc(1 << (n - 1));
          if(signo != -1)
          {
            sigaddset(&set, signo);
          }
        }
#endif
        pthread_sigmask(SIG_UNBLOCK, &set, &p->former);
      }
#endif
    }
    SIGNALGUARD_MEMFUNC_DECL void erased_signal_handler_info::release(signalc /* unused */)
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      current_signal_handler() = p->next;
      std::atomic_signal_fence(std::memory_order_seq_cst);
#ifndef _WIN32
      if(!(signal_guard_installed() & signalc::early_global_signals))
      {
        // Restore signal mask for this thread to what it was on entry to guarded section
        pthread_sigmask(SIG_SETMASK, &p->former, nullptr);
      }
#endif
      p->~signal_handler_info();
    }
    SIGNALGUARD_MEMFUNC_DECL bool erased_signal_handler_info::_set_siginfo(intptr_t signo, void *info, void *context)
    {
      auto *p = reinterpret_cast<signal_handler_info *>(_erased);
      p->signo = signo;
      if(signo == signalc::out_of_memory || signo == signalc::termination)
      {
        // This is a C++ failure handler
        p->signal = (signalc)(unsigned) signo;
        if((p->guarded & p->signal) == 0)
          return false;  // I am not guarding this signal
        return true;
      }
      // Otherwise this is either a Win32 exception code or POSIX signal number
      p->signal = signalc_from_signo(signo);
      if(p->signal == signalc::none)
        return false;  // I don't support this signal
      if((p->guarded & p->signal) == 0)
        return false;  // I am not guarding this signal
      p->have_info = p->have_context = false;
      if(info != nullptr)
      {
        memcpy(&p->info, info, sizeof(p->info));
        p->have_info = true;
      }
      if(context != nullptr)
      {
        memcpy(&p->context, context, sizeof(p->context));
        p->have_context = true;
      }
      return true;
    }
  }
}

QUICKCPPLIB_NAMESPACE_END
