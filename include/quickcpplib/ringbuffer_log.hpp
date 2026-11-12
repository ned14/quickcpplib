/* Very fast threadsafe ring buffer log
(C) 2016-2021 Niall Douglas <http://www.nedproductions.biz/> (21 commits)
File Created: Mar 2016


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

#ifndef QUICKCPPLIB_RINGBUFFER_LOG_HPP
#define QUICKCPPLIB_RINGBUFFER_LOG_HPP

#ifndef QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_DEBUG
#define QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_DEBUG 4096
#endif

#ifndef QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_NDEBUG
#define QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_NDEBUG 256
#endif

#ifdef NDEBUG
#define QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_NDEBUG
#else
#define QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_DEBUG
#endif

// If I'm on winclang, I can't stop the deprecation warnings from MSVCRT unless I do this
#if defined(_MSC_VER) && defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "packed_backtrace.hpp"
#include "scope.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>  // for ptrdiff_t
#include <cstdint>  // for uint32_t etc
#include <cstring>  // for memcmp
#include <iomanip>
#include <ostream>
#include <sstream>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#include "execinfo_win64.h"
#else
#include <dlfcn.h>
#if !QUICKCPPLIB_DISABLE_EXECINFO
#include <execinfo.h>
#endif
#include <fcntl.h>
#include <limits.h>
#include <signal.h>  // for siginfo_t
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
extern "C" char **environ;
#endif
#endif

QUICKCPPLIB_NAMESPACE_BEGIN

namespace ringbuffer_log
{
  //! Level of logged item
  enum class level : unsigned char
  {
    none = 0,
    fatal,
    error,
    warn,
    info,
    debug,
    all
  };
  //! A default ringbuffer log_level checker which returns whatever the log instance's level is
  struct default_ringbuffer_log_level_checker
  {
    level operator()(level v) const noexcept { return v; }
  };
  template <class Policy, class LogLevelChecker = default_ringbuffer_log_level_checker> class ringbuffer_log;

  //! Returns a const char * no more than 190 chars from its end
  template <class T> inline const char *last190(const T &v)
  {
    size_t size = v.size();
    return size <= 190 ? v.data() : v.data() + (size - 190);
  }
  namespace simple_ringbuffer_log_policy_detail
  {
    using level_ = level;
    struct value_type
    {
      uint64_t counter{0};
      uint64_t timestamp{0};
      union
      {
        uint32_t code32[2];
        uint64_t code64;
      };
      union
      {
        char backtrace[40];  // packed_backtrace
        char function[40];
      };
      uint8_t level : 4;
      uint8_t using_code64 : 1;
      uint8_t using_backtrace : 1;
      char message[191];

    private:
      static std::chrono::high_resolution_clock::time_point _first_item()
      {
        static std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        return now;
      }

    public:
      value_type() noexcept
          : code64{0}
          , backtrace{0}
          , level{0}
          , using_code64{0}
          , using_backtrace{0}
          , message{0}
      {
      }
      value_type(level_ _level, const char *_message, uint32_t _code1, uint32_t _code2, const char *_function = nullptr, unsigned lineno = 0)
          : counter((size_t) -1)
          , timestamp(std::chrono::duration_cast<std::chrono::nanoseconds>((_first_item(), std::chrono::high_resolution_clock::now() - _first_item())).count())
          , code32{_code1, _code2}
          , level(static_cast<uint8_t>(_level))
          , using_code64(false)
#if !QUICKCPPLIB_DISABLE_EXECINFO
          , using_backtrace(!_function)
#else
          , using_backtrace{0}
#endif
      {
        size_t _messagelen = strlen(_message) + 1;
        if(_messagelen > sizeof(message))
        {
          _messagelen = sizeof(message);
        }
        memcpy(message, _message, _messagelen);
        if(_function)
        {
          if(_function[0])
          {
            size_t _functionlen = strlen(_function) + 1;
            if(_functionlen > sizeof(function))
            {
              _functionlen = sizeof(function);
            }
            memcpy(function, _function, _functionlen);
            char temp[32], *e = function;
            for(size_t n = 0; n < sizeof(function) && *e != 0; n++, e++)
              ;
#ifdef _MSC_VER
            _ultoa_s(lineno, temp, 10);
#else
            snprintf(temp, sizeof(temp), "%u", lineno);
#endif
            temp[31] = 0;
            ptrdiff_t len = strlen(temp) + 1;
            if(function + sizeof(function) - e >= len + 1)
            {
              *e++ = ':';
              memcpy(e, temp, len);
            }
          }
          else
          {
            function[0] = 0;
          }
        }
        else
        {
          function[0] = 0;
#if !QUICKCPPLIB_DISABLE_EXECINFO
          const void *temp[16];
          memset(temp, 0, sizeof(temp));
          (void) ::backtrace((void **) temp, 16);
          packed_backtrace::make_packed_backtrace(backtrace, temp);
#endif
        }
      }
      // Field-wise, NOT memcmp over the whole object. The type has bitfield
      // padding, so two logically equal entries with different padding compared
      // UNEQUAL, and memcmp ordering compared the byte layout rather than the
      // values - on a little-endian machine `counter=256` sorted BEFORE
      // `counter=1` because the low byte is compared first (reviewer P0 r5:CH-1).
      bool operator==(const value_type &o) const noexcept
      {
        return counter == o.counter && timestamp == o.timestamp && code64 == o.code64 && level == o.level &&
               using_code64 == o.using_code64 && using_backtrace == o.using_backtrace &&
               memcmp(backtrace, o.backtrace, sizeof(backtrace)) == 0 && memcmp(message, o.message, sizeof(message)) == 0;
      }
      bool operator!=(const value_type &o) const noexcept { return !(*this == o); }
      bool operator<(const value_type &o) const noexcept
      {
        if(counter != o.counter)
          return counter < o.counter;
        if(timestamp != o.timestamp)
          return timestamp < o.timestamp;
        const int c = memcmp(message, o.message, sizeof(message));
        if(c != 0)
          return c < 0;
        return code64 < o.code64;
      }
      bool operator>(const value_type &o) const noexcept { return o < *this; }
      bool operator<=(const value_type &o) const noexcept { return !(o < *this); }
      bool operator>=(const value_type &o) const noexcept { return !(*this < o); }
    };
    static_assert(sizeof(value_type) == 256, "value_type is not 256 bytes long!");

    //! Location generator for simple_ringbuffer_log_policy's value_type
    inline std::string location(const value_type &v)
    {
      std::string ret;
      if(v.using_backtrace)
      {
        void *backtrace[16];
        memset(backtrace, 0, sizeof(backtrace));
        size_t len = 0;
        {
          packed_backtrace::packed_backtrace<> pb(v.backtrace);
          for(auto *i : pb)
          {
            backtrace[len] = i;
            len++;
            if(len == sizeof(backtrace) / sizeof(backtrace[0]))
              break;
          }
        }
#ifndef _WIN32
        bool done = false;
        // Try llvm-symbolizer for nicer backtraces
        int temp[2];
        struct _h
        {
          int fd;
        } childreadh, childwriteh, readh, writeh;
        if(-1 != ::pipe(temp))
        {
          using namespace scope;
          childreadh.fd = temp[0];
          readh.fd = temp[1];
          if(-1 != ::pipe(temp))
          {
            writeh.fd = temp[0];
            childwriteh.fd = temp[1];
            auto unmypipes = make_scope_exit([&]() noexcept {
              (void) ::close(readh.fd);
              (void) ::close(writeh.fd);
            });
            auto unhispipes = make_scope_exit([&]() noexcept {
              (void) ::close(childreadh.fd);
              (void) ::close(childwriteh.fd);
            });
            (void) ::fcntl(readh.fd, F_SETFD, FD_CLOEXEC);
            (void) ::fcntl(writeh.fd, F_SETFD, FD_CLOEXEC);

            posix_spawn_file_actions_t child_fd_actions;
            if(!::posix_spawn_file_actions_init(&child_fd_actions))
            {
              auto unactions = make_scope_exit([&]() noexcept { ::posix_spawn_file_actions_destroy(&child_fd_actions); });
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
                      if(!::posix_spawnp(&pid, "llvm-symbolizer", &child_fd_actions, nullptr, (char **) argptrs.data(), environ))
                      {
                        (void) ::close(childreadh.fd);
                        (void) ::close(childwriteh.fd);
                        std::string addrs;
                        addrs.reserve(1024);
                        for(size_t n = 0; n < len; n++)
                        {
                          Dl_info info;
                          memset(&info, 0, sizeof(info));
                          ::dladdr(backtrace[n], &info);
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
                              diff = (char *) backtrace[n] - (char *) info.dli_fbase;
                            }
                            else
                            {
                              diff = (ssize_t) backtrace[n];
                            }
                            char buffer[32];
                            snprintf(buffer, 32, "%zx", diff);
                            buffer[31] = 0;
                            addrs.append(buffer);
                          }
                          else
                          {
                            char buffer[32];
                            snprintf(buffer, 32, "%zx", (uintptr_t) backtrace[n]);
                            buffer[31] = 0;
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
                        auto unsigmask = make_scope_exit([&toblock, &oldset]() noexcept {
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
                          auto printitem = [&](size_t idx) {
                            if(n)
                              ret.append(", ");
                            auto idx2 = addrs.find(10, idx), idx3 = addrs.find(10, idx2 + 1);
                            ret.append(addrs.data() + idx2 + 1, idx3 - idx2 - 1);
                            n++;
                          };
                          size_t oldidx = 0;
                          for(size_t idx = addrs.find("\n\n"); idx != std::string::npos; oldidx = idx + 2, idx = addrs.find("\n\n", idx + 1))
                          {
                            printitem(oldidx);
                          }
                          done = true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        if(!done)
#endif

        {
#if !QUICKCPPLIB_DISABLE_EXECINFO
          char **symbols = backtrace_symbols(backtrace, len);
          if(!symbols)
            ret.append("BACKTRACE FAILED!");
          else
          {
            for(size_t n = 0; n < len; n++)
            {
              if(symbols[n])
              {
                if(n)
                  ret.append(", ");
                ret.append(symbols[n]);
              }
            }
            free(symbols);
          }
#endif
        }
      }
      else
      {
        char temp[256];
        memcpy(temp, v.function, sizeof(v.function));
        temp[sizeof(v.function)] = 0;
        ret.append(temp);
      }
      return ret;
    }
    //! std::ostream writer for simple_ringbuffer_log_policy's value_type
    inline std::ostream &operator<<(std::ostream &s, const value_type &v)
    {
      s << "+" << std::setfill('0') << std::setw(16) << v.timestamp << " " << std::setfill(' ') << std::setw(1);
      switch(v.level)
      {
      case 0:
        s << "none:  ";
        break;
      case 1:
        s << "fatal: ";
        break;
      case 2:
        s << "error: ";
        break;
      case 3:
        s << "warn:  ";
        break;
      case 4:
        s << "info:  ";
        break;
      case 5:
        s << "debug: ";
        break;
      case 6:
        s << "all:   ";
        break;
      default:
        s << "unknown: ";
        break;
      }
      if(v.using_code64)
        s << "{ " << v.code64 << " } ";
      else
        s << "{ " << v.code32[0] << ", " << v.code32[1] << " } ";
      char temp[256];
      memcpy(temp, v.message, sizeof(v.message));
      temp[sizeof(v.message)] = 0;
      s << temp << " @ ";
      s << location(v);
      return s << "\n";
    }
    //! CSV std::ostream writer for simple_ringbuffer_log_policy's value_type
    inline std::ostream &csv(std::ostream &s, const value_type &v)
    {
      // timestamp,level,using_code64,using_backtrace,code0,code1,message,backtrace
      s << v.timestamp << "," << (unsigned) v.level << "," << (unsigned) v.using_code64 << "," << (unsigned) v.using_backtrace << ",";
      if(v.using_code64)
        s << v.code64 << ",0,\"";
      else
        s << v.code32[0] << "," << v.code32[1] << ",\"";
      char temp[256];
      memcpy(temp, v.message, sizeof(v.message));
      temp[sizeof(v.message)] = 0;
      s << temp << "\",\"";
      if(v.using_backtrace)
      {
#if !QUICKCPPLIB_DISABLE_EXECINFO
        char **symbols = backtrace_symbols((void **) v.backtrace, sizeof(v.backtrace) / sizeof(v.backtrace[0]));
        if(!symbols)
          s << "BACKTRACE FAILED!";
        else
        {
          for(size_t n = 0; n < sizeof(v.backtrace) / sizeof(v.backtrace[0]); n++)
          {
            if(symbols[n])
            {
              if(n)
                s << ";";
              s << symbols[n];
            }
          }
          free(symbols);
        }
#endif
      }
      else
      {
        memcpy(temp, v.function, sizeof(v.function));
        temp[sizeof(v.function)] = 0;
        s << temp;
      }
      return s << "\"\n";
    }
  }  // namespace simple_ringbuffer_log_policy_detail

  /*! \tparam Bytes The size of the ring buffer
  \brief A ring buffer log stored in a fixed
  QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_NDEBUG/QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES_DEBUG
  std::array recording
  monotonic counter (8 bytes), high resolution clock time stamp (8 bytes),
  stack backtrace or __func__ (40 bytes), level (1 byte), 191 bytes of
  char message. Each record is 256 bytes, therefore the ring buffer
  wraps after 256/4096 entries by default.
  */
  template <size_t Bytes = QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES * 256> struct simple_ringbuffer_log_policy
  {
    //! Item logged in this log
    using value_type = simple_ringbuffer_log_policy_detail::value_type;
    //! Maximum items of this value_type in this log
    static constexpr size_t max_items = Bytes / sizeof(value_type);
    //! Container for storing log
    using container_type = std::array<value_type, max_items>;
  };

  /*! \class ringbuffer_log
  \brief Very fast threadsafe ring buffer log

  Works on the basis of an always incrementing atomic<size_t> which writes
  into the ring buffer at modulus of the ring buffer size. Items stored per
  log entry are defined by the Policy class' value_type. To log an item,
  call the QUICKCPPLIB_RINGBUFFERLOG_ITEM_* family of macros.

  Be aware iteration, indexing etc. is most recent first, so log[0] is
  the most recently logged item. Use the reversed iterators if you don't
  want this.

  Logging is wait-free and never locks: a writer allocates its record id with
  one atomic fetch_add on the counter, marks its slot with the per slot seqlock
  word, writes the record and marks the slot clean again. Nothing waits on
  anything else, so a writer cannot be delayed by a slow or descheduled peer.

  A record is *published* in the sense that matters to a reader when its slot
  holds it, not when its id was allocated: two writers can be mid-flight at once,
  and one may finish before the other, so an id below the counter is not a
  promise that the record is there yet. Readers therefore validate what they
  copy - the seqlock word must be even at both ends of the copy, and the copy
  must carry the id which was asked for - and give up after a bounded number of
  attempts, returning a default constructed record. This is what makes the
  tear-free accessors safe without a lock: a slot holding a previous wrap's
  record, or an id whose writer has not finished, fails the id check rather than
  being returned as data.

  One precondition follows from the same reasoning, and it is the only one: no
  more than `slots()` writers may be inside a push at the same time, because two
  writers whose ids differ by exactly the ring size share a slot and would
  interleave their seqlock updates. Threads which log into their own log (the
  usual arrangement) satisfy it trivially; a shared log needs fewer concurrent
  writers than it has entries, which for the default 256 entry debug ring is
  almost always true. `clear()` additionally assumes no writer is inside a push:
  it rewrites every slot, and a writer interleaving with that is a different
  question from the one seqlocks answer.

  For simple_ringbuffer_log_policy, typical item logging times are:
  - without backtrace: 1.2 microseconds.
  - with backtrace (windows): up to 33 microseconds.

  \todo Implement STL allocator for a memory mapped file on disc so log
  survives sudden process exit.
  */
  template <class Policy, class LogLevelChecker> class ringbuffer_log
  {
    friend Policy;

  public:
    /*! The container used to store the logged records set by
    Policy::container_type. Must be a ContiguousContainer.
    */
    using container_type = typename Policy::container_type;
    //! The maximum items to store according to Policy::max_items. If zero, use container's size().
    static constexpr size_t max_items = Policy::max_items;

    //! The log record type
    using value_type = typename container_type::value_type;
    //! The size type
    using size_type = typename container_type::size_type;
    //! The difference type
    using difference_type = typename container_type::difference_type;
    //! The reference type
    using reference = typename container_type::reference;
    //! The const reference type
    using const_reference = typename container_type::const_reference;
    //! The pointer type
    using pointer = typename container_type::pointer;
    //! The const pointer type
    using const_pointer = typename container_type::const_pointer;

  protected:
    template <class Parent, class Pointer, class Reference> class iterator_;
    template <class Parent, class Pointer, class Reference> class iterator_
    {
      friend class ringbuffer_log;
      template <class Parent_, class Pointer_, class Reference_> friend class iterator_;
      Parent *_parent;
      size_type _counter, _togo;

      constexpr iterator_(Parent *parent, size_type counter, size_type items)
          : _parent(parent)
          , _counter(counter)
          , _togo(items)
      {
      }

    public:
      using iterator_category = std::random_access_iterator_tag;
      using value_type = typename container_type::value_type;
      using difference_type = typename container_type::difference_type;
      using pointer = typename container_type::pointer;
      using reference = typename container_type::reference;

      constexpr iterator_()
          : _parent(nullptr)
          , _counter(0)
          , _togo(0)
      {
      }
      constexpr iterator_(const iterator_ &) noexcept = default;
      constexpr iterator_(iterator_ &&) noexcept = default;
      iterator_ &operator=(const iterator_ &) noexcept = default;
      iterator_ &operator=(iterator_ &&) noexcept = default;
      // Non-const to const iterator
      template <class Parent_, class Pointer_, class Reference_,
                typename = typename std::enable_if<!std::is_const<Pointer_>::value && !std::is_const<Reference_>::value>::type>
      constexpr iterator_(const iterator_<Parent_, Pointer_, Reference_> &o) noexcept
          : _parent(o._parent)
          , _counter(o._counter)
          , _togo(o._togo)
      {
      }
      iterator_ &operator++() noexcept
      {
        if(_parent && _togo)
        {
          --_counter;
          --_togo;
        }
        return *this;
      }
      void swap(iterator_ &o) noexcept
      {
        std::swap(_parent, o._parent);
        std::swap(_counter, o._counter);
        std::swap(_togo, o._togo);
      }
      Pointer operator->() const noexcept
      {
        if(!_parent || !_togo)
          return nullptr;
        return &_parent->_store[_parent->counter_to_idx(_counter)];
      }
      bool operator==(const iterator_ &o) const noexcept { return _parent == o._parent && _counter == o._counter && _togo == o._togo; }
      bool operator!=(const iterator_ &o) const noexcept { return _parent != o._parent || _counter != o._counter || _togo != o._togo; }
      Reference operator*() const noexcept
      {
        if(!_parent || !_togo)
        {
          static value_type v;
          return v;
        }
        return _parent->_store[_parent->counter_to_idx(_counter)];
      }
      iterator_ operator++(int) noexcept
      {
        iterator_ ret(*this);
        if(_parent && _togo)
        {
          --_counter;
          --_togo;
        }
        return ret;
      }
      iterator_ &operator--() noexcept
      {
        if(_parent && _togo < _parent->size())
        {
          ++_counter;
          ++_togo;
        }
        return *this;
      }
      iterator_ operator--(int) noexcept
      {
        iterator_ ret(*this);
        if(_parent && _togo < _parent->size())
        {
          ++_counter;
          ++_togo;
        }
        return ret;
      }
      bool operator<(const iterator_ &o) const noexcept
      {
        return _parent == o._parent && _parent->counter_to_idx(_counter) < o._parent->counter_to_idx(o._counter);
      }
      bool operator>(const iterator_ &o) const noexcept
      {
        return _parent == o._parent && _parent->counter_to_idx(_counter) > o._parent->counter_to_idx(o._counter);
      }
      bool operator<=(const iterator_ &o) const noexcept
      {
        return _parent == o._parent && _parent->counter_to_idx(_counter) <= o._parent->counter_to_idx(o._counter);
      }
      bool operator>=(const iterator_ &o) const noexcept
      {
        return _parent == o._parent && _parent->counter_to_idx(_counter) >= o._parent->counter_to_idx(o._counter);
      }
      iterator_ &operator+=(size_type v) const noexcept
      {
        if(_parent && _togo)
        {
          if(v > _togo)
            v = _togo;
          _counter -= v;
          _togo -= v;
        }
        return *this;
      }
      iterator_ operator+(size_type v) const noexcept
      {
        iterator_ ret(*this);
        if(_parent && _togo)
        {
          if(v > _togo)
            v = _togo;
          ret._counter -= v;
          ret._togo -= v;
        }
        return ret;
      }
      iterator_ &operator-=(size_type v) const noexcept
      {
        if(_parent && _togo < _parent->size())
        {
          if(v > _parent->size() - _togo)
            v = _parent->size() - _togo;
          _counter += v;
          _togo += v;
        }
        return *this;
      }
      iterator_ operator-(size_type v) const noexcept
      {
        iterator_ ret(*this);
        if(_parent && _togo < _parent->size())
        {
          if(v > _parent->size() - _togo)
            v = _parent->size() - _togo;
          ret._counter += v;
          ret._togo += v;
        }
        return ret;
      }
      difference_type operator-(const iterator_ &o) const noexcept { return (difference_type)(o._counter - _counter); }
      Reference operator[](size_type v) const noexcept { return _parent->_store[_parent->counter_to_idx(_counter + v)]; }
    };
    template <class Parent, class Pointer, class Reference> friend class iterator_;

  public:
    //! The iterator type
    using iterator = iterator_<ringbuffer_log, pointer, reference>;
    //! The const iterator type
    using const_iterator = iterator_<const ringbuffer_log, const_pointer, const_reference>;
    //! The reverse iterator type
    using reverse_iterator = std::reverse_iterator<iterator>;
    //! The const reverse iterator type
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  protected:
    container_type _store;
    std::atomic<level> _instance_level;
    std::atomic<size_type> _counter;
    std::ostream *_immediate;
    /*! One sequence word per slot: the standard seqlock protocol, and the reason
    the snapshot accessors can copy a record while a writer runs. The writer
    bumps the word to ODD before touching the slot and to the next EVEN value
    after, so a reader that sees an odd word, or a word that changed across its
    copy, knows the copy is not a record and discards it. Without this the copy
    was an ordinary read racing an ordinary write: only the counter was atomic,
    so "detect and discard" was value-based reasoning about a data race that the
    memory model does not sanction, however many stress runs failed to see it.

    This is also why nothing serialises writers. Two writers allocate different
    ids from the counter's fetch_add, so ids cannot be lost or duplicated; what
    they can do is finish in a different order from their ids, so the counter is
    not a completed-record watermark. A reader detects that by comparing the id
    the copied record carries against the one it asked for (see
    _try_snapshot_counter), which is a read of the same record the seqlock has
    already fenced - so the honest ordering question is answered per record
    rather than by making writers take turns. See the class documentation for the
    one precondition which follows: at most slots() writers inside a push.
    */
    std::vector<std::atomic<uint32_t>> _seq;
    /*! The counter value at which the log was last cleared. The counter keeps
    counting across a clear - deliberately: it is the identity of a record, and
    resetting it made every id issued before the clear valid again, naming a
    DIFFERENT live record. Records pushed before `_cleared_at` are simply not live
    any more, so an old id resolves to nothing instead of to someone else's data.
    */
    std::atomic<size_type> _cleared_at;
    //! How many times the snapshot accessors retry a slot before giving up and returning a default record
    static constexpr size_t SNAPSHOT_ATTEMPTS = 8;

    size_type counter_to_idx(size_type counter) const noexcept { return max_items ? (counter % max_items) : (counter % _store.size()); }
    size_type slots() const noexcept { return max_items ? max_items : _store.size(); }
    //! Number of records the ring holds, given the newest counter value: the ring window, less anything a clear has retired.
    size_type _live_size(size_type counter) const noexcept
    {
      const size_type cleared = _cleared_at.load(std::memory_order_acquire);
      if(counter <= cleared)
      {
        return 0;
      }
      const size_type live = counter - cleared;
      return live < slots() ? live : slots();
    }

    /*! Marks a slot as being written, returning nothing: the odd/even pair is the
    whole protocol, and both halves are fences rather than plain stores because
    the record write between them must not escape the interval. */
    void _seq_write_begin(size_type idx) noexcept
    {
      const uint32_t s = _seq[idx].load(std::memory_order_relaxed);
      _seq[idx].store(s + 1, std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    void _seq_write_end(size_type idx) noexcept
    {
      std::atomic_thread_fence(std::memory_order_seq_cst);
      const uint32_t s = _seq[idx].load(std::memory_order_relaxed);
      _seq[idx].store(s + 1, std::memory_order_relaxed);
    }

    /*! Copies the record whose unique counter is `counter_value` into `out`, returning false if
    the writer is rewriting that slot, has rewritten it while the copy was made, or has moved far
    enough for the record to have been recycled.
    */
    bool _try_snapshot_counter(size_type counter_value, value_type &out) const noexcept
    {
      const size_type before = _counter.load(std::memory_order_acquire);
      // What `before` can tell us here is only whether `c` is a plausible id at
      // all: it is the high water mark of id *allocation*, so a wrapped slot may
      // hold a newer record - or one still being written - while `c` is long
      // recycled. Deciding "is this record `c`" from the counter alone cannot be
      // done, which is why the copy is validated by the id it carries instead of
      // by how far behind the counter it is; this test only keeps the common
      // "that id never existed / it is ancient" cases from touching memory.
      // Note the `<= slots()` below is deliberately permissive: tightening it
      // would reject the oldest record the ring still holds, which is what an
      // earlier version of this test did.
      if(counter_value >= before || (before - counter_value) > slots() ||
         counter_value < _cleared_at.load(std::memory_order_acquire))
      {
        return false;
      }
      const size_type idx = counter_to_idx(counter_value);
      const uint32_t seq_before = _seq[idx].load(std::memory_order_acquire);
      if((seq_before & 1u) != 0)
      {
        // A writer is inside this slot right now; nothing here is a record.
        return false;
      }
      out = _store[idx];
      std::atomic_thread_fence(std::memory_order_acquire);
      if(_seq[idx].load(std::memory_order_relaxed) != seq_before)
      {
        return false;
      }
      /* The seqlock proves the copy is one whole write of this slot; it does not
      prove the write was the record which was asked for. Writers publish in their
      own order, so this slot can hold the previous wrap's record (id `c -
      slots()`) or an older one while `c`'s writer is still inside it, and before
      the id was checked the copy was returned as if it were `c`. The record
      carries its own id, so ask it: a mismatched copy is not a torn copy, it is
      simply not the record which was requested. */
      if(out.counter != counter_value)
      {
        return false;
      }
      // The id check above is the authority; the counter re-read is kept because
      // it is free and rejects a slot which was recycled while the copy was made.
      return (_counter.load(std::memory_order_acquire) - counter_value) <= slots();
    }

  public:
    //! Default construction, passes through args to container_type
    template <class... Args>
    explicit ringbuffer_log(level starting_level, Args &&... args) noexcept(noexcept(container_type(std::forward<Args>(args)...)))
        : _store(std::forward<Args>(args)...)
        , _instance_level(starting_level)
        , _counter(0)
        , _immediate(nullptr)
        , _cleared_at(0)
        , _seq(slots())
    {
    }
    //! No copying
    ringbuffer_log(const ringbuffer_log &) = delete;
    //! No moving
    ringbuffer_log(ringbuffer_log &&) = delete;
    //! No copying
    ringbuffer_log &operator=(const ringbuffer_log &) = delete;
    //! No moving
    ringbuffer_log &operator=(ringbuffer_log &&) = delete;
    //! Swaps with another instance
    void swap(ringbuffer_log &o) noexcept
    {
      std::swap(_store, o._store);
      auto t = o._instance_level.load(std::memory_order_relaxed);
      o._instance_level.store(_instance_level.load(std::memory_order_relaxed), std::memory_order_relaxed);
      _instance_level.store(t, std::memory_order_relaxed);
      std::swap(_counter, o._counter);
      std::swap(_cleared_at, o._cleared_at);
      std::swap(_immediate, o._immediate);
      // A vector of atomics cannot itself be swapped, and the sequence words
      // belong with the records they describe, so exchange them element-wise.
      const size_type n = _seq.size() < o._seq.size() ? _seq.size() : o._seq.size();
      for(size_type i = 0; i < n; i++)
      {
        const uint32_t a = _seq[i].load(std::memory_order_relaxed);
        const uint32_t b = o._seq[i].load(std::memory_order_relaxed);
        _seq[i].store(b, std::memory_order_relaxed);
        o._seq[i].store(a, std::memory_order_relaxed);
      }
    }

    //! THREADSAFE Returns the log level from the instance filtered by any `LogLevelChecker`.
    level log_level() const noexcept { return LogLevelChecker()(instance_log_level()); }
    //! THREADSAFE Returns the current per-instance log level
    level instance_log_level() const noexcept { return _instance_level.load(std::memory_order_relaxed); }
    //! THREADSAFE Sets the current per-instance log level
    void instance_log_level(level new_level) noexcept { _instance_level.store(new_level, std::memory_order_relaxed); }

    //! Returns true if the log is empty
    bool empty() const noexcept { return size() == 0; }
    //! Returns the number of items in the log
    size_type size() const noexcept { return _live_size(_counter.load(std::memory_order_acquire)); }
    //! Returns the maximum number of items in the log
    size_type max_size() const noexcept { return max_items ? max_items : _store.size(); }
    //! Returns any `std::ostream` immediately printed to when a new log entry is added
    std::ostream *immediate() const noexcept { return _immediate; }
    //! Set any `std::ostream` immediately printed to when a new log entry is added
    void immediate(std::ostream *s) noexcept { _immediate = s; }

    //! Used to tag an index as being an absolute lookup of a unique counter value returned by push_back/emplace_back.
    struct unique_id
    {
      size_type value;
      constexpr unique_id(size_type _value)
          : value(_value)
      {
      }
    };
    //! True if a unique id is still valid
    bool valid(unique_id id) const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type cleared = _cleared_at.load(std::memory_order_acquire);
      return id.value < counter && id.value >= cleared && (counter - id.value) <= slots();
    }

    /*! \brief Returns a tear-free copy of the record `pos` items from the newest, or a default
    constructed record if the ring does not hold that many.
    `pos` counts from the newest exactly as `operator[](size_type)` does. The accessors which hand
    back a reference into the ring (`front()`, `back()`, `operator[]`, `at()` and the iterators)
    are racy by construction: a writer which wraps the ring can rewrite the very slot a reader is
    looking at, and no ordering prevents that. This copies the record out and discards the copy if
    the writer got to the slot first, so what it returns is always one whole record.
    */
    value_type snapshot(size_type pos) const noexcept
    {
      value_type ret;
      // The ring holds at most `held` records, so a request from beyond that can
      // never be satisfied. Asking _try_snapshot_counter for it used to loop
      // forever: the counter-based liveness test rejects exactly pos >= held, so
      // snapshot(size()) - and snapshot(size()-1) once the ring had wrapped -
      // spun with no escape, in a const noexcept accessor documented as
      // returning a default constructed record in that case. The attempt bound
      // covers the other end: a writer that keeps overwriting the requested slot
      // must make the reader give up, not livelock.
      for(size_t attempt = 0; attempt < SNAPSHOT_ATTEMPTS; attempt++)
      {
        const size_type counted = _counter.load(std::memory_order_acquire);
        if(pos >= _live_size(counted))
        {
          return value_type();
        }
        if(_try_snapshot_counter(counted - 1 - pos, ret))
        {
          return ret;
        }
        std::this_thread::yield();
      }
      return value_type();
    }
    /*! \brief Copies the record `pos` items from the newest into `out`, returning false if the
    ring does not hold that many records or if the writer overwrote the slot while it was being
    copied (in which case `out` is left unspecified). Unlike `snapshot()`, this never retries.
    */
    bool try_snapshot(size_type pos, value_type &out) const noexcept
    {
      const size_type counted = _counter.load(std::memory_order_acquire);
      return pos < _live_size(counted) && _try_snapshot_counter(counted - 1 - pos, out);
    }
    /*! \brief Returns a tear-free copy of the record with the given unique id, or a default
    constructed record if the ring no longer holds it.
    */
    value_type snapshot_by_id(unique_id id) const noexcept
    {
      value_type ret;
      for(size_t attempt = 0; attempt < SNAPSHOT_ATTEMPTS; attempt++)
      {
        const size_type counted = _counter.load(std::memory_order_acquire);
        if(id.value >= counted || (counted - id.value) > slots() ||
           id.value < _cleared_at.load(std::memory_order_acquire))
        {
          return value_type();
        }
        if(_try_snapshot_counter(id.value, ret))
        {
          return ret;
        }
        std::this_thread::yield();
      }
      return value_type();
    }
    //! Returns the front of the ringbuffer. Be careful of races with concurrent modifies.
    reference front() noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1)]; }
    //! Returns the front of the ringbuffer. Be careful of races with concurrent modifies.
    const_reference front() const noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1)]; }
