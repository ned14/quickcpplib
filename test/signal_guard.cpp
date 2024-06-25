/* Test the signal guard
(C) 2018 - 2021 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#define _CRT_SECURE_NO_WARNINGS 1

#include "../include/quickcpplib/signal_guard.hpp"

#include "../include/quickcpplib/scope.hpp"
#include "../include/quickcpplib/span.hpp"
#include "../include/quickcpplib/string_view.hpp"
#ifdef _WIN32
#include "../include/quickcpplib/execinfo_win64.h"
#else
#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <fenv.h>
#include <spawn.h>
#include <sys/wait.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
extern "C" char **environ;
#endif
#ifdef __APPLE__
#include <cfenv>
#endif
#endif

#include "../include/quickcpplib/boost/test/unit_test.hpp"

#include <fenv.h>

#include "timing.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-attributes"
#endif

BOOST_AUTO_TEST_SUITE(signal_guard)

BOOST_AUTO_TEST_CASE(signal_guard / works / threadlocal, "Tests that signal_guard works as advertised (thread local)")
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  signal_guard_install i(signalc_set::segmentation_fault | signalc_set::cxx_termination);
  std::cout << "1" << std::endl;
  {
    int ret = signal_guard(
    signalc_set::segmentation_fault,
    []()
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((no_sanitize_undefined))
#endif
    {
      volatile int *a = nullptr;
      return *a;
    },
    [](const raised_signal_info * /*unused*/) -> int { return 78; });
    BOOST_CHECK(ret == 78);
  }
  std::cout << "2" << std::endl;
  {
    int ret = signal_guard(
    signalc_set::cxx_termination, []() -> int { std::terminate(); },
    [](const raised_signal_info * /*unused*/) -> int { return 78; });
    BOOST_CHECK(ret == 78);
  }
  std::cout << "3" << std::endl;
  {
    int ret = signal_guard(
    signalc_set::segmentation_fault,
    []() -> int
    {
      thrd_raise_signal(signalc::segmentation_fault);
      return 5;
    },
    [](const raised_signal_info * /*unused*/) -> int { return 78; });
    BOOST_CHECK(ret == 78);
  }
  std::cout << "4" << std::endl;
}

BOOST_AUTO_TEST_CASE(signal_guard / works / global, "Tests that signal_guard works as advertised (global)")
{
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  std::cout << "1" << std::endl;
  jmp_buf buf;
  auto decider =
  make_signal_guard_global_decider(signalc_set::segmentation_fault | signalc_set::cxx_termination,
                                   [&buf](raised_signal_info * /*unused*/) -> bool { longjmp(buf, 78); });
  auto ret = setjmp(buf);
  if(!ret)
  {
    []()
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((no_sanitize_undefined))
#endif
    {
      volatile int *a = nullptr;
      (void) *a;
    }
    ();
  }
  BOOST_CHECK(ret == 78);
  std::cout << "2" << std::endl;
  ret = setjmp(buf);
  if(!ret)
  {
    std::terminate();
  }
  BOOST_CHECK(ret == 78);
  std::cout << "3" << std::endl;
  ret = setjmp(buf);
  if(!ret)
  {
    thrd_raise_signal(signalc::segmentation_fault);
  }
  BOOST_CHECK(ret == 78);
  std::cout << "4" << std::endl;
}

BOOST_AUTO_TEST_CASE(signal_guard / works / watchdog, "Tests that the signal_guard_watchdog works as expected")
{
#ifdef __APPLE__
  return;  // Lack of POSIX timers on Mac OS means this test fails
#endif
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  {
    std::atomic<bool> watchdog_ran{false};
    auto watchdog = make_signal_guard_watchdog([&] { watchdog_ran = true; }, 500);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    BOOST_CHECK(watchdog_ran == true);
  }
  {
    std::atomic<bool> watchdog_ran{false};
    auto watchdog = make_signal_guard_watchdog([&] { watchdog_ran = true; }, 500);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    watchdog.release();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    BOOST_CHECK(watchdog_ran == false);
  }
}

