# local-bind-cpp-library v0.20 (11th Oct 2014)

(C) 2014 Niall Douglas http://www.nedproductions.biz/

This is a tool which can generate a set of local bindings of some C++ library into some namespace. It does this
by parsing the source C++ library using clang's AST library and extracting a regular expression matched API into
a sequence of template alias, using, and typedefs in the local namespace.

## Usage:

Usage looks like this:

    ./genmap bind/stl11/std/thread BOOST_STL11_THREAD_MAP_ "std::([^_][^:]*)" thread

This would grok through everything declared into namespace std by doing `#include <thread>` not prefixed with an
underscore and not inside another namespace or scope and generate a set of bindings into include/thread.

In case that outputs too much to be portable, you can reduce to a least common subset too:

    ./genmap bind/stl11/std/mutex BOOST_STL11_MUTEX_MAP_ "std::([^_][^:]*)" mutex "boost::([^_][^:]*)" boost/thread.hpp

This takes all items common to both `#include <mutex>` and `#include <boost/thread.hpp>` and generates
bindings just for those binding to the first item, so here it would be std::*. If one regular expression isn't
enough, you can separate multiple regexs with commas e.g.

    ./genmap bind/stl11/std/thread BOOST_STL11_THREAD_MAP_ "std::([^_][^:]*),std::(this_thread::[^_][^:]*)" thread "boost::([^_][^:]*),boost::(this_thread::[^_][^:]*)" boost/thread.hpp

If you put the boost match in front of the std match, you'll get bindings for boost instead of bindings for std.
This lets you have preprocessor macros choose which bindings your library will use. For example, let us say the
library Boost.Spinlock wants some implementation of atomic, mutex, thread and chrono bound into the namespace
`boost::spinlock`, so this is how boost/spinlock.hpp might look, and I have added explanatory comments inline:

    #ifndef BOOST_SPINLOCK_HPP
    #define BOOST_SPINLOCK_HPP

    #include "local-bind-cpp-library/include/import.hpp"
    #ifndef BOOST_SPINLOCK_V1_STL11_IMPL
    #define BOOST_SPINLOCK_V1_STL11_IMPL std
    #endif

This says we wish to default to using the standard C++ 11 STL. If someone defined BOOST_SPINLOCK_V1_STL11_IMPL to be 'boost'
instead, then the forthcoming binds would bind the boost implementation instead of the std implementation.

    #define BOOST_SPINLOCK_V1 (boost), (spinlock), (BOOST_LOCAL_BIND_NAMESPACE_VERSION(v1, BOOST_SPINLOCK_V1_STL11_IMPL), inline)

Here we say that we are implementing the library boost::spinlock. The v1_stlimpl will be appended if and only if the
compiler understands inline namespaces.

    #define BOOST_SPINLOCK_V1_NAMESPACE       BOOST_LOCAL_BIND_NAMESPACE      (BOOST_SPINLOCK_V1)
    #define BOOST_SPINLOCK_V1_NAMESPACE_BEGIN BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_SPINLOCK_V1)
    #define BOOST_SPINLOCK_V1_NAMESPACE_END   BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_SPINLOCK_V1)

    #define BOOST_STL11_ATOMIC_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_SPINLOCK_V1, (stl11, inline))
    #define BOOST_STL11_ATOMIC_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_SPINLOCK_V1, (stl11, inline))

Some preprocessor metaprogramming generates the correct namespacing boilerplate, so the above sets
BOOST_SPINLOCK_V1_NAMESPACE to `boost::spinlock::v1`, BOOST_SPINLOCK_V1_NAMESPACE_BEGIN to
`namespace boost { namespace spinlock { inline namespace v1 {` and BOOST_SPINLOCK_V1_NAMESPACE_END to the
correct number of closing braces. It naturally follows from there that BOOST_STL11_ATOMIC_MAP_NAMESPACE_BEGIN
expands into `namespace boost { namespace spinlock { inline namespace v1 { inline namespace stl11 {` which is
where we are requesting some `atomic<>` implementation to become bound into. The following is therefore obvious:
    
    #define BOOST_STL11_CHRONO_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_SPINLOCK_V1, (stl11, inline), (chrono))
    #define BOOST_STL11_CHRONO_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_SPINLOCK_V1, (stl11, inline), (chrono))
    #define BOOST_STL11_MUTEX_MAP_NAMESPACE_BEGIN         BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_SPINLOCK_V1, (stl11, inline))
    #define BOOST_STL11_MUTEX_MAP_NAMESPACE_END           BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_SPINLOCK_V1, (stl11, inline))
    #define BOOST_STL11_THREAD_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_SPINLOCK_V1, (stl11, inline))
    #define BOOST_STL11_THREAD_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_SPINLOCK_V1, (stl11, inline))
    #include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_SPINLOCK_V1_STL11_IMPL, atomic)

