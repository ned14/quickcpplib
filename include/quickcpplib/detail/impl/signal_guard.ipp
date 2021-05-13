/* Signal guard support
(C) 2018-2021 Niall Douglas <http://www.nedproductions.biz/> (4 commits)
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

#include "../include/quickcpplib/signal_guard.hpp"

#include "../include/quickcpplib/spinlock.hpp"

#include <cstring>       // for memset etc
#include <system_error>  // for system_error

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4190)  // C-linkage with UDTs
#endif
extern "C" union raised_signal_info_value thrd_signal_guard_call(const sigset_t *signals, thrd_signal_guard_guarded_t guarded,
                                                                 thrd_signal_guard_recover_t recovery, thrd_signal_guard_decide_t decider,
                                                                 union raised_signal_info_value value)
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  signalc_set mask;
  for(size_t n = 0; n < 31; n++)
  {
    if(sigismember(signals, n))
    {
      mask |= static_cast<signalc_set>(1ULL << n);
    }
  }
  return signal_guard(mask, guarded, recovery, decider, value);
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

extern "C" bool thrd_raise_signal(int signo, void *raw_info, void *raw_context)
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  return thrd_raise_signal(static_cast<signalc>(1U << signo), raw_info, raw_context);
}

extern "C" void *signal_guard_create(const sigset_t *guarded)
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  signalc_set mask;
  for(size_t n = 0; n < 31; n++)
  {
    if(sigismember(guarded, n))
    {
      mask |= static_cast<signalc_set>(1ULL << n);
    }
  }
  return new(std::nothrow) QUICKCPPLIB_NAMESPACE::signal_guard::signal_guard_install(mask);
}

extern "C" bool signal_guard_destroy(void *i)
{
  auto *p = (QUICKCPPLIB_NAMESPACE::signal_guard::signal_guard_install *) i;
  delete p;
  return true;
}

QUICKCPPLIB_NAMESPACE_BEGIN

namespace signal_guard
{

  namespace detail
  {
    struct signal_guard_decider_callable
    {
      thrd_signal_guard_decide_t decider;
      raised_signal_info_value value;
      bool operator()(raised_signal_info *rsi) const noexcept
      {
        rsi->value = value;
        return decider(rsi);
      }
    };
  }  // namespace detail
}  // namespace signal_guard
QUICKCPPLIB_NAMESPACE_END

extern "C" void *signal_guard_decider_create(const sigset_t *guarded, bool callfirst, thrd_signal_guard_decide_t decider, union raised_signal_info_value value)
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  signalc_set mask;
  for(size_t n = 0; n < 31; n++)
  {
    if(sigismember(guarded, n))
    {
      mask |= static_cast<signalc_set>(1ULL << n);
    }
  }
  return new(std::nothrow)
  signal_guard_global_decider<detail::signal_guard_decider_callable>(mask, detail::signal_guard_decider_callable{decider, value}, callfirst);
}
extern "C" bool signal_guard_decider_destroy(void *decider)
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  auto *p = (signal_guard_global_decider<detail::signal_guard_decider_callable> *) decider;
  delete p;
  return true;
}


QUICKCPPLIB_NAMESPACE_BEGIN

namespace signal_guard
{
  namespace detail
  {
    static thread_local thread_local_signal_guard *_current_thread_local_signal_handler;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    SIGNALGUARD_FUNC_DECL QUICKCPPLIB_NOINLINE void push_thread_local_signal_handler(thread_local_signal_guard *g) noexcept
    {
      g->previous = _current_thread_local_signal_handler;
      _current_thread_local_signal_handler = g;
    }
    SIGNALGUARD_FUNC_DECL QUICKCPPLIB_NOINLINE void pop_thread_local_signal_handler(thread_local_signal_guard *g) noexcept
    {
      assert(_current_thread_local_signal_handler == g);
      if(_current_thread_local_signal_handler != g)
      {
        abort();
      }
      _current_thread_local_signal_handler = g->previous;
    }
    SIGNALGUARD_FUNC_DECL QUICKCPPLIB_NOINLINE thread_local_signal_guard *current_thread_local_signal_handler() noexcept
    {
      return _current_thread_local_signal_handler;
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    SIGNALGUARD_FUNC_DECL const char *signalc_to_string(signalc code) noexcept
    {
      static constexpr const struct
      {
        signalc code;
        const char *string;
      } strings[] = {
      //
      {signalc::none, "none"},                                               //
      {signalc::abort_process, "Signal abort process"},                      //
      {signalc::undefined_memory_access, "Signal undefined memory access"},  //
      {signalc::illegal_instruction, "Signal illegal instruction"},          //
      {signalc::interrupt, "Signal interrupt"},                              //
      {signalc::broken_pipe, "Signal broken pipe"},                          //
      {signalc::segmentation_fault, "Signal segmentation fault"},            //
      {signalc::floating_point_error, "Signal floating point error"},        //
      {signalc::out_of_memory, "C++ out of memory"},                         //
      {signalc::termination, "C++ termination"}                              //
      };
      for(auto &i : strings)
      {
        if(code == i.code)
          return i.string;
      }
      return "unknown";
    }

    // Overload for when a C++ signal is being raised
    inline bool set_siginfo(thread_local_signal_guard *g, signalc _cppsignal)
    {
      auto cppsignal = static_cast<signalc_set>(1ULL << static_cast<int>(_cppsignal));
      if(g->guarded & cppsignal)
      {
        g->info.signo = static_cast<int>(_cppsignal);
        g->info.error_code = 0;  // no system error code for C++ signals
        g->info.addr = 0;        // no address for OOM nor terminate
        g->info.value.ptr_value = nullptr;
        g->info.raw_info = nullptr;
        g->info.raw_context = nullptr;
        return true;
      }
      return false;
    }

    static configurable_spinlock::spinlock<uintptr_t> lock;
    static unsigned new_handler_count, terminate_handler_count;
    static std::new_handler new_handler_old;
#ifdef _MSC_VER
    static thread_local std::terminate_handler terminate_handler_old;
#else
    static std::terminate_handler terminate_handler_old;
#endif
    struct global_signal_decider
    {
      global_signal_decider *next, *prev;
      signalc_set guarded;
      thrd_signal_guard_decide_t decider;
      raised_signal_info_value value;
    };
    static global_signal_decider *global_signal_deciders_front, *global_signal_deciders_back;
#ifdef _WIN32
    static void *win32_global_signal_decider1;
    static void *win32_global_signal_decider2;
#endif

    inline void new_handler()
    {
      for(auto *shi = current_thread_local_signal_handler(); shi != nullptr; shi = shi->previous)
      {
        if(set_siginfo(shi, signalc::out_of_memory))
        {
          if(!shi->call_continuer())
          {
            longjmp(shi->info.buf, 1);
          }
        }
      }
      lock.lock();
      if(global_signal_deciders_front != nullptr)
      {
        raised_signal_info rsi;
        memset(&rsi, 0, sizeof(rsi));
        rsi.signo = static_cast<int>(signalc::out_of_memory);
        auto *d = global_signal_deciders_front;
        for(size_t n = 0; d != nullptr; n++)
        {
          size_t i = 0;
          for(d = global_signal_deciders_front; d != nullptr; d = d->next)
          {
            if(i++ == n)
            {
              if(d->guarded & (1ULL << static_cast<int>(signalc::out_of_memory)))
              {
                rsi.value = d->value;
                lock.unlock();
                if(d->decider(&rsi))
                {
                  // Cannot continue OOM
                  abort();
                }
                lock.lock();
              }
              break;
            }
          }
        }
      }
      if(new_handler_old != nullptr)
      {
        auto *h = new_handler_old;
        lock.unlock();
        h();
      }
      else
      {
        lock.unlock();
        throw std::bad_alloc();
      }
    }
    inline void terminate_handler()
    {
      for(auto *shi = current_thread_local_signal_handler(); shi != nullptr; shi = shi->previous)
      {
        if(set_siginfo(shi, signalc::termination))
        {
          if(!shi->call_continuer())
          {
            longjmp(shi->info.buf, 1);
          }
        }
      }
      lock.lock();
      if(global_signal_deciders_front != nullptr)
      {
        raised_signal_info rsi;
        memset(&rsi, 0, sizeof(rsi));
        rsi.signo = static_cast<int>(signalc::termination);
        auto *d = global_signal_deciders_front;
        for(size_t n = 0; d != nullptr; n++)
        {
          size_t i = 0;
          for(d = global_signal_deciders_front; d != nullptr; d = d->next)
          {
            if(i++ == n)
            {
              if(d->guarded & (1ULL << static_cast<int>(signalc::termination)))
              {
                rsi.value = d->value;
                lock.unlock();
                if(d->decider(&rsi))
                {
                  // Cannot continue termination
                  abort();
                }
                lock.lock();
              }
              break;
            }
          }
        }
      }
      if(terminate_handler_old != nullptr)
      {
        auto *h = terminate_handler_old;
        lock.unlock();
        h();
      }
      else
      {
        lock.unlock();
        std::abort();
      }
    }
#ifdef _MSC_VER
    static thread_local struct _win32_set_terminate_handler_per_thread_t
    {
      _win32_set_terminate_handler_per_thread_t()
      {
        // On MSVC, the terminate handler is thread local, so we have no choice but to always
        // set it for every thread
        terminate_handler_old = std::set_terminate(terminate_handler);
      }
    } _win32_set_terminate_handler_per_thread;
#endif


#ifdef _WIN32
    namespace win32
    {
      typedef struct _EXCEPTION_RECORD
      {
        unsigned long ExceptionCode;
        unsigned long ExceptionFlags;
        struct _EXCEPTION_RECORD *ExceptionRecord;
        void *ExceptionAddress;
        unsigned long NumberParameters;
        unsigned long long ExceptionInformation[15];
      } EXCEPTION_RECORD;

      typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;

      struct CONTEXT;
      typedef CONTEXT *PCONTEXT;

      typedef struct _EXCEPTION_POINTERS
      {
        PEXCEPTION_RECORD ExceptionRecord;
        PCONTEXT ContextRecord;
      } EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

      typedef long(__stdcall *PVECTORED_EXCEPTION_HANDLER)(struct _EXCEPTION_POINTERS *ExceptionInfo);

      extern void __stdcall RaiseException(unsigned long dwExceptionCode, unsigned long dwExceptionFlags, unsigned long nNumberOfArguments,
                                           const unsigned long long *lpArguments);
      extern PVECTORED_EXCEPTION_HANDLER __stdcall SetUnhandledExceptionFilter(PVECTORED_EXCEPTION_HANDLER Handler);
      extern void *__stdcall AddVectoredContinueHandler(unsigned long First, PVECTORED_EXCEPTION_HANDLER Handler);
      extern unsigned long __stdcall RemoveVectoredContinueHandler(void *Handle);
#pragma comment(lib, "kernel32.lib")
#ifndef QUICKCPPLIB_DISABLE_ABI_PERMUTATION
#define QUICKCPPLIB_SIGNAL_GUARD_SYMBOL2(a, b, c) a #b c
#define QUICKCPPLIB_SIGNAL_GUARD_SYMBOL1(a, b, c) QUICKCPPLIB_SIGNAL_GUARD_SYMBOL2(a, b, c)
#define QUICKCPPLIB_SIGNAL_GUARD_SYMBOL(a, b) QUICKCPPLIB_SIGNAL_GUARD_SYMBOL1(a, QUICKCPPLIB_PREVIOUS_COMMIT_UNIQUE, b)
#if(defined(__x86_64__) || defined(_M_X64)) || (defined(__aarch64__) || defined(_M_ARM64))
#pragma comment(linker,                                                                                                                                        \
                QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RaiseException@win32@detail@signal_guard@_", "@quickcpplib@@YAXKKKPEB_K@Z=RaiseException"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@_",                             \
                                                        "@quickcpplib@@YAP6AJPEAU_EXCEPTION_POINTERS@12345@@ZP6AJ0@Z@Z=SetUnhandledExceptionFilter"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@_",                              \
                                                        "@quickcpplib@@YAPEAXKP6AJPEAU_EXCEPTION_POINTERS@12345@@Z@Z=AddVectoredContinueHandler"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@_",                           \
                                                        "@quickcpplib@@YAKPEAX@Z=RemoveVectoredContinueHandler"))
#elif defined(__x86__) || defined(_M_IX86) || defined(__i386__)
#pragma comment(                                                                                                                                               \
linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RaiseException@win32@detail@signal_guard@_", "@quickcpplib@@YGXKKKPB_K@Z=__imp__RaiseException@16"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@_",                             \
                                                        "@quickcpplib@@YGP6GJPAU_EXCEPTION_POINTERS@12345@@ZP6GJ0@Z@Z=__imp__SetUnhandledExceptionFilter@4"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@_",                              \
                                                        "@quickcpplib@@YGPAXKP6GJPAU_EXCEPTION_POINTERS@12345@@Z@Z=__imp__AddVectoredContinueHandler@8"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@_",                           \
                                                        "@quickcpplib@@YGKPAX@Z=__imp__RemoveVectoredContinueHandler@4"))
#elif defined(__arm__) || defined(_M_ARM)
#pragma comment(linker,                                                                                                                                        \
                QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RaiseException@win32@detail@signal_guard@_", "@quickcpplib@@YAXKKKPB_K@Z=RaiseException"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@_",                             \
                                                        "@quickcpplib@@YAP6AJPAU_EXCEPTION_POINTERS@12345@@ZP6AJ0@Z@Z=SetUnhandledExceptionFilter"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@_",                              \
                                                        "@quickcpplib@@YAPAXKP6AJPAU_EXCEPTION_POINTERS@12345@@Z@Z=AddVectoredContinueHandler"))
#pragma comment(linker, QUICKCPPLIB_SIGNAL_GUARD_SYMBOL("/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@_",                           \
                                                        "@quickcpplib@@YAKPAX@Z=RemoveVectoredContinueHandler"))
#else
#error Unknown architecture
#endif
#else
#if(defined(__x86_64__) || defined(_M_X64)) || (defined(__aarch64__) || defined(_M_ARM64))
#pragma comment(linker, "/alternatename:?RaiseException@win32@detail@signal_guard@quickcpplib@@YAXKKKPEB_K@Z=RaiseException")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@quickcpplib@@YAP6AJPEAU_EXCEPTION_POINTERS@12345@@ZP6AJ0@Z@Z=SetUnhandledExceptionFilter")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YAPEAXKP6AJPEAU_EXCEPTION_POINTERS@12345@@Z@Z=AddVectoredContinueHandler")
#pragma comment(linker, "/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YAKPEAX@Z=RemoveVectoredContinueHandler")
#elif defined(__x86__) || defined(_M_IX86) || defined(__i386__)
#pragma comment(linker, "/alternatename:?RaiseException@win32@detail@signal_guard@quickcpplib@@YGXKKKPB_K@Z=__imp__RaiseException@16")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@quickcpplib@@YGP6GJPAU_EXCEPTION_POINTERS@@@ZP6GJ0@Z@Z=__imp__SetUnhandledExceptionFilter@4")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YGPAXKP6GJPAU_EXCEPTION_POINTERS@@@Z@Z=__imp__AddVectoredContinueHandler@8")
#pragma comment(linker, "/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YGKPAX@Z=__imp__RemoveVectoredContinueHandler@4")
#elif defined(__arm__) || defined(_M_ARM)
#pragma comment(linker, "/alternatename:?RaiseException@win32@detail@signal_guard@quickcpplib@@YAXKKKPB_K@Z=RaiseException")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?SetUnhandledExceptionFilter@win32@detail@signal_guard@quickcpplib@@YAP6AJPAU_EXCEPTION_POINTERS@12345@@ZP6AJ0@Z@Z=SetUnhandledExceptionFilter")
#pragma comment(                                                                                                                                               \
linker,                                                                                                                                                        \
"/alternatename:?AddVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YAPAXKP6AJPAU_EXCEPTION_POINTERS@12345@@Z@Z=AddVectoredContinueHandler")
#pragma comment(linker, "/alternatename:?RemoveVectoredContinueHandler@win32@detail@signal_guard@quickcpplib@@YAKPAX@Z=RemoveVectoredContinueHandler")
#else
#error Unknown architecture
#endif
#endif
    }  // namespace win32
    inline unsigned long win32_exception_code_from_signalc(signalc c)
    {
      switch(c)
      {
      default:
        abort();
      case signalc::abort_process:
        return ((unsigned long) 0xC0000025L) /*EXCEPTION_NONCONTINUABLE_EXCEPTION*/;
      case signalc::undefined_memory_access:
        return ((unsigned long) 0xC0000006L) /*EXCEPTION_IN_PAGE_ERROR*/;
      case signalc::illegal_instruction:
        return ((unsigned long) 0xC000001DL) /*EXCEPTION_ILLEGAL_INSTRUCTION*/;
      // case signalc::interrupt:
      //  return SIGINT;
      // case signalc::broken_pipe:
      //  return SIGPIPE;
      case signalc::segmentation_fault:
        return ((unsigned long) 0xC0000005L) /*EXCEPTION_ACCESS_VIOLATION*/;
      case signalc::floating_point_error:
        return ((unsigned long) 0xC0000090L) /*EXCEPTION_FLT_INVALID_OPERATION*/;
      }
    }
    inline signalc signalc_from_win32_exception_code(unsigned long c)
    {
      switch(c)
      {
      case((unsigned long) 0xC0000025L) /*EXCEPTION_NONCONTINUABLE_EXCEPTION*/:
        return signalc::abort_process;
      case((unsigned long) 0xC0000006L) /*EXCEPTION_IN_PAGE_ERROR*/:
        return signalc::undefined_memory_access;
      case((unsigned long) 0xC000001DL) /*EXCEPTION_ILLEGAL_INSTRUCTION*/:
        return signalc::illegal_instruction;
      // case SIGINT:
      //  return signalc::interrupt;
      // case SIGPIPE:
      //  return signalc::broken_pipe;
      case((unsigned long) 0xC0000005L) /*EXCEPTION_ACCESS_VIOLATION*/:
        return signalc::segmentation_fault;
      case((unsigned long) 0xC000008DL) /*EXCEPTION_FLT_DENORMAL_OPERAND*/:
      case((unsigned long) 0xC000008EL) /*EXCEPTION_FLT_DIVIDE_BY_ZERO*/:
      case((unsigned long) 0xC000008FL) /*EXCEPTION_FLT_INEXACT_RESULT*/:
      case((unsigned long) 0xC0000090L) /*EXCEPTION_FLT_INVALID_OPERATION*/:
      case((unsigned long) 0xC0000091L) /*EXCEPTION_FLT_OVERFLOW*/:
      case((unsigned long) 0xC0000092L) /*EXCEPTION_FLT_STACK_CHECK*/:
      case((unsigned long) 0xC0000093L) /*EXCEPTION_FLT_UNDERFLOW*/:
        return signalc::floating_point_error;
      case((unsigned long) 0xC00000FDL) /*EXCEPTION_STACK_OVERFLOW*/:
        return signalc::out_of_memory;
      default:
        return signalc::none;
      }
    }
    // Overload for when a Win32 exception is being raised
    inline bool set_siginfo(thread_local_signal_guard *g, unsigned long code, win32::PEXCEPTION_RECORD raw_info, win32::PCONTEXT raw_context)
    {
      auto signo = signalc_from_win32_exception_code(code);
      auto signalset = static_cast<signalc_set>(1ULL << static_cast<int>(signo));
      if(g->guarded & signalset)
      {
        g->info.signo = static_cast<int>(signo);
        g->info.error_code = (long) raw_info->ExceptionInformation[2];
        g->info.addr = (void *) raw_info->ExceptionInformation[1];
        g->info.value.ptr_value = nullptr;
        g->info.raw_info = raw_info;
        g->info.raw_context = raw_context;
        return true;
      }
      return false;
    }
    SIGNALGUARD_FUNC_DECL long win32_exception_filter_function(unsigned long code, win32::_EXCEPTION_POINTERS *ptrs) noexcept
    {
      for(auto *shi = current_thread_local_signal_handler(); shi != nullptr; shi = shi->previous)
      {
        if(set_siginfo(shi, code, ptrs->ExceptionRecord, ptrs->ContextRecord))
        {
          if(shi->call_continuer())
          {
            // continue execution
            return (long) -1 /*EXCEPTION_CONTINUE_EXECUTION*/;
          }
          else
          {
            // invoke longjmp
            return 1 /*EXCEPTION_EXECUTE_HANDLER*/;
          }
        }
      }
      return 0 /*EXCEPTION_CONTINUE_SEARCH*/;
    }
    SIGNALGUARD_FUNC_DECL long __stdcall win32_vectored_exception_function(win32::_EXCEPTION_POINTERS *ptrs) noexcept
    {
      lock.lock();
      if(global_signal_deciders_front != nullptr)
      {
        auto *raw_info = ptrs->ExceptionRecord;
        auto *raw_context = ptrs->ContextRecord;
        const auto signo = signalc_from_win32_exception_code(raw_info->ExceptionCode);
        const signalc_set signo_set = 1ULL << static_cast<int>(signo);
        raised_signal_info rsi;
        memset(&rsi, 0, sizeof(rsi));
        rsi.signo = static_cast<int>(signo);
        rsi.error_code = (long) raw_info->ExceptionInformation[2];
        rsi.addr = (void *) raw_info->ExceptionInformation[1];
        rsi.raw_info = raw_info;
        rsi.raw_context = raw_context;
        auto *d = global_signal_deciders_front;
        for(size_t n = 0; d != nullptr; n++)
        {
          size_t i = 0;
          for(d = global_signal_deciders_front; d != nullptr; d = d->next)
          {
            if(i++ == n)
            {
              if(d->guarded & signo_set)
              {
                rsi.value = d->value;
                lock.unlock();
                if(d->decider(&rsi))
                {
                  return (long) -1 /*EXCEPTION_CONTINUE_EXECUTION*/;
                }
                lock.lock();
              }
              break;
            }
          }
        }
      }
      lock.unlock();
      return 0 /*EXCEPTION_CONTINUE_SEARCH*/;
    }
