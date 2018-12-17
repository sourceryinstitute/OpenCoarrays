OpenCoarrays for the World
==========================

This document outlines steps to prepare for use of OpenCoarrays 
by non-GNU compilers.  The major steps include

* A new application binary interface (ABI) that uses the Fortran 2018 standard descriptors.
* Code documentation using Doxygen.
* Modify the compiler front-end (using [f18] as the first use case) to generate calls to the new OpenCoarrays ABI.

[f18]: https://github.com/flang-compiler/f18
