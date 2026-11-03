/* Test hamming_ecc (the class under test used to be wrongly called secded_ecc and is a plain Hamming code, not a
SECDED code)
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/quickcpplib/algorithm/hamming_ecc.hpp"
#include "../include/quickcpplib/algorithm/small_prng.hpp"
#include "../include/quickcpplib/boost/test/unit_test.hpp"

#include "timing.h"

BOOST_AUTO_TEST_SUITE(hamming)

BOOST_AUTO_TEST_CASE(hamming / works, "Tests that algorithm::hamming_ecc works as advertised")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm;
  static constexpr size_t bytes = 4096;
  small_prng::small_prng rand;

  std::vector<char> buffer(bytes);
  for(size_t n = 0; n < bytes / sizeof(small_prng::small_prng::value_type); n++)
  {
    reinterpret_cast<small_prng::small_prng::value_type *>(buffer.data())[n] = rand();
  }
  hamming_ecc::hamming_ecc<bytes> engine;
  using ecc_type = hamming_ecc::hamming_ecc<bytes>::result_type;
  ecc_type eccbits = engine.result_bits_valid();
  std::cout << "\n\nECC will be " << eccbits << " bits long" << std::endl;
  ecc_type ecc = engine(buffer.data());
  std::cout << "ECC was calculated to be " << std::hex << ecc << std::dec << std::endl;

  std::cout << "Flipping every bit in the buffer to see if it is correctly detected ..." << std::endl;
  auto begin = std::chrono::high_resolution_clock::now();
  for(size_t toflip = 0; toflip < bytes * 8; toflip++)
  {
    buffer[toflip / 8] ^= ((size_t) 1 << (toflip % 8));
    ecc_type newecc = engine(buffer.data());
    if(ecc == newecc)
    {
      std::cerr << "ERROR: Flipping bit " << toflip << " not detected!" << std::endl;
      BOOST_CHECK(ecc != newecc);
    }
    else
    {
      ecc_type badbit = engine.find_bad_bit(ecc, newecc);
      if(badbit != toflip)
      {
        std::cerr << "ERROR: Bad bit " << badbit << " is not the bit " << toflip << " we flipped!" << std::endl;
        BOOST_CHECK(badbit == toflip);
      }
      //      else
      //        std::cout << "SUCCESS: Bit flip " << toflip << " correctly detected" << std::endl;
    }
    if(2 != engine.verify(buffer.data(), ecc))
    {
      std::cerr << "ERROR: verify() did not heal the buffer!" << std::endl;
      BOOST_CHECK(false);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - begin);
  std::cout << "Checking and fixing is approximately " << (bytes * 10000 / diff.count() / 1024 / 1024) << " Mb/sec" << std::endl;

  std::cout << "\nFlipping two bits in the buffer to see if it is correctly detected ..." << std::endl;
  buffer[0] ^= 1;
  begin = std::chrono::high_resolution_clock::now();
  for(size_t toflip = 1; toflip < bytes * 8; toflip++)
  {
    buffer[toflip / 8] ^= ((size_t) 1 << (toflip % 8));
    ecc_type newecc = engine(buffer.data());
    if(ecc == newecc)
    {
      std::cerr << "ERROR: Flipping bits 0 and " << toflip << " not detected!" << std::endl;
      BOOST_CHECK(ecc != newecc);
    }
    buffer[toflip / 8] ^= ((size_t) 1 << (toflip % 8));
  }
  end = std::chrono::high_resolution_clock::now();
  diff = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - begin);
  std::cout << "Calculating is approximately " << (bytes * 10000 / diff.count() / 1024 / 1024) << " Mb/sec" << std::endl;

  std::cout << "\nCalculating speeds ..." << std::endl;
  size_t foo = 0;
  begin = std::chrono::high_resolution_clock::now();
  auto _begin = ticksclock();
  for(size_t n = 0; n < 10000; n++)
  {
    buffer[0] = (char) n;
    foo += engine(buffer.data());
  }
  auto _end = ticksclock();
  end = std::chrono::high_resolution_clock::now();
  diff = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - begin);
  if(foo)
    std::cout << "Fixed buffer size calculating is approximately " << (bytes * 10000 / diff.count() / 1024 / 1024) << " Mb/sec, or " << ((_end - _begin) / 10000.0 / 4096) << " cycles/byte" << std::endl;
  foo = 0;
  begin = std::chrono::high_resolution_clock::now();
  _begin = ticksclock();
  for(size_t n = 0; n < 10000; n++)
  {
    buffer[0] = (char) n;
    foo += engine(buffer.data(), bytes);
  }
  _end = ticksclock();
  end = std::chrono::high_resolution_clock::now();
  diff = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - begin);
  if(foo)
    std::cout << "Variable buffer size calculating is approximately " << (bytes * 10000 / diff.count() / 1024 / 1024) << " Mb/sec, or " << ((_end - _begin) / 10000.0 / 4096) << " cycles/byte" << std::endl;
  foo = 0;
  begin = std::chrono::high_resolution_clock::now();
  for(size_t n = 0; n < 1000; n++)
  {
    buffer[0] = (char) n;
    foo += engine.verify(buffer.data(), ecc);
  }
  end = std::chrono::high_resolution_clock::now();
  diff = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - begin);
  if(foo)
    std::cout << "Checking and fixing is approximately " << (bytes * 1000 / diff.count() / 1024 / 1024) << " Mb/sec" << std::endl;
}

// A blocksize which is not a multiple of eight must not read past the caller's
// buffer, and must compute the same ECC as the byte-wise overload for the same
// bytes. The fixed-size overload used to read 8 bytes per lump regardless, so
// blocksize % 8 != 0 over-read the buffer and ecc_table, and (after that was
// fixed) indexed the table from the wrong offset for the tail (reviewer P0
// r3:F1).
BOOST_AUTO_TEST_CASE(hamming / partial lumps, "Tests that hamming_ecc handles a blocksize which is not a multiple of eight")
{
  using namespace QUICKCPPLIB_NAMESPACE::algorithm;
  auto check = [](size_t bytes_) {
    std::vector<char> buffer(bytes_);
    for(size_t n = 0; n < bytes_; n++)
      buffer[n] = (char) (n * 7 + 3);
    hamming_ecc::hamming_ecc<9> nine;
    hamming_ecc::hamming_ecc<15> fifteen;
    if(bytes_ == 9)
    {
      BOOST_CHECK(nine(buffer.data()) == nine(0, buffer.data(), bytes_));
    }
    else
    {
      BOOST_CHECK(fifteen(buffer.data()) == fifteen(0, buffer.data(), bytes_));
    }
  };
  check(9);
  check(15);
}

BOOST_AUTO_TEST_SUITE_END()