#ifdef __cpp_exceptions
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    reference at(size_type pos)
    {
      if(pos >= size())
        throw std::out_of_range("index exceeds size");
      return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1 - pos)];
    }
    //! Returns a reference to the specified element.
    reference at(unique_id id)
    {
      if(!valid(id))
        throw std::out_of_range("index exceeds size");
      return _store[counter_to_idx(id.value)];
    }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    const_reference at(size_type pos) const
    {
      if(pos >= size())
        throw std::out_of_range("index exceeds size");
      return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1 - pos)];
    }
    //! Returns a reference to the specified element.
    const_reference at(unique_id id) const
    {
      if(!valid(id))
        throw std::out_of_range("index exceeds size");
      return _store[counter_to_idx(id.value)];
    }
#endif
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    reference operator[](size_type pos) noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1 - pos)]; }
    //! Returns a reference to the specified element.
    reference operator[](unique_id id) noexcept { return _store[counter_to_idx(id.value)]; }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    const_reference operator[](size_type pos) const noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_acquire) - 1 - pos)]; }
    //! Returns a reference to the specified element.
    const_reference operator[](unique_id id) const noexcept { return _store[counter_to_idx(id.value)]; }
    //! Returns the back of the ringbuffer. Be careful of races with concurrent modifies.
    reference back() noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return _store[counter_to_idx(counter - size)];
    }
    //! Returns the back of the ringbuffer. Be careful of races with concurrent modifies.
    const_reference back() const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return _store[counter_to_idx(counter - size)];
    }

    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    iterator begin() noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    const_iterator begin() const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return const_iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    const_iterator cbegin() const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return const_iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    iterator end() noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return iterator(this, counter - 1 - size, 0);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    const_iterator end() const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return const_iterator(this, counter - 1 - size, 0);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    const_iterator cend() const noexcept
    {
      const size_type counter = _counter.load(std::memory_order_acquire);
      const size_type size = _live_size(counter);
      return const_iterator(this, counter - 1 - size, 0);
    }

    //! Clears the log
    /*! Rewrites every slot and retires every id issued so far.

    Takes no lock and cannot take one without reintroducing the serialisation the
    class documentation rules out, so it assumes no writer is inside a push: a
    writer interleaving with the loop below shares a slot with it and the two
    would interleave their seqlock updates. Clearing a log concurrently with
    logging into it is a teardown/reset operation, not a logging operation; the
    per-slot marking is still worth doing, because it stops a reader accepting a
    slot mid-clear as a record, which is a different race and one this can close.
    */
    void clear() noexcept
    {
      // Clearing rewrites every slot, so it takes the same dirty/clean marking a
      // writer does: without it a reader could accept a slot mid-clear as a
      // record, exactly as it could mid-write.
      for(size_type i = 0; i < _store.size(); i++)
      {
        const size_type idx = i % slots();
        _seq_write_begin(idx);
        _store[i] = value_type();
        _seq_write_end(idx);
      }
      // Retire everything pushed so far rather than resetting the counter: a
      // reset made ids issued before the clear name whatever record now occupies
      // that slot, i.e. silently resolve to someone else's data.
      _cleared_at.store(_counter.load(std::memory_order_acquire), std::memory_order_release);
    }
    //! THREADSAFE Logs a new item, returning its unique counter id
    size_type push_back(value_type &&v) noexcept
    {
      if(static_cast<level>(v.level) <= log_level())
      {
        if(_immediate)
          *_immediate << v << std::endl;
        // One atomic read-modify-write allocates the record's id, so concurrent
        // writers never take the same one and nothing has to serialise them; the
        // counter is *not* a completed-record watermark, because this writer may
        // finish after a writer with a larger id. A reader sorts that out per
        // record, with the seqlock plus the id the record carries - see the class
        // documentation and _try_snapshot_counter.
        size_type thisitem = _counter.fetch_add(1, std::memory_order_acq_rel);
        v.counter = thisitem;
        const size_type idx = counter_to_idx(thisitem);
        // Publish the slot as dirty before it is touched and clean again after it
        // is whole, so a reader can tell a copy of a record from a copy of a
        // record being written. See _seq.
        _seq_write_begin(idx);
        _store[idx] = std::move(v);
        _seq_write_end(idx);
        return thisitem;
      }
      return (size_type) -1;
    }
    //! THREADSAFE Logs a new item, returning its unique counter id
    template <class... Args> size_type emplace_back(level __level, Args &&... args) noexcept
    {
      if(__level <= log_level())
      {
        value_type v(__level, std::forward<Args>(args)...);
        if(_immediate)
          *_immediate << v << std::endl;
        // As push_back: the id comes from one fetch_add, the slot is marked with
        // the seqlock, and no writer waits for another.
        size_type thisitem = _counter.fetch_add(1, std::memory_order_acq_rel);
        v.counter = thisitem;
        const size_type idx = counter_to_idx(thisitem);
        _seq_write_begin(idx);
        _store[idx] = std::move(v);
        _seq_write_end(idx);
        return thisitem;
      }
      return (size_type) -1;
    }
  };

  //! std::ostream writer for a log
  template <class Policy, class LogLevelChecker> inline std::ostream &operator<<(std::ostream &s, const ringbuffer_log<Policy, LogLevelChecker> &l)
  {
    for(const auto &i : l)
    {
      s << i;
    }
    return s;
  }

  //! CSV string writer for a log
  template <class Policy, class LogLevelChecker> inline std::string csv(const ringbuffer_log<Policy, LogLevelChecker> &l)
  {
    std::stringstream s;
    // timestamp,level,using_code64,using_backtrace,code0,code1,message,backtrace
    s << "timestamp,level,using_code64,using_backtrace,code0,code1,message,backtrace\n";
    for(const auto &i : l)
    {
      csv(s, i);
    }
    return s.str();
  }

  //! Alias for a simple ringbuffer log
  template <size_t Bytes = QUICKCPPLIB_RINGBUFFER_LOG_DEFAULT_ENTRIES * 256, class LogLevelChecker = default_ringbuffer_log_level_checker>
  using simple_ringbuffer_log = ringbuffer_log<simple_ringbuffer_log_policy<Bytes>, LogLevelChecker>;
}  // namespace ringbuffer_log

