/* Function pointer support
(C) 2019 Niall Douglas <http://www.nedproductions.biz/> (4 commits)
File Created: June 2019


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

#ifndef QUICKCPPLIB_FUNCTION_PTR_HPP
#define QUICKCPPLIB_FUNCTION_PTR_HPP

#include "config.hpp"

#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN

//! \brief The namespace for the function pointer type
namespace function_ptr
{
  /*! \brief A move only lightweight `std::function` alternative, with small object optimisation.

  Requirements for small object optimisation:

  1. `sizeof(U) + sizeof(void *) <= 64`
  2. `U` must be nothrow move constructible.
  */
  template <class F> class function_ptr;
  template <class R, class... Args> class function_ptr<R(Args...)>
  {
    template <class R_, class U, class... Args2> friend constexpr inline function_ptr<R_> emplace_function_ptr(Args2 &&... args);  // NOLINT

    struct function_ptr_storage
    {
      function_ptr_storage() = default;
      function_ptr_storage(const function_ptr_storage &) = delete;
      function_ptr_storage(function_ptr_storage &&) = delete;
      function_ptr_storage &operator=(const function_ptr_storage &) = delete;
      function_ptr_storage &operator=(function_ptr_storage &&) = delete;
      virtual ~function_ptr_storage() = default;
      virtual R operator()(Args &&... args) = 0;
      virtual function_ptr_storage *move(char *v) noexcept = 0;
    };
    template <class U> struct function_ptr_storage_nonmoveable : public function_ptr_storage
    {
      U c;
      template <class... Args2>
      constexpr explicit function_ptr_storage_nonmoveable(Args2 &&... args)
          : c(static_cast<Args2 &&>(args)...)
      {
      }
      function_ptr_storage_nonmoveable(const function_ptr_storage_nonmoveable &) = delete;
      function_ptr_storage_nonmoveable(function_ptr_storage_nonmoveable &&) = delete;
      function_ptr_storage_nonmoveable &operator=(const function_ptr_storage_nonmoveable &) = delete;
      function_ptr_storage_nonmoveable &operator=(function_ptr_storage_nonmoveable &&) = delete;
      R operator()(Args &&... args) final { return c(static_cast<Args &&>(args)...); }
      function_ptr_storage *move(char * /*unused*/) noexcept final { abort(); }
    };
    template <class U> struct function_ptr_storage_moveable : public function_ptr_storage
    {
      struct standin_t
      {
        template <class... Args2> standin_t(Args2 &&...) {}
        R operator()(Args &&... args) { return {}; }
      };
      using type = std::conditional_t<std::is_move_constructible<U>::value, U, standin_t>;
      type c;
      template <class... Args2>
      constexpr explicit function_ptr_storage_moveable(Args2 &&... args)
          : c(static_cast<Args2 &&>(args)...)
      {
      }
      function_ptr_storage_moveable(const function_ptr_storage_moveable &) = delete;
      function_ptr_storage_moveable(function_ptr_storage_moveable &&o) noexcept  // NOLINT
      : c(static_cast<type &&>(o.c))
      {
      }
      function_ptr_storage_moveable &operator=(const function_ptr_storage_moveable &) = delete;
      function_ptr_storage_moveable &operator=(function_ptr_storage_moveable &&) = delete;
      R operator()(Args &&... args) final { return c(static_cast<Args &&>(args)...); }
      function_ptr_storage *move(char *v) noexcept final { return new(v) function_ptr_storage_moveable(static_cast<function_ptr_storage_moveable &&>(*this)); }
    };

    function_ptr_storage *_ptr{nullptr};
    char _sso[64 - sizeof(function_ptr_storage *)];
    template <class U>
    static constexpr bool is_ssoable = std::is_nothrow_move_constructible<typename std::decay<U>::type>::value  //
                                       && (sizeof(typename std::decay<U>::type) <= sizeof(_sso));

    template <class U> struct emplace_t
    {
    };
    template <class U> struct allocate_t
    {
    };
    template <class U> struct noallocate_t
    {
    };
    template <class U, class... Args2>
    constexpr function_ptr(allocate_t<U> /*unused*/, Args2 &&... args)
        : _ptr(new function_ptr_storage_nonmoveable<typename std::decay<U>::type>(static_cast<Args2 &&>(args)...))
    {
    }
    template <class U, class... Args2>
    constexpr function_ptr(noallocate_t<U> /*unused*/, Args2 &&... args)
        : _ptr(new((void *)(is_ssoable<U> ? _sso : nullptr)) function_ptr_storage_moveable<typename std::decay<U>::type>(static_cast<Args2 &&>(args)...))
    {
    }
    // Delegate to non-moveable, or moveable allocating constructor, or non-allocating constructor as necessary
    template <class U, class... Args2>
    constexpr explicit function_ptr(emplace_t<U> /*unused*/, Args2 &&... args)
        : function_ptr(is_ssoable<U> ? function_ptr(noallocate_t<U>{}, static_cast<Args2 &&>(args)...) : function_ptr(allocate_t<U>{}, static_cast<Args2 &&>(args)...))
    {
    }

    bool _is_ssoed() const noexcept { return (void *) _ptr == (void *) _sso; }

  public:
    using result_type = R;
    constexpr function_ptr() = default;
    constexpr function_ptr(function_ptr &&o) noexcept
    {
      if(o._is_ssoed())
      {
        _ptr = o._ptr->move(_sso);
      }
      else
      {
        _ptr = o._ptr;
      }
      o._ptr = nullptr;
    }
    function_ptr &operator=(function_ptr &&o) noexcept
    {
      this->~function_ptr();
      new(this) function_ptr(static_cast<function_ptr &&>(o));
      return *this;
    }
    function_ptr(const function_ptr &) = delete;
    function_ptr &operator=(const function_ptr &) = delete;
    ~function_ptr()
    {
      if(!_is_ssoed())
      {
        delete _ptr;
      }
    }
    explicit constexpr operator bool() const noexcept { return !!_ptr; }
    template <class... Args2> constexpr R operator()(Args2... args) const { return (*_ptr)(static_cast<Args2 &&>(args)...); }
    constexpr void reset() noexcept
    {
      if(!_is_ssoed())
      {
        delete _ptr;
      }
      _ptr = nullptr;
    }
  };

  /*! \brief Return a `function_ptr<ErasedPrototype>` by emplacing `Callable(CallableConstructionArgs...)`.
  If `Callable` is nothrow move constructible and sufficiently small, avoids
  dynamic memory allocation.
   */
  template <class ErasedPrototype, class Callable, class... CallableConstructionArgs>  //
  constexpr inline function_ptr<ErasedPrototype> emplace_function_ptr(CallableConstructionArgs &&... args)
  {
    return function_ptr<ErasedPrototype>(typename function_ptr<ErasedPrototype>::template emplace_t<Callable>(), static_cast<CallableConstructionArgs &&>(args)...);
  }

  /*! \brief Return a `function_ptr<ErasedPrototype>` by from an input `Callable`.
  If `Callable` is nothrow move constructible and sufficiently small, avoids
  dynamic memory allocation.
   */
  template <class ErasedPrototype, class Callable>  //
  constexpr inline function_ptr<ErasedPrototype> make_function_ptr(Callable &&f)
  {
    return emplace_function_ptr<ErasedPrototype, std::decay_t<Callable>>(static_cast<Callable &&>(f));
  }

}  // namespace function_ptr

QUICKCPPLIB_NAMESPACE_END

#endif