#else
    struct installed_signal_handler
    {
      unsigned count;  // number of signal_install instances for this signal
      struct sigaction former;
    };
    static installed_signal_handler handler_counts[32];  // indexed by signal number

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
        // Ok, need to invoke the default handler. NOTE: The following code is racy wrt other threads manipulating the global signal handlers
        struct sigaction dfl, myformer;
        memset(&dfl, 0, sizeof(dfl));
        dfl.sa_handler = SIG_DFL;
        sigaction(signo, &dfl, &myformer);  // restore action to default
        // Unblock this signal for this thread
        sigset_t myformer2;
        sigset_t def2;
        sigemptyset(&def2);
        sigaddset(&def2, signo);
        pthread_sigmask(SIG_UNBLOCK, &def2, &myformer2);
        // Raise this signal for this thread, invoking the default action
        pthread_kill(pthread_self(), signo);   // Very likely never returns
        sigaction(signo, &myformer, nullptr);  // but if it does, restore the previous action
        return true;                           // and return true to indicate that we executed some signal handler
      }
      else if(sa.sa_flags & SA_SIGINFO)
      {
        h1 = sa.sa_sigaction;
        if(nullptr != _info)
        {
          info = *_info;
        }
        if(nullptr != _context)
        {
          context = *(const ucontext_t *) _context;
        }
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
        h1(signo, (nullptr != _info) ? &info : nullptr, (nullptr != _context) ? &context : nullptr);
      else
        h2(signo);
      pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
      return true;
    }

    // Overload for when a POSIX signal is being raised
    inline bool set_siginfo(thread_local_signal_guard *g, int signo, siginfo_t *raw_info, void *raw_context)
    {
      auto signalset = static_cast<signalc_set>(1ULL << signo);
      if(g->guarded & signalset)
      {
        g->info.signo = signo;
        g->info.error_code = (nullptr != raw_info) ? raw_info->si_errno : 0;
        g->info.addr = (nullptr != raw_info) ? raw_info->si_addr : nullptr;
        g->info.value.ptr_value = nullptr;
        g->info.raw_info = raw_info;
        g->info.raw_context = raw_context;
        return true;
      }
      return false;
    }
    // Called by POSIX to handle a raised signal
    inline void raw_signal_handler(int signo, siginfo_t *info, void *context)
    {
      // std::cout << "raw_signal_handler(" << signo << ", " << info << ", " << context << ") shi=" << shi << std::endl;
      for(auto *shi = current_thread_local_signal_handler(); shi != nullptr; shi = shi->previous)
      {
        if(set_siginfo(shi, signo, info, context))
        {
          if(!shi->call_continuer())
          {
            longjmp(shi->info.buf, 1);
          }
        }
      }
      lock.lock();
      if(global_signal_deciders_front != nullptr)
      {
        raised_signal_info rsi;
        memset(&rsi, 0, sizeof(rsi));
        rsi.signo = signo;
        rsi.error_code = (nullptr != info) ? info->si_errno : 0;
        rsi.addr = (nullptr != info) ? info->si_addr : nullptr;
        rsi.raw_info = info;
        rsi.raw_context = context;
        auto *d = global_signal_deciders_front;
        for(size_t n = 0; d != nullptr; n++)
        {
          size_t i = 0;
          for(d = global_signal_deciders_front; d != nullptr; d = d->next)
          {
            if(i++ == n)
            {
              if(d->guarded & (1ULL << signo))
              {
                rsi.value = d->value;
                lock.unlock();
                if(d->decider(&rsi))
                {
                  return;  // resume execution
                }
                lock.lock();
              }
              break;
            }
          }
        }
      }
      // Otherwise, call the previous signal handler
      if(handler_counts[signo].count > 0)
      {
        struct sigaction sa;
        sa = handler_counts[signo].former;
        lock.unlock();
        do_raise_signal(signo, sa, info, context);
      }
      else
      {
        lock.unlock();
      }
    }
