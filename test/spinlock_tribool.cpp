/* Unit testing for spinlocks
(C) 2013-2017 Niall Douglas <http://www.nedproductions.biz/> (7 commits)


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

#define NANOSECONDS_PER_CPU_CYCLE (1000000000000ULL / 3700000000ULL)

#define _CRT_SECURE_NO_WARNINGS 1

#include "../include/boost/test/unit_test.hpp"
#include "../include/spinlock.hpp"
#include "../include/tribool.hpp"
#include "timing.h"

#include <algorithm>
#include <cstdio>
#include <mutex>  // for lock_guard

#ifdef _MSC_VER
#pragma warning(disable : 4127)  // conditional expression is constant
#endif

BOOST_AUTO_TEST_SUITE(all)

BOOST_AUTO_TEST_CASE(works / spinlock / binary, "Tests that the spinlock works as intended")
{
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<bool> lock;
  BOOST_REQUIRE(lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock();

  std::lock_guard<decltype(lock)> h(lock);
  BOOST_REQUIRE(!lock.try_lock());
}

#if 0
BOOST_AUTO_TEST_CASE(works / spinlock / transacted, "Tests that the spinlock works as intended under transactions")
{
  boost_lite::configurable_spinlock::spinlock<bool> lock;
  boost_lite::configurable_spinlock::atomic<size_t> gate(0);
  size_t locked = 0;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
#pragma omp parallel for
  for(int i = 0; i < (int) (1000 * threads); i++)
  {
    BOOST_BEGIN_TRANSACT_LOCK(lock) { ++locked; }
    BOOST_END_TRANSACT_LOCK(lock)
  }
  BOOST_REQUIRE(locked == 1000 * threads);
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(works / spinlock / ordered, "Tests that the ordered spinlock works as intended")
{
  boost_lite::configurable_spinlock::ordered_spinlock<> lock;
  BOOST_REQUIRE(lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock();

  std::lock_guard<decltype(lock)> h(lock);
  BOOST_REQUIRE(!lock.try_lock());
}
#endif

BOOST_AUTO_TEST_CASE(works / spinlock / shared, "Tests that the shared spinlock works as intended")
{
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<> lock;
  BOOST_REQUIRE(lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock_shared());
  lock.unlock();

  BOOST_REQUIRE(lock.try_lock_shared());
  BOOST_REQUIRE(!lock.try_lock());
  BOOST_REQUIRE(lock.try_lock_shared());
  BOOST_REQUIRE(!lock.try_lock());
  BOOST_REQUIRE(lock.try_lock_shared());
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock_shared();
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock_shared();
  BOOST_REQUIRE(!lock.try_lock());
  lock.unlock_shared();
  BOOST_REQUIRE(lock.try_lock());
  BOOST_REQUIRE(!lock.try_lock_shared());
  lock.unlock();
}

template <class locktype> void TestLockCorrectness(const char *desc)
{
  locktype lock;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> writers(0);
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> exclusives(0);
  std::vector<std::thread> threads;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> done(std::thread::hardware_concurrency() + 1);
  for(size_t n = 0; n < std::thread::hardware_concurrency(); n++)
  {
    threads.push_back(std::thread([&] {
      --done;
      while(done)
      {
        std::this_thread::yield();
      }
      while(!done)
      {
        lock.lock();
        size_t _writers = ++writers;
        if(_writers > 1)
        {
          BOOST_REQUIRE(_writers <= 1);
        }
        ++exclusives;
        --writers;
        lock.unlock();
      }
      ++done;
    }));
  }
  printf("Running %s ...\n", desc);
  while(done != 1)
  {
    std::this_thread::yield();
  }
  auto start = std::chrono::high_resolution_clock::now();
  --done;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  printf("   Stopping test ...\n");
  ++done;
  while(done != std::thread::hardware_concurrency() + 1)
  {
    std::this_thread::yield();
  }
  auto end = std::chrono::high_resolution_clock::now();
  for(auto &i : threads)
  {
    i.join();
  }
  printf("   Achieved %u exclusive ops/sec\n\n", (unsigned) (exclusives / std::chrono::duration_cast<std::chrono::seconds>(end - start).count()));  // NOLINT
}

template <class locktype, size_t testreaders> void TestSharedLockCorrectness(const char *desc)
{
  locktype lock;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> writers(0), readers(0), maxreaders(0);
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> exclusives(0), shared(0);
  std::vector<std::thread> threads;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> done(std::thread::hardware_concurrency() + 1);
  for(size_t n = 0; n < std::thread::hardware_concurrency(); n++)
  {
    threads.push_back(std::thread([&, n] {
      --done;
      while(done)
      {
        std::this_thread::yield();
      }
      if(n < testreaders)
      {
        while(!done)
        {
          lock.lock_shared();
          size_t _ = ++readers;
          if(_ > maxreaders)
          {
            maxreaders = _;
          }
          size_t _writers = writers;
          if(_writers > 0)
          {
            BOOST_REQUIRE(_writers == 0);
          }
          ++shared;
          --readers;
          lock.unlock_shared();
        }
      }
      else
      {
        while(!done)
        {
          lock.lock();
          size_t _writers = ++writers;
          if(_writers > 1)
          {
            BOOST_REQUIRE(_writers == 1);
          }
          size_t _readers = readers;
          if(_readers > 0)
          {
            BOOST_REQUIRE(_readers == 0);
          }
          ++exclusives;
          --writers;
          lock.unlock();
        }
      }
      ++done;
    }));
  }
  printf("Running %s ...\n", desc);
  while(done != 1)
  {
    std::this_thread::yield();
  }
  auto start = std::chrono::high_resolution_clock::now();
  --done;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  printf("   Stopping test ...\n");
  ++done;
  while(done != std::thread::hardware_concurrency() + 1)
  {
    std::this_thread::yield();
  }
  auto end = std::chrono::high_resolution_clock::now();
  for(auto &i : threads)
  {
    i.join();
  }
  printf("   Achieved %u exclusive ops/sec and %u shared ops/sec, maxreaders=%u\n\n", (unsigned) (exclusives / std::chrono::duration_cast<std::chrono::seconds>(end - start).count()), (unsigned) (shared / std::chrono::duration_cast<std::chrono::seconds>(end - start).count()), (unsigned) maxreaders);  // NOLINT
}

BOOST_AUTO_TEST_CASE(works / spinlock / threaded / binary, "Tests that the binary spinlock works as intended under threads")
{
  TestLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<uintptr_t>>("binary spinlock correctness testing");
}
#if 0
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / ordered, "Tests that the ordered spinlock works as intended under threads")
{
  TestLockCorrectness<boost_lite::configurable_spinlock::ordered_spinlock<>>("ordered spinlock correctness testing");
}
#endif
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / shared / exclusive, "Tests that the shared spinlock works as intended under threads")
{
  TestLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>>("exclusive shared spinlock correctness testing");
}
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / shared / shared, "Tests that the shared spinlock works as intended under threads")
{
  TestSharedLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>, 999>("shared shared spinlock correctness testing");
}
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / shared / 1reader, "Tests that the shared spinlock works as intended under threads")
{
  TestSharedLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>, 1>("single reader shared spinlock correctness testing");
}
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / shared / 2reader, "Tests that the shared spinlock works as intended under threads")
{
  TestSharedLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>, 2>("dual reader shared spinlock correctness testing");
}
BOOST_AUTO_TEST_CASE(works / spinlock / threaded / shared / 3reader, "Tests that the shared spinlock works as intended under threads")
{
  TestSharedLockCorrectness<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>, 3>("triple reader shared spinlock correctness testing");
}


template <bool tristate, class T> struct do_lock
{
  void operator()(T &lock) { lock.lock(); }
};
template <class T> struct do_lock<true, T>
{
  void operator()(T &lock)
  {
    int e = 0;
    lock.lock(e);
  }
};
template <class T> struct do_lock_shared
{
  explicit do_lock_shared(T & /*unused*/) {}
};
template <class T> struct do_lock_shared<QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<T>>  // NOLINT
{
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<T> &_lock;
  explicit do_lock_shared(QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<T> &lock)
      : _lock(lock)
  {
    _lock.lock_shared();
  }
  ~do_lock_shared() { _lock.unlock_shared(); }
};