// This routine is async signal safe, apart from malloc. Probably okay most of the time ?!?
template <class Printer>
inline void _symbolise_stack_backtrace(Printer &&print, QUICKCPPLIB_NAMESPACE::span::span<void *> bt)
{
#ifdef _WIN32
  // There isn't an implementation of backtrace_symbols_fd() for Windows, so
  // the following is not async signal safe, it calls malloc().
  char **strings = backtrace_symbols(bt.data(), bt.size());
  for(size_t n = 0; n < bt.size(); n++)
  {
    print("\n   ");
    print(strings[n]);
  }
  free(strings);
  print("\n");
#else
  int done = 0;
#if 1
  // Try llvm-symbolizer for nicer backtraces. Note that all this is async signal
  // unsafe, and may well hang or confound the backtrace.
  int temp[2];
  struct _h
  {
    int fd;
  } childreadh, childwriteh, readh, writeh;
  if(-1 != ::pipe(temp))
  {
    childreadh.fd = temp[0];
    readh.fd = temp[1];
    if(-1 != ::pipe(temp))
    {
      writeh.fd = temp[0];
      childwriteh.fd = temp[1];
      auto unmypipes = QUICKCPPLIB_NAMESPACE::scope::make_scope_exit(
      [&]() noexcept
      {
        (void) ::close(readh.fd);
        (void) ::close(writeh.fd);
      });
      auto unhispipes = QUICKCPPLIB_NAMESPACE::scope::make_scope_exit(
      [&]() noexcept
      {
        (void) ::close(childreadh.fd);
        (void) ::close(childwriteh.fd);
      });
      (void) ::fcntl(readh.fd, F_SETFD, FD_CLOEXEC);
      (void) ::fcntl(writeh.fd, F_SETFD, FD_CLOEXEC);

      posix_spawn_file_actions_t child_fd_actions;
      if(!::posix_spawn_file_actions_init(&child_fd_actions))
      {
        auto unactions = QUICKCPPLIB_NAMESPACE::scope::make_scope_exit(
        [&]() noexcept { ::posix_spawn_file_actions_destroy(&child_fd_actions); });
        if(!::posix_spawn_file_actions_adddup2(&child_fd_actions, childreadh.fd, STDIN_FILENO))
        {
          if(!::posix_spawn_file_actions_addclose(&child_fd_actions, childreadh.fd))
          {
            if(!::posix_spawn_file_actions_adddup2(&child_fd_actions, childwriteh.fd, STDOUT_FILENO))
            {
              if(!::posix_spawn_file_actions_addclose(&child_fd_actions, childwriteh.fd))
              {
                pid_t pid;
                std::vector<const char *> argptrs(2);
                argptrs[0] = "llvm-symbolizer";
                if(!::posix_spawnp(&pid, "llvm-symbolizer", &child_fd_actions, nullptr, (char **) argptrs.data(),
                                   environ))
                {
                  (void) ::close(childreadh.fd);
                  (void) ::close(childwriteh.fd);
                  std::string addrs;
                  addrs.reserve(1024);
                  for(size_t n = 0; n < bt.size(); n++)
                  {
                    Dl_info info;
                    memset(&info, 0, sizeof(info));
                    ::dladdr(bt[n], &info);
                    if(info.dli_fname == nullptr)
                    {
                      break;
                    }
                    // std::cerr << bt[n] << " dli_fname = " << info.dli_fname << " dli_fbase = " << info.dli_fbase
                    //          << std::endl;
                    addrs.append(info.dli_fname);
                    addrs.append(" 0x");
                    const bool has_slash = (strrchr(info.dli_fname, '/') != nullptr);
                    const bool is_dll = (strstr(info.dli_fname, ".so") != nullptr);
                    if(has_slash)
                    {
                      ssize_t diff;
                      if(is_dll)
                      {
                        diff = (char *) bt[n] - (char *) info.dli_fbase;
                      }
                      else
                      {
                        diff = (ssize_t) bt[n];
                      }
                      char buffer[32];
                      snprintf(buffer, 32, "%zx", diff);
                      addrs.append(buffer);
                    }
                    else
                    {
                      char buffer[32];
                      snprintf(buffer, 32, "%zx", (uintptr_t) bt[n]);
                      addrs.append(buffer);
                    }
                    addrs.push_back('\n');
                  }
                  // std::cerr << "\n\n---\n" << addrs << "---\n\n" << std::endl;
                  // Suppress SIGPIPE
                  sigset_t toblock, oldset;
                  sigemptyset(&toblock);
                  sigaddset(&toblock, SIGPIPE);
                  pthread_sigmask(SIG_BLOCK, &toblock, &oldset);
                  auto unsigmask = QUICKCPPLIB_NAMESPACE::scope::make_scope_exit(
                  [&toblock, &oldset]() noexcept
                  {
#ifdef __APPLE__
                    pthread_kill(pthread_self(), SIGPIPE);
                    int cleared = 0;
                    sigwait(&toblock, &cleared);
#else
                    struct timespec ts = {0, 0};
                    sigtimedwait(&toblock, 0, &ts);
#endif
                    pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
                  });
                  (void) unsigmask;
                  ssize_t written = ::write(readh.fd, addrs.data(), addrs.size());
                  (void) ::close(readh.fd);
                  addrs.clear();
                  if(written != -1)
                  {
                    char buffer[1024];
                    for(;;)
                    {
                      auto bytes = ::read(writeh.fd, buffer, sizeof(buffer));
                      if(bytes < 1)
                        break;
                      addrs.append(buffer, bytes);
                    }
                    (void) ::close(writeh.fd);
                    unmypipes.release();
                    unhispipes.release();
                  }
                  // std::cerr << "\n\n---\n" << addrs << "---\n\n" << std::endl;
                  // reap child
                  siginfo_t info;
                  memset(&info, 0, sizeof(info));
                  int options = WEXITED | WSTOPPED;
                  if(-1 == ::waitid(P_PID, pid, &info, options))
                    abort();
                  if(!addrs.empty())
                  {
                    // We want the second line from every section separated by a double newline
                    size_t n = 0;
                    done = 1;
                    auto printitem = [&](size_t idx)
                    {
                      print("\n   ");
                      auto idx2 = addrs.find(10, idx), idx3 = addrs.find(10, idx2 + 1);
                      QUICKCPPLIB_NAMESPACE::string_view::string_view sv(addrs.data() + idx2 + 1, idx3 - idx2 - 1);
                      if(sv.size() == 6 && 0 == memcmp(sv.data(), "??:0:0", 6))
                      {
                        char **s = backtrace_symbols(&bt[n], 1);
                        print(s[0]);
                        free(s);
                        done = 2;
                      }
                      else
                      {
                        print(sv.data(), sv.size());
                      }
                      n++;
                    };
                    size_t oldidx = 0;
                    for(size_t idx = addrs.find("\n\n"); idx != std::string::npos;
                        oldidx = idx + 2, idx = addrs.find("\n\n", idx + 1))
                    {
                      printitem(oldidx);
                    }
                    print("\n");
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  if(done == 2)
  {
    print("\nllvm-symbolizer could not find or parse the debug info for at least one\nstack frame above. You "
          "may wish to consider telling gcc to use older\nformat debug info, or else use a newer "
          "llvm-symbolizer. Or install\nyour system's debug symbols, and enable debug info during build!\n");
  }
#endif
  if(done == 0)
  {
    if(!bt.empty())
    {
      char **s = ::backtrace_symbols(bt.data(), bt.size());
      for(size_t n = 0; n < bt.size(); n++)
      {
        print("\n   ");
        print(s[n]);
      }
      free(s);
      print("\n\nllvm-symbolizer was not found on PATH, so raw data is given above.\nTo figure out the source "
            "file and line "
            "number, call\n\nllvm-symbolizer --obj=<binary path> <offset "
            "address>\n\nfor any line printed above, or\n\naddr2line -e <binary path> <offset address>\n\nwill "
            "also work.\n");
    }
  }
#endif
}

BOOST_AUTO_TEST_CASE(signal_guard / works / multithreaded,
                     "Tests that signal_guard works as advertised across multiple threads")
{
#if QUICKCPPLIB_IN_THREAD_SANITIZER
  return;  // hangs tsan, indeed you can't even Ctrl-C out of it!
#endif
#ifdef _WIN32
  if(getenv("CI") != nullptr)
  {
    return;
  }
#endif
  static thread_local jmp_buf buf;
  static std::atomic<bool> done(false);
  auto handler = QUICKCPPLIB_NAMESPACE::signal_guard::make_signal_guard_global_decider(
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::abort_process |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::undefined_memory_access |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::illegal_instruction |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::segmentation_fault |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::floating_point_error |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::cxx_out_of_memory |
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::cxx_termination,
  [](QUICKCPPLIB_NAMESPACE::signal_guard::raised_signal_info *rsi) -> bool
  {
    void *bt[64];
    auto btlen = ::backtrace(bt, 64);
    auto print = [](const char *s, size_t len = (size_t) -1)
    {
      if(len == (size_t) -1)
      {
        len = strlen(s);
      }
#ifdef _WIN32
      using namespace win32;
      unsigned long written = 0;
      (void) WriteFile(GetStdHandle((unsigned long) -12 /*STD_ERROR_HANDLE*/), s, (unsigned long) len, &written,
                       nullptr);
#else
      if(-1 == ::write(2, s, len))
      {
        abort();
      }
#endif
    };
    print("FATAL: I experienced unrecoverable failure '");
    print(QUICKCPPLIB_NAMESPACE::signal_guard::detail::signalc_to_string(
    static_cast<QUICKCPPLIB_NAMESPACE::signal_guard::signalc>(rsi->signo)));
    print("'. Backtrace:\n");
    _symbolise_stack_backtrace(print, {bt, (size_t) btlen});
    print("\n");
    longjmp(buf, 10 + done);
  },
  false);
  std::vector<std::thread> threads;
  static std::atomic<unsigned> count(0);
  for(size_t n = 0; n < std::thread::hardware_concurrency(); n++)
  {
    threads.emplace_back(
    []
    {
      auto ret = setjmp(buf);
      if(!ret || ret == 10)
      {
        ++count;
        []()
#if defined(__GNUC__) || defined(__clang__)
        __attribute__((no_sanitize_undefined))
#endif
        {
          volatile int *a = nullptr;
          (void) *a;
        }
        ();
      }
    });
  }
#ifdef _MSC_VER
  // MSVC finds exception raises across many threads very slow to do
  std::this_thread::sleep_for(std::chrono::seconds((getenv("CI") != nullptr) ? 1 : 3));
#else
  std::this_thread::sleep_for(std::chrono::seconds(10));
#endif
  done = true;
  for(auto &i : threads)
  {
    i.join();
  }
  BOOST_CHECK(count > 10);
}

BOOST_AUTO_TEST_CASE(signal_guard / works / recursive,
                     "Tests that signal_guard works as advertised when being recursed into across multiple threads")
{
#if QUICKCPPLIB_IN_THREAD_SANITIZER
  return;  // hangs tsan, indeed you can't even Ctrl-C out of it!
#endif
  static thread_local jmp_buf buf;
  static std::atomic<bool> done(false);
  static auto print = [](const char *s, size_t len = (size_t) -1)
  {
    if(len == (size_t) -1)
    {
      len = strlen(s);
    }
#ifdef _WIN32
    using namespace win32;
    unsigned long written = 0;
    (void) WriteFile(GetStdHandle((unsigned long) -12 /*STD_ERROR_HANDLE*/), s, (unsigned long) len, &written, nullptr);
#else
    if(-1 == ::write(2, s, len))
    {
      abort();
    }
#endif
  };
  auto handler1 = QUICKCPPLIB_NAMESPACE::signal_guard::make_signal_guard_global_decider(
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::segmentation_fault,
  [](QUICKCPPLIB_NAMESPACE::signal_guard::raised_signal_info * /*unused*/) -> bool
  {
    // Throw an exception within a noexcept to trigger termination
    print("During handle of segfault, causing floating point error\n");
#ifdef _MSC_VER
    _controlfp(0, _MCW_EM);
#else
#ifdef __APPLE__
    auto feenableexcept = [](int excepts)
    {
      excepts = excepts & FE_ALL_EXCEPT;
      fenv_t fenv;
      fegetenv(&fenv);
#ifdef __x86_64__
      fenv.__control &= ~excepts;
      fenv.__mxcsr &= ~(excepts << 7);
#elif defined(__arm__)
      fenv.__fpscr |= 1u << 9u; /* __fpscr_trap_enable_div_by_zero */
#elif defined(__aarch64__)
      fenv.__fpcr |= 1u << 9u; /* __fpcr_trap_enable_div_by_zero */
#else
#error "Unknown platform"
#endif
      fesetenv(&fenv);
    };
#endif
    feenableexcept(FE_DIVBYZERO);
#endif
    feraiseexcept(FE_DIVBYZERO);
    abort();
  },
  false);
  auto handler2 = QUICKCPPLIB_NAMESPACE::signal_guard::make_signal_guard_global_decider(
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::floating_point_error,
  [](QUICKCPPLIB_NAMESPACE::signal_guard::raised_signal_info * /*unused*/) -> bool
  {
    // Throw an exception within a noexcept to trigger termination
    print("During handle of segfault and handle of floating point error, causing out of memory\n");
    // new int[UINT64_MAX/sizeof(int)] does not do what I want, so ...
    std::get_new_handler()();
    abort();
  },
  false);
  auto handler3 = QUICKCPPLIB_NAMESPACE::signal_guard::make_signal_guard_global_decider(
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::cxx_out_of_memory,
  [](QUICKCPPLIB_NAMESPACE::signal_guard::raised_signal_info * /*unused*/) -> bool
  {
    // Throw an exception within a noexcept to trigger termination
    print(
    "During handle of segfault and handle of floating point error and handle of out of memory, causing termination\n");
    std::terminate();
  },
  false);
  auto handler4 = QUICKCPPLIB_NAMESPACE::signal_guard::make_signal_guard_global_decider(
  QUICKCPPLIB_NAMESPACE::signal_guard::signalc_set::cxx_termination,
  [](QUICKCPPLIB_NAMESPACE::signal_guard::raised_signal_info * /*unused*/) -> bool
  {
    print("Abandoning handle of termination via longjmp to restart the loop\n");
    longjmp(buf, 10 + done);
  },
  false);
  std::vector<std::thread> threads;
  static std::atomic<unsigned> count(0);
  for(size_t n = 0; n < std::thread::hardware_concurrency(); n++)
  {
    threads.emplace_back(
    []
    {
      auto ret = setjmp(buf);
      if(!ret || ret == 10)
      {
        ++count;
        print("Segfaulting this thread\n");
        []()
#if defined(__GNUC__) || defined(__clang__)
        __attribute__((no_sanitize_undefined))
#endif
        {
          volatile int *a = nullptr;
          (void) *a;  // segfault
        }
        ();
      }
    });
  }
#ifdef _MSC_VER
  // MSVC finds recursive exception raises across many threads very slow to do
  std::this_thread::sleep_for(std::chrono::seconds((getenv("CI") != nullptr) ? 1 : 3));
#else
  std::this_thread::sleep_for(std::chrono::seconds(10));
#endif
  done = true;
  for(auto &i : threads)
  {
    i.join();
  }
  BOOST_CHECK(count > 10);
}


BOOST_AUTO_TEST_CASE(signal_guard / performance / threadlocal,
                     "Tests that the signal_guard has reasonable performance (thread local)")
{
#if QUICKCPPLIB_IN_THREAD_SANITIZER
  return;  // hangs tsan, indeed you can't even Ctrl-C out of it!
#endif
  using namespace QUICKCPPLIB_NAMESPACE::signal_guard;
  {
    auto begin = nanoclock();
    while(nanoclock() - begin < 1000000000ULL)
      ;
  }
  uint64_t overhead = 99999999;
  {
    for(size_t n = 0; n < 100; n++)
    {
      volatile uint64_t begin = ticksclock();
      volatile uint64_t end = ticksclock();
      if(end - begin < overhead)
        overhead = end - begin;
    }
  }
  {
    signal_guard_install i(signalc_set::segmentation_fault);
    uint64_t ticks = 0;
    for(size_t n = 0; n < 128; n++)
    {
      uint64_t begin = ticksclock();
      volatile int ret = signal_guard(
      signalc_set::segmentation_fault, []() -> int { return 5; },
      [](const raised_signal_info * /*unused*/) -> int { return 78; });
      uint64_t end = ticksclock();
      (void) ret;
      // std::cout << (end - begin - overhead) << std::endl;
      ticks += end - begin - overhead;
    }
    std::cout << "It takes " << (ticks / 128) << " CPU ticks to execute successful code (overhead was " << overhead
              << ")" << std::endl;
  }
}


BOOST_AUTO_TEST_SUITE_END()
