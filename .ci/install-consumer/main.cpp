// A downstream consumer of an INSTALLED quickcpplib: it includes a public header
// by its installed path and uses something from it, so the lane fails if the
// install layout, the exported include directories or the exported compile
// features are wrong. See .ci/install-consumer/CMakeLists.txt.
#include <quickcpplib/algorithm/prime_modulus.hpp>

#include <cstdio>

int main()
{
  // constexpr, and a value that is pinned in the header's table: 251 is the prime
  // just below 2^8. A changed answer here means the installed header is not the
  // one that was built, which is exactly the kind of packaging mistake that no
  // add_subdirectory() consumer can see.
  constexpr std::uint64_t prime = QUICKCPPLIB_NAMESPACE::algorithm::prime_modulus::twos_power_prime(8);
  static_assert(prime == 251, "twos_power_prime(8) should be 251");
  std::printf("installed quickcpplib works: twos_power_prime(8) == %llu\n", (unsigned long long) prime);
  return prime == 251 ? 0 : 1;
}
