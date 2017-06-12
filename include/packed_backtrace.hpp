/* Space packed stack backtrace
(C) 2017 Niall Douglas <http://www.nedproductions.biz/> (21 commits)
File Created: Jun 2017


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

#ifndef QUICKCPPLIB_PACKED_BACKTRACE_HPP
#define QUICKCPPLIB_PACKED_BACKTRACE_HPP

#include "config.hpp"
#include "span.hpp"

#include <cstddef>  // for ptrdiff_t etc
#include <cstdint>  // for uint32_t etc

QUICKCPPLIB_NAMESPACE_BEGIN

namespace packed_backtrace
{
  namespace detail
  {
    template <class FramePtrType, size_t FrameTypeSize> class packed_backtrace
    {
      using storage_type = span::span<FramePtrType>;
      storage_type _storage;

    protected:
      explicit packed_backtrace(span::span<char> storage)
          : _storage(reinterpret_cast<FramePtrType *>(storage.data()), storage.size() / sizeof(FramePtrType))
      {
      }

    public:
      //! The type stored in the container
      using value_type = typename storage_type::element_type;
      //! The size type
      using size_type = size_t;
      //! The difference type
      using difference_type = typename storage_type::difference_type;
      //! The reference type
      using reference = typename storage_type::reference;
      //! The const reference type
      using const_reference = typename storage_type::const_reference;
      //! The pointer type
      using pointer = typename storage_type::pointer;
      //! The const pointer type
      using const_pointer = typename storage_type::const_pointer;
      //! The iterator type
      using iterator = typename storage_type::iterator;
      //! The const iterator type
      using const_iterator = typename storage_type::const_iterator;
      //! The reverse iterator type
      using reverse_iterator = typename storage_type::reverse_iterator;
      //! The const reverse iterator type
      using const_reverse_iterator = typename storage_type::const_reverse_iterator;

      //! Returns true if the index is empty
      bool empty() const noexcept { return _storage.empty(); }
      //! Returns the number of items in the backtrace
      size_type size() const noexcept { return _storage.size(); }
      //! Returns the maximum number of items in the backtrace
      size_type max_size() const noexcept { return _storage.size(); }
      //! Returns an iterator to the first item in the backtrace.
      iterator begin() noexcept { return _storage.begin(); }
      //! Returns an iterator to the first item in the backtrace.
      const_iterator begin() const noexcept { return _storage.begin(); }
      //! Returns an iterator to the first item in the backtrace.
      const_iterator cbegin() const noexcept { return begin(); }
      //! Returns an iterator to the item after the last in the backtrace.
      iterator end() noexcept { return _storage.end(); }
      //! Returns an iterator to the item after the last in the backtrace.
      const_iterator end() const noexcept { return _storage.end(); }
      //! Returns an iterator to the item after the last in the backtrace.
      const_iterator cend() const noexcept { return end(); }
      //! Returns the specified element, unchecked.
      value_type operator[](size_type idx) const noexcept { return _storage[idx]; }
      //! Returns the specified element, checked.
      value_type at(size_type idx) const noexcept { return _storage.at(idx); }
      //! Swaps with another instance
      void swap(packed_backtrace &o) noexcept { _storage.swap(o._storage); }

      //! Assigns a raw stack backtrace to the packed storage
      void assign(span::span<value_type> input) noexcept { memcpy(_storage.data(), input.data(), _storage.size_bytes()); }
    };
    /*
    template <template <class> class Container, class FramePtrType> class packed_backtrace<Container, FramePtrType, 8>
    {
    protected:
      Container<uint8_t> _storage;
    };
    */
  }
  /*! \class packed_backtrace
  \brief A space packed stack backtrace letting you store twice or more
  stack frames in the same space.
  \tparam ContiguousContainerType Some STL container meeting the `ContiguousContainer` concept. `std::array<>` and `std::vector` are
  two of the most commonly used.
  \tparam FramePtrType The type each stack backtrace frame ought to be represented as.

  64 bit address stack backtraces tend to waste a lot of storage which can be a problem
  when storing lengthy backtraces. Most 64 bit architectures only use the first 43 bits
  of address space wasting 2.5 bytes per entry, plus stack backtraces tend to be within
  a few megabytes and even kilobytes from one another. Intelligently packing the bits
  based on this knowledge can double or more the number of items you can store for a
  given number of bytes at virtually no runtime overhead, unlike compression.

  On 32 bit architectures this class simply stores the stack normally, but otherwise
  works the same.

  The 64-bit coding scheme is quite straightforward:

  * Top bits are 11 when it's bits 63-41 of a 64 bit absolute address (3 bytes)
  * Top bits are 10 when it's bits 43-21 of a 64 bit absolute address (3 bytes)
  * Top bits are 01 when it's a 22 bit offset from previous (3 bytes) (+- 0x40`0000, 4Mb)
  * Top bits are 00 when it's a 14 bit offset from previous (2 bytes) (+- 0x4000, 16Kb)
  * Potential improvement: 12 to 18 items in 40 bytes instead of 5 items

  Sample 1:
  0000`07fe`fd4e`10ac - 6 bytes
  0000`07fe`f48b`ffc7 - 3 bytes
  0000`07fe`f48b`ff70 - 2 bytes
  0000`07fe`f48b`fe23 - 2 bytes
  0000`07fe`f48d`51d8 - 3 bytes
  0000`07fe`f499`5249 - 3 bytes
  0000`07fe`f48a`ef28 - 3 bytes
  0000`07fe`f48a`ecc9 - 2 bytes
  0000`07fe`f071`244c - 6 bytes
  0000`07fe`f071`11b5 - 2 bytes
  0000`07ff`0015`0acf - 6 bytes
  0000`07ff`0015`098c - 2 bytes (40 bytes, 12 items, usually 96 bytes, 58% reduction)

  Sample 2:
  0000003d06e34950 - 6 bytes
  0000000000400bcd - 6 bytes
  0000000000400bf5 - 2 bytes
  0000003d06e1ffe0 - 6 bytes
  00000000004009f9 - 6 bytes (26 bytes, 5 items, usually 40 bytes, 35% reduction)
  */
  template <class FramePtrType> class packed_backtrace : public detail::packed_backtrace<FramePtrType, sizeof(FramePtrType)>
  {
    using base = detail::packed_backtrace<FramePtrType, sizeof(FramePtrType)>;

  public:
    //! \brief Default constructor
    explicit packed_backtrace(span::span<char> storage)
        : base(storage)
    {
    }
    //! \brief Default copy constructor
    packed_backtrace(const packed_backtrace &) = default;
    //! \brief Default move constructor
    packed_backtrace(packed_backtrace &&) = default;
    //! \brief Default copy assignment
    packed_backtrace &operator=(const packed_backtrace &) = default;
    //! \brief Default move assignment
    packed_backtrace &operator=(packed_backtrace &&) = default;
  };

  //! \brief
  inline packed_backtrace<const void *> make_packed_backtrace(span::span<char> output, span::span<const void *> input)
  {
    packed_backtrace<const void *> ret(output);
    ret.assign(input);
    return ret;
  }
}

QUICKCPPLIB_NAMESPACE_END

#endif
