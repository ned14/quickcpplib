# Quick C++ Libraries

Eliminate all the tedious hassle when making state-of-the-art C++ 14 or 17 libraries!

(C) 2014-2018 Niall Douglas http://www.nedproductions.biz/

Linux & MacOS: [![Build Status](https://travis-ci.org/ned14/quickcpplib.svg?branch=master)](https://travis-ci.org/ned14/quickcpplib) Windows: [![Build status](https://ci.appveyor.com/api/projects/status/8974h34i4i4233vy/branch/master?svg=true)](https://ci.appveyor.com/project/ned14/quickcpplib/branch/master)

CTest dashboard: http://my.cdash.org/index.php?project=QuickCppLib

Documentation: https://ned14.github.io/quickcpplib/

Would you like state-of-the-art modern cmake 3 build with all the fancy features ready to go like address, memory, thread and undefined behaviour sanitisers, clang-tidy linting, doxygen docs generation, git submodule dependency tracking and ABI version stamping, C++ Modules support and more?

Would you like to generate partially preprocessed single header file editions of your library for maximum convenience and ease of install for your end users?

Would you like a collection of highly reusable useful routines and code built on top of the C++ 14 STL?

Then this is the library you've been looking for! Designed for use as an **embedded git submodule**, but can also be used to generate by script libraries which don't look like they are QuickCppLib based e.g. apparently authentic Boost libraries.

**WARNING: This library is in a highly alpha code state. As much as it has been written to a very high quality and has been tested quite extensively in use by other libraries, it remains in substantial flux and is as such a shifting foundation. You have been warned!**

## Requirements:
- GCC 6 or later (Linux)
- clang 3.5 or later (Linux, OS X, BSD, Android)
- Visual Studio 2015 Update 2 or later (Windows)
- cmake 3.1 or later

## Todo:

- [x] Get docs auto updating per commit
- [x] Raise CDash dashboard
- [x] Write `class packed_backtrace`. Add testing.
- [x] Write script which can generate a Boost library from a quickcpplib library, copying just
the parts needed.

## Commits and tags in this git repository can be verified using:
<pre>
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v2

mDMEVvMacRYJKwYBBAHaRw8BAQdAp+Qn6djfxWQYtAEvDmv4feVmGALEQH/pYpBC
llaXNQe0WE5pYWxsIERvdWdsYXMgKHMgW3VuZGVyc2NvcmVdIHNvdXJjZWZvcmdl
IHthdH0gbmVkcHJvZCBbZG90XSBjb20pIDxzcGFtdHJhcEBuZWRwcm9kLmNvbT6I
eQQTFggAIQUCVvMacQIbAwULCQgHAgYVCAkKCwIEFgIDAQIeAQIXgAAKCRCELDV4
Zvkgx4vwAP9gxeQUsp7ARMFGxfbR0xPf6fRbH+miMUg2e7rYNuHtLQD9EUoR32We
V8SjvX4r/deKniWctvCi5JccgfUwXkVzFAk=
=puFk
-----END PGP PUBLIC KEY BLOCK-----
</pre>
