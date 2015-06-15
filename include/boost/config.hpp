/* boostmacros.hpp
Provides lightweight Boost macros
(C) 2014 Niall Douglas http://www.nedprod.com/
File Created: Aug 2014


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

#ifndef BOOST_BINDLIB_BOOST_MACROS_HPP
#define BOOST_BINDLIB_BOOST_MACROS_HPP

#include "../cpp_feature.h"
 
#ifndef BOOST_SMT_PAUSE
# if defined(_MSC_VER) && _MSC_VER >= 1310 && ( defined(_M_IX86) || defined(_M_X64) )
extern "C" void _mm_pause();
#  pragma intrinsic( _mm_pause )
#  define BOOST_SMT_PAUSE _mm_pause();
# elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#  define BOOST_SMT_PAUSE __asm__ __volatile__( "rep; nop" : : : "memory" );
# endif
#endif

#ifndef BOOST_NOEXCEPT
# ifdef __cpp_noexcept
#  define BOOST_NOEXCEPT noexcept
# endif
#endif
#ifndef BOOST_NOEXCEPT
# define BOOST_NOEXCEPT
#endif

#ifndef BOOST_NOEXCEPT_OR_NOTHROW
# ifdef __cpp_noexcept
#  define BOOST_NOEXCEPT_OR_NOTHROW noexcept
# endif
#endif
#ifndef BOOST_NOEXCEPT_OR_NOTHROW
# define BOOST_NOEXCEPT_OR_NOTHROW throw()
#endif

#ifndef BOOST_NOEXCEPT_IF
# ifdef __cpp_noexcept
#  define BOOST_NOEXCEPT_IF(v) noexcept(v)
# endif
#endif
#ifndef BOOST_NOEXCEPT_IF
# define BOOST_NOEXCEPT_IF(v)
#endif

#ifndef BOOST_NOEXCEPT_EXPR
# ifdef __cpp_noexcept
#  define BOOST_NOEXCEPT_EXPR(v) noexcept(v)
# endif
#endif
#ifndef BOOST_NOEXCEPT_EXPR
# define BOOST_NOEXCEPT_EXPR(v) false
#endif

#ifndef BOOST_CONSTEXPR
# ifdef __cpp_constexpr
#  define BOOST_CONSTEXPR constexpr
# endif
#endif
#ifndef BOOST_CONSTEXPR
# define BOOST_CONSTEXPR
#endif

#ifndef BOOST_CXX14_CONSTEXPR
# ifdef __cpp_relaxed_constexpr
#  define BOOST_CXX14_CONSTEXPR constexpr
# endif
#endif
#ifndef BOOST_CXX14_CONSTEXPR
# define BOOST_CXX14_CONSTEXPR
#endif

#ifndef BOOST_CONSTEXPR_OR_CONST
# ifdef __cpp_constexpr
#  define BOOST_CONSTEXPR_OR_CONST constexpr
# endif
#endif
#ifndef BOOST_CONSTEXPR_OR_CONST
# define BOOST_CONSTEXPR_OR_CONST const
#endif

#ifndef BOOST_STATIC_CONSTEXPR
# ifdef __cpp_constexpr
#  define BOOST_STATIC_CONSTEXPR static constexpr
# endif
#endif
#ifndef BOOST_STATIC_CONSTEXPR
# define BOOST_STATIC_CONSTEXPR static const
#endif

#ifndef BOOST_FORCEINLINE
# if defined(_MSC_VER)
#  define BOOST_FORCEINLINE __forceinline
# elif defined(__GNUC__)
#  define BOOST_FORCEINLINE __attribute__((always_inline))
# else
#  define BOOST_FORCEINLINE
# endif
#endif

#ifndef BOOST_NOINLINE
# if defined(_MSC_VER)
#  define BOOST_NOINLINE __declspec(noinline)
# elif defined(__GNUC__)
#  define BOOST_NOINLINE __attribute__((noinline))
# else
#  define BOOST_NOINLINE
# endif
#endif

#ifndef BOOST_SYMBOL_VISIBLE
# if defined(_MSC_VER)
#  define BOOST_SYMBOL_VISIBLE
# elif defined(__GNUC__)
#  define BOOST_SYMBOL_VISIBLE __attribute__((visibility("default")))
# else
#  define BOOST_SYMBOL_VISIBLE
# endif
#endif

#ifndef BOOST_SYMBOL_EXPORT
# if defined(_MSC_VER)
#  define BOOST_SYMBOL_EXPORT __declspec(dllexport)
# elif defined(__GNUC__)
#  define BOOST_SYMBOL_EXPORT __attribute__((visibility("default")))
# else
#  define BOOST_SYMBOL_EXPORT
# endif
#endif

#ifndef BOOST_SYMBOL_IMPORT
# if defined(_MSC_VER)
#  define BOOST_SYMBOL_IMPORT __declspec(dllimport)
# elif defined(__GNUC__)
#  define BOOST_SYMBOL_IMPORT
# else
#  define BOOST_SYMBOL_IMPORT
# endif
#endif

#ifndef BOOST_FOREACH
#define BOOST_FOREACH(a, b) for(a : b)
#endif

#ifndef BOOST_MSVC
#ifdef _MSC_VER
#define BOOST_MSVC _MSC_VER
#endif
#endif
#ifndef BOOST_WINDOWS
#ifdef WIN32
#define BOOST_WINDOWS 1
#endif
#endif
#ifndef BOOST_GCC
#ifdef __GNUC__
#define BOOST_GCC (__GNUC__ * 10000 \
                   + __GNUC_MINOR__ * 100 \
                   + __GNUC_PATCHLEVEL__)
#endif
#endif

/* The following are for convenience, but really should be regex find & replace in modern C++ */
#ifndef BOOST_FWD_REF
#define BOOST_FWD_REF(a) a&&
#endif

