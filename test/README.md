Garden Test Suite
=================

This subdirectory contains an incomplete set of unit tests that `fpm` can run
after `fpm` is used to build OpenCoarrays.  This is an experimental capability
that uses the [Garden] unit test utility.  If this Garden test suite grows to a
level of comprehensiveness similar to that of the [tests](../tests) that `ctest`
runs after `cmake` is used to build OpenCoarrays, then the `ctest` tests will be
removed and the CMake scripts will be updated to use the Garden tests.

[Garden]: https://gitlab.com/everythingfunctional/garden
