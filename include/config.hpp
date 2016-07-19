#ifndef BOOSTLITE_CONFIG_HPP
#define BOOSTLITE_CONFIG_HPP

#include "revision.hpp"

// clang-format off
#define BOOSTLITE_NAMESPACE boost_lite::_ ## BOOSTLITE_PREVIOUS_COMMIT_UNIQUE
#define BOOSTLITE_NAMESPACE_BEGIN namespace boost_lite { inline namespace _ ## BOOSTLITE_PREVIOUS_COMMIT_UNIQUE {
#define BOOSTLITE_NAMESPACE_END } }
// clang-format on

#ifdef _MSC_VER
#define BOOSTLITE_BIND_MESSAGE_PRAGMA2(x) __pragma(message(x))
#define BOOSTLITE_BIND_MESSAGE_PRAGMA(x) BOOSTLITE_BIND_MESSAGE_PRAGMA2(x)
#define BOOSTLITE_BIND_MESSAGE_PREFIX(type) __FILE__ "(" BOOSTLITE_BIND_STRINGIZE2(__LINE__) "): " type ": "
#define BOOSTLITE_BIND_MESSAGE_(type, prefix, msg) BOOSTLITE_BIND_MESSAGE_PRAGMA(prefix msg)
#else
#define BOOSTLITE_BIND_MESSAGE_PRAGMA2(x) _Pragma(#x)
#define BOOSTLITE_BIND_MESSAGE_PRAGMA(type, x) BOOSTLITE_BIND_MESSAGE_PRAGMA2(type x)
#define BOOSTLITE_BIND_MESSAGE_(type, prefix, msg) BOOSTLITE_BIND_MESSAGE_PRAGMA(type, msg)
#endif
//! Have the compiler output a message
#define BOOSTLITE_MESSAGE(msg) BOOSTLITE_BIND_MESSAGE_(message, BOOSTLITE_BIND_MESSAGE_PREFIX("message"), msg)
//! Have the compiler output a note
#define BOOSTLITE_NOTE(msg) BOOSTLITE_BIND_MESSAGE_(message, BOOSTLITE_BIND_MESSAGE_PREFIX("note"), msg)
//! Have the compiler output a warning
#define BOOSTLITE_WARNING(msg) BOOSTLITE_BIND_MESSAGE_(GCC warning, BOOSTLITE_BIND_MESSAGE_PREFIX("warning"), msg)
//! Have the compiler output an error
#define BOOSTLITE_ERROR(msg) BOOSTLITE_BIND_MESSAGE_(GCC error, BOOSTLITE_BIND_MESSAGE_PREFIX("error"), msg)

#endif