#ifndef BOOST_RV_REF
#define BOOST_RV_REF(a) a&&
#endif

#ifndef BOOST_COPY_ASSIGN_REF
#define BOOST_COPY_ASSIGN_REF(a) const a&
#endif

#ifndef BOOST_STATIC_ASSERT_MSG
#define BOOST_STATIC_ASSERT_MSG(v, m) static_assert((v), m)
#endif

// Map SG10 feature macros to Boost ones
#if !defined(__cpp_exceptions) && !defined(BOOST_NO_EXCEPTIONS)
# define BOOST_NO_EXCEPTIONS 1
#endif

#if !defined(__cpp_rtti) && !defined(BOOST_NO_RTTI)
# define BOOST_NO_RTTI 1
#endif


// C++ 11
//#if !defined(__cpp_access_control_sfinae)
//# define __cpp_access_control_sfinae 190000
//#endif

#if !defined(__cpp_alias_templates) && !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
# define BOOST_NO_CXX11_TEMPLATE_ALIASES 1
#endif

#if !defined(__cpp_alignas) && !defined(BOOST_NO_CXX11_ALIGNAS)
# define BOOST_NO_CXX11_ALIGNAS 1
#endif

//#if !defined(__cpp_attributes)
//# define __cpp_attributes 190000
//#endif

#if !defined(__cpp_constexpr) && !defined(BOOST_NO_CXX11_CONSTEXPR)
# define BOOST_NO_CXX11_CONSTEXPR 1
#endif

#if !defined(__cpp_decltype) && !defined(BOOST_NO_CXX11_DECLTYPE)
# define BOOST_NO_CXX11_DECLTYPE 1
#endif

#if !defined(__cpp_default_function_template_args) && !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
# define BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS 1
#endif

#if !defined(__cpp_defaulted_functions) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
# define BOOST_NO_CXX11_DEFAULTED_FUNCTIONS 1
#endif

//#if !defined(__cpp_delegating_constructors)
//# define __cpp_delegating_constructors 190000
//#endif

#if !defined(__cpp_deleted_functions) && !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
# define BOOST_NO_CXX11_DELETED_FUNCTIONS 1
#endif

#if !defined(__cpp_explicit_conversions) && !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
# define BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS 1
#endif

#if !defined(__cpp_generalized_initializers) && !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
# define BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX 1
#endif

//#if !defined(__cpp_implicit_moves)
//# define __cpp_implicit_moves 190000
//#endif

//#if !defined(__cpp_inheriting_constructors)
//# define __cpp_inheriting_constructors 190000
//#endif

#if !defined(__cpp_inline_namespaces) && !defined(BOOST_NO_CXX11_INLINE_NAMESPACES)
# define BOOST_NO_CXX11_INLINE_NAMESPACES 1
#endif

#if !defined(__cpp_lambdas) && !defined(BOOST_NO_CXX11_LAMBDAS)
# define BOOST_NO_CXX11_LAMBDAS 1
#endif

//#if !defined(__cpp_local_type_template_args)
//# define __cpp_local_type_template_args 190000
//#endif

#if !defined(__cpp_noexcept) && !defined(BOOST_NO_CXX11_NOEXCEPT)
# define BOOST_NO_CXX11_NOEXCEPT 1
#endif

//#if !defined(__cpp_nonstatic_member_init)
//# define __cpp_nonstatic_member_init 190000
//#endif