template <class locktype> double CalculatePerformance(int use_transact)
{
  locktype lock;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> gate(0);
  struct
  {
    size_t value;
    char padding[64 - sizeof(size_t)];
  } count[64];
  memset(&count, 0, sizeof(count));
  usCount start, end;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
  // printf("There are %u threads in this CPU\n", (unsigned) threads);
  start = GetUsCount();
#ifdef _MSC_VER
#pragma omp parallel for
  for(long thread = 0; thread < (long) threads; thread++)
#else
#pragma omp parallel for
  for(size_t thread = 0; thread < threads; thread++)
#endif
  {
    --gate;
    while(gate)
    {
    }
    for(size_t n = 0; n < 10000000; n++)
    {
      if(use_transact == 1)
      {
        QUICKCPPLIB_BEGIN_TRANSACT_LOCK(lock) { ++count[thread].value; }
        QUICKCPPLIB_END_TRANSACT_LOCK(lock)
      }
      else if(use_transact == 2)
      {
        do_lock_shared<locktype> h(lock);
        ++count[thread].value;
      }
      else
      {
        do_lock<std::is_same<typename locktype::value_type, int>::value, locktype>()(lock);
        ++count[thread].value;
        lock.unlock();
      }
    }
  }
  end = GetUsCount();
  size_t increments = 0;
  for(size_t thread = 0; thread < threads; thread++)
  {
    BOOST_REQUIRE(count[thread].value == 10000000);
    increments += count[thread].value;
  }
  return increments / ((end - start) / 1000000000000.0);
}

