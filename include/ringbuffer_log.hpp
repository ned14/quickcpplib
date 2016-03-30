/* ringbuffer_log.hpp
Very fast threadsafe ring buffer log
(C) 2016 Niall Douglas http://www.nedprod.com/
File Created: Mar 2016


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef BOOST_BINDLIB_IMPORT_HPP
#define BOOST_BINDLIB_IMPORT_HPP

#include "cpp_feature.h"

#include <array>
#include <atomic>
#include <chrono>
#include <system_error>

#ifdef _WIN32
#include "execinfo_win64.h"
#else
#include <execinfo.h>
#endif

namespace ringbuffer_log
{
  template <class Policy> class ringbuffer_log;
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

  /*! \struct simple_ringbuffer_log_policy
  \brief A ring buffer log stored in a fixed 64Kb std::array recording
  monotonic counter (8 bytes), high resolution clock time stamp (8 bytes),
  stack backtrace or __func__ (40 bytes), level (1 byte), 191 bytes of
  char message. Each record is 256 bytes, therefore the ring buffer
  wraps after 256 entries.
  */
  template <size_t Bytes = 65536> struct simple_ringbuffer_log_policy
  {
    using level_ = level;
    using uint8 = unsigned char;
    using uint32 = unsigned int;
    using uint64 = unsigned long long;
    struct value_type
    {
      uint64 counter;
      uint64 timestamp;
      union {
        uint32 code32[2];
        uint64 code64;
      };
      union {
        uint64 backtrace[5];
        char function[40];
      };
      uint8 level : 4;
      uint8 using_code64 : 1;
      uint8 using_backtrace : 1;
      char message[191];

      static std::chrono::high_resolution_clock::time_point first_item()
      {
        static std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        return now;
      }
      value_type() { memset(this, 0, sizeof(*this)); }
      value_type(level_ _level, const char *_message, uint32 _code1, uint32 _code2, const char *_function = nullptr)
          : counter((size_t) -1)
          , timestamp(std::chrono::duration_cast<std::chrono::nanoseconds>((first_item(), std::chrono::high_resolution_clock::now() - first_item())).count())
          , code32{_code1, _code2}
          , level(static_cast<uint8>(_level))
          , using_code64(false)
          , using_backtrace(!_function)
      {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)  // use of strncpy
#endif
        strncpy(message, _message, sizeof(message));
        if(_function)
          strncpy(function, _function, sizeof(function));
        else
        {
          constexpr size_t items = 1 + sizeof(backtrace) / sizeof(backtrace[0]);
          void *temp[items];
          memset(temp, 0, sizeof(temp));
          size_t len = ::backtrace(temp, items);
          memcpy(backtrace, temp + 1, sizeof(backtrace));
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
      }
      bool operator==(const value_type &o) const noexcept { return !memcmp(this, &o, sizeof(*this)); }
      bool operator!=(const value_type &o) const noexcept { return memcmp(this, &o, sizeof(*this)); }
    };
    static_assert(sizeof(value_type) == 256, "value_type is not 256 bytes long!");
    static constexpr size_t max_items = Bytes / sizeof(value_type);
    using container_type = std::array<value_type, max_items>;
  };

  /*! \class ringbuffer_log
  \brief Very fast threadsafe ring buffer log

  Works on the basis of an always incrementing atomic<size_t> which writes
  into the ring buffer at modulus of the ring buffer size. Items stored per
  log entry are defined by the Policy class' value_type. To log an item,
  call the BINDLIB_RINGBUFFERLOG_ITEM macro. Here might be a typical stanza
  creating a log for some library or part thereof.

  Log item helpers include:
  - Fast stack backtrace.

  TODO:
  - Should be a 4/8/12/16Kb memory mapped file on disc so it survives sudden process
  exit.
  */
  template <class Policy> class ringbuffer_log
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
    template <class Pointer, class Reference> class iterator_;
    template <class Pointer, class Reference> class iterator_ : public std::iterator<std::random_access_iterator_tag, value_type, difference_type, pointer, reference>
    {
      friend class ringbuffer_log;
      template <class Pointer_, class Reference_> friend class iterator_;
      ringbuffer_log *_parent;
      size_type _counter, _togo;

      constexpr iterator_(ringbuffer_log *parent, size_type counter, size_type items)
          : _parent(parent)
          , _counter(counter)
          , _togo(items)
      {
      }

    public:
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
      template <class Pointer_, class Reference_, typename = std::enable_if_t<!std::is_const<Pointer_>::value && !std::is_const<Reference_>::value>> constexpr iterator_(const iterator_<Pointer_, Reference_> &o) noexcept : _parent(o._parent), _counter(o._counter), _togo(o._togo) {}
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
          return (Reference)(*(Pointer *) 0);
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
      bool operator<(const iterator_ &o) const noexcept { return _parent == o._parent && _parent->counter_to_idx(_counter) < o._parent->counter_to_idx(o._counter); }
      bool operator>(const iterator_ &o) const noexcept { return _parent == o._parent && _parent->counter_to_idx(_counter) > o._parent->counter_to_idx(o._counter); }
      bool operator<=(const iterator_ &o) const noexcept { return _parent == o._parent && _parent->counter_to_idx(_counter) <= o._parent->counter_to_idx(o._counter); }
      bool operator>=(const iterator_ &o) const noexcept { return _parent == o._parent && _parent->counter_to_idx(_counter) >= o._parent->counter_to_idx(o._counter); }
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
    template <class Pointer, class Reference> friend class iterator_;

  public:
    //! The iterator type
    using iterator = iterator_<pointer, reference>;
    //! The const iterator type
    using const_iterator = iterator_<const_pointer, const_reference>;
    //! The reverse iterator type
    using reverse_iterator = std::reverse_iterator<iterator>;
    //! The const reverse iterator type
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  protected:
    container_type _store;
    level _level;
    std::atomic<size_type> _counter;

    size_type counter_to_idx(size_type counter) noexcept { return max_items ? (counter % max_items) : (counter % _store.size()); }
  public:
    //! Default construction, passes through args to container_type
    template <class... Args>
    ringbuffer_log(level starting_level, Args &&... args)
        : _store(std::forward<Args>(args)...)
        , _level(starting_level)
        , _counter(0)
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
      std::swap(_level, o._level);
      std::swap(_counter, o._counter);
    }

    //! Returns the current log level
    level log_level() const noexcept { return _level; }
    //! Returns the current log level
    void log_level(level new_level) noexcept { _level = new_level; }

    //! Returns true if the log is empty
    bool empty() const noexcept { return _counter.load(std::memory_order_relaxed) == 0; }
    //! Returns the number of items in the log
    size_type size() const noexcept
    {
      size_type ret = _counter.load(std::memory_order_relaxed);
      if(_store.size() < ret)
        ret = _store.size();
      return ret;
    }
    //! Returns the maximum number of items in the log
    size_type max_size() const noexcept { return max_items ? max_items : _store.size(); }

    //! Returns the front of the ringbuffer. Be careful of races with concurrent modifies.
    reference front() noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1)]; }
    //! Returns the front of the ringbuffer. Be careful of races with concurrent modifies.
    const_reference front() const noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1)]; }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    reference at(size_type pos) noexcept
    {
      if(pos >= size())
        throw std::out_of_range();
      return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1 - pos)];
    }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    const_reference at(size_type pos) const noexcept
    {
      if(pos >= size())
        throw std::out_of_range();
      return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1 - pos)];
    }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    reference operator[](size_type pos) noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1 - pos)]; }
    //! Returns a reference to the specified element. Be careful of races with concurrent modifies.
    const_reference operator[](size_type pos) const noexcept { return _store[counter_to_idx(_counter.load(std::memory_order_relaxed) - 1 - pos)]; }
    //! Returns the back of the ringbuffer. Be careful of races with concurrent modifies.
    reference back() noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return _store[counter_to_idx(counter - size)];
    }
    //! Returns the back of the ringbuffer. Be careful of races with concurrent modifies.
    const_reference back() const noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return _store[counter_to_idx(counter - size)];
    }

    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    iterator begin() noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    const_iterator begin() const noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the first item in the log. Be careful of races with concurrent modifies.
    const_iterator cbegin() const noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1, size);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    iterator end() noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1 - size, 0);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    const_iterator end() const noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1 - size, 0);
    }
    //! Returns an iterator to the item after the last in the log. Be careful of races with concurrent modifies.
    const_iterator cend() const noexcept
    {
      size_type counter = _counter.load(std::memory_order_relaxed);
      size_type size = counter;
      if(_store.size() < size)
        size = _store.size();
      return iterator(this, counter - 1 - size, 0);
    }

    //! Clears the log
    void clear() noexcept
    {
      _counter.store(0, std::memory_order_relaxed);
      std::fill(_store.begin(), _store.end(), value_type());
    }
    //! Logs a new item, returning its unique id
    size_type push_back(value_type &&v) noexcept
    {
      if(static_cast<level>(v.level) <= _level)
      {
        size_type thisitem = _counter++;
        v.counter = thisitem;
        _store[counter_to_idx(thisitem)] = std::move(v);
        return thisitem;
      }
      return (size_type) -1;
    }
    //! Logs a new item, returning its unique id
    template <class... Args> size_type emplace_back(Args &&... args) noexcept
    {
      value_type v(std::forward<Args>(args)...);
      if(static_cast<level>(v.level) <= _level)
      {
        size_type thisitem = _counter++;
        v.counter = thisitem;
        _store[counter_to_idx(thisitem)] = std::move(v);
        return thisitem;
      }
      return (size_type) -1;
    }
  };

  //! Alias for a simple ringbuffer log
  template <size_t Bytes = 65536> using simple_ringbuffer_log = ringbuffer_log<simple_ringbuffer_log_policy<Bytes>>;
}

