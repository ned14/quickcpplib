/* Test algorithm::hash
(C) 2021 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


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

#include "../include/quickcpplib/algorithm/hash.hpp"
#include "../include/quickcpplib/algorithm/string.hpp"
#include "../include/quickcpplib/boost/test/unit_test.hpp"
#include "../include/quickcpplib/string_view.hpp"

BOOST_AUTO_TEST_SUITE(algorithm_)

BOOST_AUTO_TEST_CASE(hash / sha256, "Tests that algorithm::hash256() works as advertised")
{
  struct sha256_test
  {
    QUICKCPPLIB_NAMESPACE::string_view::string_view what, sha256;
  };
  static const sha256_test items[] = {
  //
  {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},                                                                   //
  {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},                                                                //
  {"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", "a8ae6e6ee929abea3afcfc5258c8ccd6f85273e0d4626d26c7279f3250f77c8e"},   //
  {"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde", "057ee79ece0b9a849552ab8d3c335fe9a5f1c46ef5f1d9b190c295728628299c"},    //
  {"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0", "2a6ad82f3620d3ebe9d678c812ae12312699d673240d5be8fac0910a70000d93"},  //
  {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},           //
  {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
   "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"}  //
  };
  for(auto &i : items)
  {
    char _buffer[64];
    QUICKCPPLIB_NAMESPACE::string_view::string_view buffer(_buffer, 64);
    {
      auto hash = QUICKCPPLIB_NAMESPACE::algorithm::hash::sha256_hash::hash(i.what.data(), i.what.size());
      QUICKCPPLIB_NAMESPACE::algorithm::string::to_hex_string(QUICKCPPLIB_NAMESPACE::span::span<char>(_buffer),
                                                              QUICKCPPLIB_NAMESPACE::span::span<const uint8_t>(hash.as_bytes));
      std::cout << "\nWas:       " << buffer << "\nShould be: " << i.sha256 << std::endl;
      BOOST_CHECK(buffer == i.sha256);
    }

    QUICKCPPLIB_NAMESPACE::algorithm::hash::sha256_hash impl;
    for(size_t n = 0; n < i.what.size(); n++)
    {
      impl.add(&i.what[n], 1);
    }
    auto hash = impl.finalise();
    QUICKCPPLIB_NAMESPACE::algorithm::string::to_hex_string(QUICKCPPLIB_NAMESPACE::span::span<char>(_buffer),
                                                            QUICKCPPLIB_NAMESPACE::span::span<const uint8_t>(hash.as_bytes));
    std::cout << "\nWas:       " << buffer << "\nShould be: " << i.sha256 << std::endl;
    BOOST_CHECK(buffer == i.sha256);
  }
}

BOOST_AUTO_TEST_SUITE_END()
