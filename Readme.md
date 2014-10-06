# local-bind-cpp-library v0.01 (6th Oct 2014)

(C) 2014 Niall Douglas http://www.nedproductions.biz/

This is a tool which can generate a set of local bindings of some C++ library into some namespace. It does this
by parsing the source C++ library using clang's AST library and extracting a regular expression matched API into
a sequence of template alias, using, and typedefs in the local namespace.

Usage looks like this:

    ./genmap include/thread STL11_MAP_ "std::([^_].*)" thread

This would grok through everything declared into namespace std by doing
`#include <thread>` not prefixed with an underscore and generate a set of bindings into include/thread.

In case that outputs too much to be portable, you can reduced to a least common subset too:

    ./genmap include/mutex STL11_MAP_ "std::([^_].*)" mutex "boost::([^_].*)" boost/thread.hpp

This takes all items common to both `#include <mutex>` and `#include <boost/thread.hpp>` and generates
bindings just for those. If one regular expression isn't enough, you can separate multiple regexs with commas.

To then include those local namespace bindings into let's say namespace `boost::spinlock` one simply does this:

    #define STL11_MAP_BEGIN_NAMESPACE namespace boost { namespace spinlock { inline namespace stl {
    #define STL11_MAP_END_NAMESPACE } } }
    #include "local-bind-cpp-library/include/atomic"
    #include "local-bind-cpp-library/include/chrono"
    #include "local-bind-cpp-library/include/mutex"
    #include "local-bind-cpp-library/include/thread"
    #undef STL11_MAP_BEGIN_NAMESPACE
    #undef STL11_MAP_END_NAMESPACE


##  Why on earth would you possibly want such a thing?

Ah, because now you can write code without caring where your dependent library comes from. Let's say you're
writing code which could use the C++ 11 STL or Boost as a substitute - the former is declared into namespace
std, the latter into namespace boost. With the above bindings, you no longer need to manually bind functions
from some macro determined source into your code, you simply import bindings into your project's namespace
and use without namespace prefixing - whichever STL implementation is bound at the top is what gets picked
up by your library.

This lets you custom configure, per library, some arbitrary mix of dependent libraries. For example, if you want
all C++ 11 STL *except* for future<T> promise<T> which you want from Boost.Thread, this binding tool makes
achieving that far easier.

It also lets one break off Boost libraries away from Boost and become capable of being used standalone or
with a very minimal set of dependencies. This makes actual modular Boost usage possible (at last!).

## What state is this tool in?

Very, very early. It's basically hacked together right now, and doesn't yet support typedef binding, variable
binding or function binding. It does mostly support template type binding and enum binding (both scoped and
traditional) well enough to provide usable bindings for most of the C++ 11 STL (bundled in include/stl11) for:

* array
* atomic
* chrono
* condition_variable
* filesystem (defaulted to Boost's implementation)
* future
* mutex
* random
* ratio
* system_error
* thread
* tuple (the lack of make_tuple is a pain though)
* type_traits
* typeindex

Your compiler *must* support template aliasing for these bindings to work.

Bindings really needing function binding support to be useful are:

* functional
* regex