The BOOST_LOCAL_BIND_INCLUDE_STL11 generates the path to the bindings include file for whatever atomic
implementation you chose, so if BOOST_SPINLOCK_V1_STL11_IMPL were std it will bind into
`namespace boost { namespace spinlock { inline namespace v1 { inline namespace stl11 {` the atomic
implementation of `std::atomic`.

    #include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_SPINLOCK_V1_STL11_IMPL, chrono)
    #include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_SPINLOCK_V1_STL11_IMPL, mutex)
    #include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_SPINLOCK_V1_STL11_IMPL, thread)
    
    ... other includes ...
    
    BOOST_SPINLOCK_V1_NAMESPACE_BEGIN
    
    BOOST_LOCAL_BIND_DECLARE(BOOST_SPINLOCK_V1)
    
    ... boost.spinlock implementation goes here ...
    
    BOOST_SPINLOCK_V1_NAMESPACE_END
    
    #endif // BOOST_SPINLOCK_HPP
    
    #ifdef BOOST_SPINLOCK_MAP_NAMESPACE_BEGIN
    #include "spinlock.bind.hpp"
    #endif

Having bound in whichever implementation of the C++ 11 STL you wish (i.e. external code gets to choose)
into the local implementation namespace, Boost.Spinlock implementation code can simply use `atomic<T>`
with no namespace qualifier and the user selected implementation gets chosen automatically. Each library
can mash up its own particular mix of externally chosen STL implementations as so desired.

You might note above that after the header guard exits, we test for BOOST_SPINLOCK_MAP_NAMESPACE_BEGIN
and if defined we include the bindings for the library we just defined! This facility allows code wishing
to bind Boost.Spinlock into their local namespace to simply do:

    #define BOOST_SPINLOCK_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(MY_NAMESPACE, (spinlock))
    #define BOOST_SPINLOCK_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (MY_NAMESPACE, (spinlock))
    #include "boost/spinlock.hpp"
    
Or more directly:

    #define BOOST_SPINLOCK_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(MY_NAMESPACE, (spinlock))
    #define BOOST_SPINLOCK_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (MY_NAMESPACE, (spinlock))
    #include "boost/spinlock.bind.hpp"


##  Why on earth would you possibly want such a thing?

Ah, because now you can write code without caring where your dependent library comes from. Let's say you're
writing code which could use either the C++ 11 STL or Boost as a substitute - the former is declared into namespace
std, the latter into namespace boost. Ordinarily you have to `#ifdef` your way along, either at every point
of use, or write some brittle bindings which manually replicate STL items into an internal namespace as a thin
shim to the real implementation.

Thanks to template aliasing in C++ 11 onwards, all that shim
writing goes away! You can now write code **once**, using an internal namespace containing your preferred combination
of dependent library implementation, and you no longer need to care about who is the real implementation.

This lets you custom configure, per library, some arbitrary mix of dependent libraries. For example, if you want
all C++ 11 STL *except* for future<T> promise<T> which you want from Boost.Thread, this binding tool makes
achieving that far easier. You generate your bindings and then hand edit them to reflect your particular
custom configuration, shipping the custom configuration as an internal implementation detail. Much easier!

It also lets one break off Boost libraries away from Boost and become capable of being used standalone or
with a very minimal set of dependencies instead of dragging in the entire of Boost for even a small library.
This makes actual modular Boost usage possible (at last!).


## What state is this tool in?

Very, very early. It's basically hacked together right now, and doesn't yet support namespace binding, variable
binding and probably a few other things like unions. It does mostly support template type binding, enum binding
(both scoped and traditional) and function binding well enough to provide usable bindings for most of the C++ 11 STL:

* array
* atomic
* chrono
* condition_variable
* filesystem
* functional
* future
* mutex
* random
* ratio
* regex
* system_error
* thread
* tuple
* type_traits
* typeindex

Your compiler *must* support template aliasing for these bindings to work, and you can find a prebuilt set
targeting Boost and the STL in:

* bind/stl11/boost/* - these bind the Boost implementation
* bind/stl11/std/* - these bind the std implementation