//! Logs an item to the log with calling function name
#define BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION(log, level, message, code1, code2) (log).emplace_back((level), (message), (code1), (code2), __func__)
//! Logs an item to the log with stack backtrace
#define BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE(log, level, message, code1, code2) (log).emplace_back((level), (message), (code1), (code2), nullptr)

#ifndef BINDLIB_RINGBUFFERLOG_LEVEL
#if defined(_DEBUG) || defined(DEBUG)
#define BINDLIB_RINGBUFFERLOG_LEVEL 5  // debug
#else
#define BINDLIB_RINGBUFFERLOG_LEVEL 2  // error
#endif
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 1
//! Logs an item to the log at fatal level with calling function name
#define BINDLIB_RINGBUFFERLOG_FATAL_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::fatal, (message), (code1), (code2))
//! Logs an item to the log at fatal level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_FATAL_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::fatal, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_FATAL_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_FATAL_BACKTRACE(log, message, code1, code2)
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 2
//! Logs an item to the log at error level with calling function name
#define BINDLIB_RINGBUFFERLOG_ERROR_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::error, (message), (code1), (code2))
//! Logs an item to the log at error level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_ERROR_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::error, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_ERROR_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_ERROR_BACKTRACE(log, message, code1, code2)
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 3
//! Logs an item to the log at warn level with calling function name
#define BINDLIB_RINGBUFFERLOG_WARN_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::warn, (message), (code1), (code2))
//! Logs an item to the log at warn level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_WARN_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::warn, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_WARN_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_WARN_BACKTRACE(log, message, code1, code2)
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 4
//! Logs an item to the log at info level with calling function name
#define BINDLIB_RINGBUFFERLOG_INFO_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::info, (message), (code1), (code2))
//! Logs an item to the log at info level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_INFO_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::info, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_INFO_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_INFO_BACKTRACE(log, message, code1, code2)
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 5
//! Logs an item to the log at debug level with calling function name
#define BINDLIB_RINGBUFFERLOG_DEBUG_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::debug, (message), (code1), (code2))
//! Logs an item to the log at debug level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_DEBUG_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::debug, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_DEBUG_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_DEBUG_BACKTRACE(log, message, code1, code2)
#endif

#if BINDLIB_RINGBUFFERLOG_LEVEL >= 6
//! Logs an item to the log at all level with calling function name
#define BINDLIB_RINGBUFFERLOG_ALL_FUNCTION(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_FUNCTION((log), ringbuffer_log::level::all, (message), (code1), (code2))
//! Logs an item to the log at all level with stack backtrace
#define BINDLIB_RINGBUFFERLOG_ALL_BACKTRACE(log, message, code1, code2) BINDLIB_RINGBUFFERLOG_ITEM_BACKTRACE((log), ringbuffer_log::level::all, (message), (code1), (code2))
#else
#define BINDLIB_RINGBUFFERLOG_ALL_FUNCTION(log, message, code1, code2)
#define BINDLIB_RINGBUFFERLOG_ALL_BACKTRACE(log, message, code1, code2)
#endif

#endif