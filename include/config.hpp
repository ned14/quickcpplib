#ifndef BOOSTLITE_CONFIG_HPP
#define BOOSTLITE_CONFIG_HPP

#include "revision.hpp"

// clang-format off
#define BOOSTLITE_NAMESPACE boost_lite::_ ## BOOSTLITE_PREVIOUS_COMMIT_UNIQUE
#define BOOSTLITE_NAMESPACE_BEGIN namespace boost_lite { inline namespace _ ## BOOSTLITE_PREVIOUS_COMMIT_UNIQUE {
#define BOOSTLITE_NAMESPACE_END } }
// clang-format on

#endif