BOOST_AUTO_TEST_CASE(performance / spinlock / binary, "Tests the performance of binary spinlocks")
{
  printf("\n=== Binary spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<uintptr_t>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
}

#if 0
BOOST_AUTO_TEST_CASE(performance / spinlock / binary / transaction, "Tests the performance of binary spinlock transactions")
{
  printf("\n=== Transacted binary spinlock performance ===\n");
  using locktype = boost_lite::configurable_spinlock::spinlock<uintptr_t>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
}
#endif

BOOST_AUTO_TEST_CASE(performance / spinlock / tristate, "Tests the performance of tristate spinlocks")
{
  printf("\n=== Tristate spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<int>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
}

#if 0
BOOST_AUTO_TEST_CASE(performance / spinlock / tristate / transaction, "Tests the performance of tristate spinlock transactions")
{
  printf("\n=== Transacted tristate spinlock performance ===\n");
  using locktype = boost_lite::configurable_spinlock::spinlock<int>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
}
#endif

BOOST_AUTO_TEST_CASE(performance / spinlock / pointer, "Tests the performance of pointer spinlocks")
{
  printf("\n=== Pointer spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<QUICKCPPLIB_NAMESPACE::configurable_spinlock::lockable_ptr<int>>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
}

#if 0
BOOST_AUTO_TEST_CASE(performance / spinlock / pointer / transaction, "Tests the performance of pointer spinlock transactions")
{
  printf("\n=== Transacted pointer spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<boost_lite::configurable_spinlock::lockable_ptr<int>>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(1));
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(performance / spinlock / ordered, "Tests the performance of ordered spinlocks")
{
  printf("\n=== Ordered spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::ordered_spinlock<>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
}
#endif

BOOST_AUTO_TEST_CASE(performance / spinlock / shared / exclusive, "Tests the performance of shared exclusive spinlocks")
{
  printf("\n=== Shared exclusive spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(0));
}

BOOST_AUTO_TEST_CASE(performance / spinlock / shared / shared, "Tests the performance of shared exclusive spinlocks")
{
  printf("\n=== Shared shared spinlock performance ===\n");
  using locktype = QUICKCPPLIB_NAMESPACE::configurable_spinlock::shared_spinlock<>;
  printf("1. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(2));
  printf("2. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(2));
  printf("3. Achieved %lf transactions per second\n", CalculatePerformance<locktype>(2));
}


#if 0
static double CalculateMallocPerformance(size_t size, bool use_transact)
{
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::spinlock<bool> lock;
  QUICKCPPLIB_NAMESPACE::configurable_spinlock::atomic<size_t> gate(0);
  usCount start, end;
#pragma omp parallel
  {
    ++gate;
  }
  size_t threads = gate;
  // printf("There are %u threads in this CPU\n", (unsigned) threads);
  start = GetUsCount();
#pragma omp parallel for
  for(int n = 0; n < 10000000 * threads; n++)
  {
    void *p;
    if(use_transact)
    {
      BOOST_BEGIN_TRANSACT_LOCK(lock) { p = malloc(size); }
      BOOST_END_TRANSACT_LOCK(lock)
    }
    else
    {
      std::lock_guard<decltype(lock)> g(lock);
      p = malloc(size);
    }
    if(use_transact)
    {
      BOOST_BEGIN_TRANSACT_LOCK(lock) { free(p); }
      BOOST_END_TRANSACT_LOCK(lock)
    }
    else
    {
      std::lock_guard<decltype(lock)> g(lock);
      free(p);
    }
  }
  end = GetUsCount();
  BOOST_REQUIRE(true);
  //  printf("size=%u\n", (unsigned) map.size());
  return threads * 10000000 / ((end - start) / 1000000000000.0);
}

BOOST_AUTO_TEST_CASE(performance / malloc / transact / small, "Tests the transact performance of multiple threads using small memory allocations")
{
  printf("\n=== Small malloc transact performance ===\n");
  printf("1. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
  printf("2. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
  printf("3. Achieved %lf transactions per second\n", CalculateMallocPerformance(16, 1));
}

BOOST_AUTO_TEST_CASE(performance / malloc / transact / large, "Tests the transact performance of multiple threads using large memory allocations")
{
  printf("\n=== Large malloc transact performance ===\n");
  printf("1. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
  printf("2. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
  printf("3. Achieved %lf transactions per second\n", CalculateMallocPerformance(65536, 1));
}
#endif


BOOST_AUTO_TEST_CASE(works / tribool, "Tests that the tribool works as intended")
{
  using QUICKCPPLIB_NAMESPACE::tribool::tribool;
  auto t(tribool::true_), f(tribool::false_), o(tribool::other), u(tribool::unknown);
  BOOST_CHECK(true_(t));
  BOOST_CHECK(false_(f));
  BOOST_CHECK(other(o));
  BOOST_CHECK(t != f);
  BOOST_CHECK(t != o);
  BOOST_CHECK(f != o);
  BOOST_CHECK(o == u);
  BOOST_CHECK(~t == f);
  BOOST_CHECK(~f == t);
  BOOST_CHECK(~u == u);
  BOOST_CHECK((f & f) == f);
  BOOST_CHECK((f & t) == f);
  BOOST_CHECK((f & u) == f);
  BOOST_CHECK((u & u) == u);
  BOOST_CHECK((u & t) == u);
  BOOST_CHECK((t & t) == t);
  BOOST_CHECK((f | f) == f);
  BOOST_CHECK((f | u) == u);
  BOOST_CHECK((f | t) == t);
  BOOST_CHECK((u | u) == u);
  BOOST_CHECK((u | t) == t);
  BOOST_CHECK((t | t) == t);
  BOOST_CHECK(std::min(f, u) == f);
  BOOST_CHECK(std::min(f, t) == f);
  BOOST_CHECK(std::min(u, t) == u);
  BOOST_CHECK(std::min(t, t) == t);
  BOOST_CHECK(std::max(f, u) == u);
  BOOST_CHECK(std::max(f, t) == t);
  std::cout << "bool false is " << false << ", bool true is " << true << std::endl;
  std::cout << "tribool false is " << f << ", tribool unknown is " << u << ", tribool true is " << t << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