#endif
  }  // namespace detail

  SIGNALGUARD_MEMFUNC_DECL signal_guard_install::signal_guard_install(signalc_set guarded)
      : _guarded(guarded)
  {
#ifndef _WIN32
    sigset_t set;
    sigemptyset(&set);
    detail::lock.lock();
    for(int signo = 0; signo < 32; signo++)
    {
      if((static_cast<uint64_t>(guarded) & (1 << signo)) != 0)
      {
        if(!detail::handler_counts[signo].count++)
        {
          struct sigaction sa;
          memset(&sa, 0, sizeof(sa));
          sa.sa_sigaction = detail::raw_signal_handler;
          sa.sa_flags = SA_SIGINFO | SA_NODEFER;
          if(-1 == sigaction(signo, &sa, &detail::handler_counts[signo].former))
          {
            detail::handler_counts[signo].count--;
            detail::lock.unlock();
            throw std::system_error(errno, std::system_category());
          }
        }
      }
      if(detail::handler_counts[signo].count > 0)
      {
        sigaddset(&set, signo);
      }
    }
    // Globally enable all signals we have installed handlers for
    if(-1 == sigprocmask(SIG_UNBLOCK, &set, nullptr))
    {
      detail::lock.unlock();
      throw std::system_error(errno, std::system_category());
    }
    detail::signal_guards_installed().store(detail::signal_guards_installed().load(std::memory_order_relaxed) | guarded, std::memory_order_relaxed);
    detail::lock.unlock();
#endif
    if((guarded & signalc_set::out_of_memory) || (guarded & signalc_set::termination))
    {
      detail::lock.lock();
      if(guarded & signalc_set::out_of_memory)
      {
        if(!detail::new_handler_count++)
        {
          detail::new_handler_old = std::set_new_handler(detail::new_handler);
        }
      }
      if(guarded & signalc_set::termination)
      {
        if(!detail::terminate_handler_count++)
        {
#ifndef _MSC_VER
          detail::terminate_handler_old = std::set_terminate(detail::terminate_handler);
#endif
        }
      }
      detail::lock.unlock();
    }
  }

  SIGNALGUARD_MEMFUNC_DECL signal_guard_install::~signal_guard_install()
  {
    if((_guarded & signalc_set::out_of_memory) || (_guarded & signalc_set::termination))
    {
      detail::lock.lock();
      if(_guarded & signalc_set::out_of_memory)
      {
        if(!--detail::new_handler_count)
        {
          std::set_new_handler(detail::new_handler_old);
        }
      }
      if(_guarded & signalc_set::termination)
      {
        if(!--detail::terminate_handler_count)
        {
#ifndef _MSC_VER
          std::set_terminate(detail::terminate_handler_old);
#endif
        }
      }
      detail::lock.unlock();
    }
#ifndef _WIN32
    uint64_t handlers_installed = 0;
    sigset_t set;
    sigemptyset(&set);
    bool setsigprocmask = false;
    detail::lock.lock();
    for(int signo = 0; signo < 32; signo++)
    {
      if((static_cast<uint64_t>(_guarded) & (1 << signo)) != 0)
      {
        int ret = 0;
        if(!--detail::handler_counts[signo].count)
        {
          ret = sigaction(signo, &detail::handler_counts[signo].former, nullptr);
          if(ret == -1)
          {
            detail::lock.unlock();
            abort();
            detail::lock.lock();
          }
          sigaddset(&set, signo);
          setsigprocmask = true;
        }
      }
      if(detail::handler_counts[signo].count > 0)
      {
        handlers_installed |= (1ULL << signo);
      }
    }
    if(detail::new_handler_count > 0)
    {
      handlers_installed |= signalc_set::out_of_memory;
    }
    if(detail::terminate_handler_count > 0)
    {
      handlers_installed |= signalc_set::termination;
    }
    detail::signal_guards_installed().store(static_cast<signalc_set>(handlers_installed), std::memory_order_relaxed);
    detail::lock.unlock();
    if(setsigprocmask)
    {
      sigprocmask(SIG_BLOCK, &set, nullptr);
    }
#endif
  }

  namespace detail
  {
    static inline bool signal_guard_global_decider_impl_indirect(raised_signal_info *rsi)
    {
      auto *p = (signal_guard_global_decider_impl *) rsi->value.ptr_value;
      return p->_call(rsi);
    }
    SIGNALGUARD_MEMFUNC_DECL signal_guard_global_decider_impl::signal_guard_global_decider_impl(signalc_set guarded, bool callfirst)
        : _install(guarded)
    {
      auto *p = new global_signal_decider;
      _p = p;
      p->guarded = guarded;
      p->decider = signal_guard_global_decider_impl_indirect;
      p->value.ptr_value = this;
      lock.lock();
      global_signal_decider **a, **b;
      if(global_signal_deciders_front == nullptr)
      {
        a = &global_signal_deciders_front;
        b = &global_signal_deciders_back;
      }
      else if(callfirst)
      {
        a = &global_signal_deciders_front;
        b = &global_signal_deciders_front->prev;
      }
      else
      {
        a = &global_signal_deciders_back->next;
        b = &global_signal_deciders_back;
      }
      p->next = *a;
      p->prev = *b;
      *a = p;
      *b = p;
#ifdef _WIN32
      if(nullptr == win32_global_signal_decider2)
      {
        /* The interaction between AddVectoredContinueHandler, AddVectoredExceptionHandler,
        UnhandledExceptionFilter, and frame-based EH is completely undocumented in Microsoft
        documentation. The following is the truth, as determined by empirical testing:

        1. Vectored exception handlers get called first, before anything else, including
        frame-based EH. This is not what the MSDN documentation hints at.

        2. Frame-based EH filters are now run.

        3. UnhandledExceptionFilter() is now called. On older Windows, this invokes the
        debugger if being run under the debugger, otherwise continues search. But as of
        at least Windows 7 onwards, if no debugger is attached, it invokes Windows
        Error Reporting to send a core dump to Microsoft.

        4. Vectored continue handlers now get called, AFTER the frame-based EH. Again,
        not what MSDN hints at.


        The change in the default non-debugger behaviour of UnhandledExceptionFilter()
        effectively makes vectored continue handlers useless. I suspect whomever made
        the change at Microsoft didn't realise that vectored continue handlers are
        invoked AFTER the unhandled exception filter, because that's really non-obvious
        from the documentation.

        Anyway this is why we install for both the continue handler and the unhandled
        exception filters. The unhandled exception filter will be called when not running
        under a debugger. The vectored continue handler will be called when running
        under a debugger, as the UnhandledExceptionFilter() function never calls the
        installed unhandled exception filter function if under a debugger.
        */
        win32_global_signal_decider1 = (void *) win32::SetUnhandledExceptionFilter(win32_vectored_exception_function);
        win32_global_signal_decider2 = (void *) win32::AddVectoredContinueHandler(1, win32_vectored_exception_function);
      }
#endif
      lock.unlock();
    }

    SIGNALGUARD_MEMFUNC_DECL signal_guard_global_decider_impl::signal_guard_global_decider_impl(signal_guard_global_decider_impl &&o) noexcept
        : _install(static_cast<signal_guard_global_decider_impl &&>(o)._install)
        , _p(o._p)
    {
      lock.lock();
      auto *p = (global_signal_decider *) _p;
      p->value.ptr_value = this;
      lock.unlock();
      o._p = nullptr;
    }

    SIGNALGUARD_MEMFUNC_DECL signal_guard_global_decider_impl::~signal_guard_global_decider_impl()
    {
      if(_p != nullptr)
      {
        auto *p = (global_signal_decider *) _p;
        lock.lock();
        global_signal_decider **a = (nullptr == p->prev) ? &global_signal_deciders_front : &p->prev->next;
        global_signal_decider **b = (nullptr == p->next) ? &global_signal_deciders_back : &p->next->prev;
        *a = p->next;
        *b = p->prev;
#ifdef _WIN32
        if(nullptr == global_signal_deciders_front)
        {
          win32::RemoveVectoredContinueHandler(win32_global_signal_decider2);
          win32::SetUnhandledExceptionFilter((win32::PVECTORED_EXCEPTION_HANDLER) win32_global_signal_decider1);
          win32_global_signal_decider1 = nullptr;
          win32_global_signal_decider2 = nullptr;
        }
#endif
        lock.unlock();
        delete p;
        _p = nullptr;
      }
    }
  }  // namespace detail

  SIGNALGUARD_FUNC_DECL bool thrd_raise_signal(signalc signo, void *_info, void *_context)
  {
    if(signo == signalc::out_of_memory)
    {
      if(std::get_new_handler() == nullptr)
      {
        std::terminate();
      }
      std::get_new_handler()();
      return true;
    }
    else if(signo == signalc::termination)
    {
      std::terminate();
    }
#ifdef _WIN32
    using detail::win32::_EXCEPTION_RECORD;
    using detail::win32::RaiseException;
    auto win32sehcode = detail::win32_exception_code_from_signalc(signo);
    if((unsigned long) -1 == win32sehcode)
      throw std::runtime_error("Unknown signal");
    (void) _context;
    const auto *info = (const _EXCEPTION_RECORD *) _info;
    // info->ExceptionInformation[0] = 0=read 1=write 8=DEP
    // info->ExceptionInformation[1] = causing address
    // info->ExceptionInformation[2] = NTSTATUS causing exception
    if(info != nullptr)
    {
      RaiseException(win32sehcode, info->ExceptionFlags, info->NumberParameters, info->ExceptionInformation);
    }
    else
    {
      RaiseException(win32sehcode, 0, 0, nullptr);
    }
    return false;
#else
    auto *info = (siginfo_t *) _info;
    // Fetch the current handler, and simulate the raise
    struct sigaction sa;
    sigaction(static_cast<int>(signo), nullptr, &sa);
    return detail::do_raise_signal(static_cast<int>(signo), sa, info, _context);
#endif
  }

}  // namespace signal_guard

QUICKCPPLIB_NAMESPACE_END
