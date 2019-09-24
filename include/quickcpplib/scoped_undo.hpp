/* Call a lambda if a scope unwinds
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (2 commits)
File Created: Aug 2016


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

#ifndef QUICKCPPLIB_SCOPED_UNDO_HPP
#define QUICKCPPLIB_SCOPED_UNDO_HPP

#include "config.hpp"

#include <type_traits>
#include <utility>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace scoped_undo
{
  namespace detail
  {
    template <typename callable> class undo_impl;
  }
  template <typename callable> inline detail::undo_impl<callable> undoer(callable c);
  namespace detail
  {
    namespace _detail
    {
      template <typename T, bool iscomparable> struct is_nullptr
      {
        bool operator()(T c) const noexcept { return !c; }
      };
      template <typename T> struct is_nullptr<T, false>
      {
        bool operator()(T) const noexcept { return false; }
      };
    }
    template <typename T> bool is_nullptr(T v) noexcept { return _detail::is_nullptr<T, std::is_trivially_constructible<bool, T>::value>()(std::forward<T>(v)); }

    template <typename callable> class undo_impl
    {
      template <typename _callable> friend undo_impl<_callable> scoped_undo::undoer(_callable c);
      bool _dismissed;
      callable _undoer;
      undo_impl() = delete;
      undo_impl(const undo_impl &) = delete;
      undo_impl &operator=(const undo_impl &) = delete;
      explicit undo_impl(callable &&c) noexcept : _dismissed(false), _undoer(std::move(c)) {}
      void _trigger()
      {
        if(!_dismissed && !is_nullptr(_undoer))
        {
          _undoer();
          _dismissed = true;
        }
      }

    public:
      undo_impl(undo_impl &&o) noexcept : _dismissed(o._dismissed), _undoer(std::move(o._undoer)) { o._dismissed = true; }
      undo_impl &operator=(undo_impl &&o) noexcept
      {
        _trigger();
        _dismissed = o._dismissed;
        _undoer = std::move(o._undoer);
        o._dismissed = true;
        return *this;
      }
      ~undo_impl() { _trigger(); }
      //! Returns if the Undoer is dismissed
      bool dismissed() const { return _dismissed; }
      //! Dismisses the Undoer
      void dismiss(bool d = true) { _dismissed = d; }
      //! Undismisses the Undoer
      void undismiss(bool d = true) { _dismissed = !d; }
    };
  }

  /*! \brief Alexandrescu style rollbacks, a la C++ 11.

  Example of usage:
  \code
  auto resetpos=undoer([&s]() { s.seekg(0, std::ios::beg); });
  ...
  resetpos.dismiss();
  \endcode

  \note Interesting factoid: This code is one of the oldest pieces of C++ Niall is
  still using. Been in continuous usage, in one form or another, since 2002!
  */
  template <typename callable> inline detail::undo_impl<callable> undoer(callable c)
  {
    // static_assert(!std::is_function<callable>::value && !std::is_member_function_pointer<callable>::value && !std::is_member_object_pointer<callable>::value && !has_call_operator<callable>::value, "Undoer applied to a type not providing a call operator");
    return detail::undo_impl<callable>(std::move(c));
  }
}

QUICKCPPLIB_NAMESPACE_END

#endif