#if !defined(__cpp_nullptr) && !defined(BOOST_NO_CXX11_NULLPTR)
# define BOOST_NO_CXX11_NULLPTR 1
#endif

//#if !defined(__cpp_override_control)
//# define __cpp_override_control 190000
//#endif

#if !defined(__cpp_reference_qualified_functions) && !defined(BOOST_NO_CXX11_REF_QUALIFIERS)
# define BOOST_NO_CXX11_REF_QUALIFIERS 1
#endif

#if !defined(__cpp_range_for) && !defined(BOOST_NO_CXX11_RANGE_BASED_FOR)
# define BOOST_NO_CXX11_RANGE_BASED_FOR 1
#endif

#if !defined(__cpp_raw_strings) && !defined(BOOST_NO_CXX11_RAW_LITERALS)
# define BOOST_NO_CXX11_RAW_LITERALS 1
#endif

#if !defined(__cpp_rvalue_references) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
# define BOOST_NO_CXX11_RVALUE_REFERENCES 1
#endif

#if !defined(__cpp_static_assert) && !defined(BOOST_NO_CXX11_STATIC_ASSERT)
# define BOOST_NO_CXX11_STATIC_ASSERT 1
#endif

//#if !defined(__cpp_thread_local)
//# define __cpp_thread_local 190000
//#endif

#if !defined(__cpp_auto_type) && !defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
# define BOOST_NO_CXX11_AUTO_DECLARATIONS 1
# define BOOST_NO_CXX11_AUTO_MULTIDECLARATIONS 1
#endif

#if !defined(__cpp_strong_enums) && !defined(BOOST_NO_CXX11_SCOPED_ENUMS)
# define BOOST_NO_CXX11_SCOPED_ENUMS 1
#endif

#if !defined(__cpp_trailing_return) && !defined(BOOST_NO_CXX11_TRAILING_RESULT_TYPES)
# define BOOST_NO_CXX11_TRAILING_RESULT_TYPES 1
#endif

#if !defined(__cpp_unicode_literals) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS)
# define BOOST_NO_CXX11_UNICODE_LITERALS 1
#endif

//#if !defined(__cpp_unrestricted_unions)
//# define __cpp_unrestricted_unions 190000
//#endif

#if !defined(__cpp_user_defined_literals) && !defined(BOOST_NO_CXX11_USER_DEFINED_LITERALS)
# define BOOST_NO_CXX11_USER_DEFINED_LITERALS 1
#endif

#if !defined(__cpp_variadic_templates) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
# define BOOST_NO_CXX11_VARIADIC_TEMPLATES 1
#endif


// C++ 14
#if !defined(__cpp_binary_literals) && !defined(BOOST_NO_CXX14_BINARY_LITERALS)
# define BOOST_NO_CXX14_BINARY_LITERALS 1
#endif

//#if !defined(__cpp_contextual_conversions)
//# define __cpp_contextual_conversions 190000
//#endif

#if !defined(__cpp_decltype_auto) && !defined(BOOST_NO_CXX14_DECLTYPE_AUTO)
# define BOOST_NO_CXX14_DECLTYPE_AUTO 1
#endif

#if !defined(__cpp_aggregate_nsdmi) && !defined(BOOST_NO_CXX14_AGGREGATE_NSDMI)
# define BOOST_NO_CXX14_AGGREGATE_NSDMI 1
#endif

#if !defined(__cpp_digit_separators) && !defined(BOOST_NO_CXX14_DIGIT_SEPARATORS)
# define BOOST_NO_CXX14_DIGIT_SEPARATORS 1
#endif

#if !defined(__cpp_init_captures) && !defined(BOOST_NO_CXX14_INITIALIZED_LAMBDA_CAPTURES)
# define BOOST_NO_CXX14_INITIALIZED_LAMBDA_CAPTURES 1
#endif

#if !defined(__cpp_generic_lambdas) && !defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
# define BOOST_NO_CXX14_GENERIC_LAMBDAS 1
#endif

#if !defined(__cpp_relaxed_constexpr) && !defined(BOOST_NO_CXX14_CONSTEXPR)
# define BOOST_NO_CXX14_CONSTEXPR 1
#endif

#if !defined(__cpp_return_type_deduction) && !defined(BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION)
# define BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION 1
#endif

//#if !defined(__cpp_runtime_arrays)
//# define __cpp_runtime_arrays 190000
//#endif

#if !defined(__cpp_variable_templates) && !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)
# define BOOST_NO_CXX14_VARIABLE_TEMPLATES 1
#endif

#endif
