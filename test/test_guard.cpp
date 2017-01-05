
// Define std::boost::asio ABI
#define BOOST_AFIO_USE_BOOST_THREAD 0
#define BOOST_AFIO_USE_BOOST_FILESYSTEM 1
#define ASIO_STANDALONE 1
#include "test_guard.hpp"

static int (*abi1)()=BOOST_AFIO_V1_NAMESPACE::foo;

// Should be ignored as second inclusion with same ABI
#include "test_guard.hpp"

// Define new boost::std::std ABI
#undef BOOST_AFIO_USE_BOOST_THREAD
#undef BOOST_AFIO_USE_BOOST_FILESYSTEM
#undef ASIO_STANDALONE
#define BOOST_AFIO_USE_BOOST_THREAD 1
#define BOOST_AFIO_USE_BOOST_FILESYSTEM 0
#define ASIO_STANDALONE 0
#include "test_guard.hpp"

static int (*abi2)()=BOOST_AFIO_V1_NAMESPACE::foo;

// Should be ignored as second inclusion with same ABI
#include "test_guard.hpp"

// Put it back to std::boost::asio ABI and ensure it is still ignored
#undef BOOST_AFIO_USE_BOOST_THREAD
#undef BOOST_AFIO_USE_BOOST_FILESYSTEM
#undef ASIO_STANDALONE
#define BOOST_AFIO_USE_BOOST_THREAD 0
#define BOOST_AFIO_USE_BOOST_FILESYSTEM 1
#define ASIO_STANDALONE 1
#include "test_guard.hpp"

static int (*abi3)()=BOOST_AFIO_V1_NAMESPACE::foo;

//extern "C" void printf(const char *, ...);
int main(void)
{
#if 0
  printf("std::boost::asio::foo=%p\n", abi1);
#ifdef __cpp_inline_namespaces
  printf("boost::std::std::foo=%p\n", abi2);
#endif
  printf("std::boost::asio::foo=%p\n", abi3);
  //printf("result=%d\n", abi1!=abi3 || abi1==abi2 || abi2==abi3);
#endif
  return abi1!=abi3 || abi1==abi2 || abi2==abi3;
}