QUICKCPPLIB_NAMESPACE_END


//! Logs an item to the log with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION(log, level, message, code1, code2) (log).emplace_back((level), (message), (code1), (code2), __func__, __LINE__)
//! Logs an item to the log with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE(log, level, message, code1, code2) (log).emplace_back((level), (message), (code1), (code2), nullptr)

#ifndef QUICKCPPLIB_RINGBUFFERLOG_LEVEL
#if defined(_DEBUG) || defined(DEBUG)
#define QUICKCPPLIB_RINGBUFFERLOG_LEVEL 5  // debug
#else
#define QUICKCPPLIB_RINGBUFFERLOG_LEVEL 2  // error
#endif
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 1
//! Logs an item to the log at fatal level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_FATAL_FUNCTION(log, message, code1, code2)                                                                                   \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::fatal, (message), (code1), (code2))
//! Logs an item to the log at fatal level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_FATAL_BACKTRACE(log, message, code1, code2)                                                                                  \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::fatal, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_FATAL_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_FATAL_BACKTRACE(log, message, code1, code2)
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 2
//! Logs an item to the log at error level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_ERROR_FUNCTION(log, message, code1, code2)                                                                                   \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::error, (message), (code1), (code2))
//! Logs an item to the log at error level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_ERROR_BACKTRACE(log, message, code1, code2)                                                                                  \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::error, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_ERROR_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_ERROR_BACKTRACE(log, message, code1, code2)
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 3
//! Logs an item to the log at warn level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_WARN_FUNCTION(log, message, code1, code2)                                                                                    \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::warn, (message), (code1), (code2))
//! Logs an item to the log at warn level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_WARN_BACKTRACE(log, message, code1, code2)                                                                                   \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::warn, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_WARN_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_WARN_BACKTRACE(log, message, code1, code2)
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 4
//! Logs an item to the log at info level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_INFO_FUNCTION(log, message, code1, code2)                                                                                    \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::info, (message), (code1), (code2))
//! Logs an item to the log at info level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_INFO_BACKTRACE(log, message, code1, code2)                                                                                   \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::info, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_INFO_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_INFO_BACKTRACE(log, message, code1, code2)
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 5
//! Logs an item to the log at debug level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_DEBUG_FUNCTION(log, message, code1, code2)                                                                                   \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::debug, (message), (code1), (code2))
//! Logs an item to the log at debug level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_DEBUG_BACKTRACE(log, message, code1, code2)                                                                                  \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::debug, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_DEBUG_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_DEBUG_BACKTRACE(log, message, code1, code2)
#endif

#if QUICKCPPLIB_RINGBUFFERLOG_LEVEL >= 6
//! Logs an item to the log at all level with calling function name
#define QUICKCPPLIB_RINGBUFFERLOG_ALL_FUNCTION(log, message, code1, code2)                                                                                     \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::all, (message), (code1), (code2))
//! Logs an item to the log at all level with stack backtrace
#define QUICKCPPLIB_RINGBUFFERLOG_ALL_BACKTRACE(log, message, code1, code2)                                                                                    \
  QUICKCPPLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::all, (message), (code1), (code2))
#else
#define QUICKCPPLIB_RINGBUFFERLOG_ALL_FUNCTION(log, message, code1, code2)
#define QUICKCPPLIB_RINGBUFFERLOG_ALL_BACKTRACE(log, message, code1, code2)
#endif

#endif
