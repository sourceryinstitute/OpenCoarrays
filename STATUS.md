<a name="top"> </a>

[This document is formatted with GitHub-Flavored Markdown.              ]:#
[For better viewing, including hyperlinks, read it online at            ]:#
[https://github.com/sourceryinstitute/opencoarrays/blob/master/STATUS.md]:#

OpenCoarrays Status
===================

[![Download as PDF][pdf img]](http://md2pdf.herokuapp.com/sourceryinstitute/opencoarrays/blob/master/STATUS.pdf)

Download this file as a PDF document
[here](http://md2pdf.herokuapp.com/sourceryinstitute/opencoarrays/blob/master/STATUS.pdf).

 *  [Feature Coverage](#feature-coverage)
 *  [Compiler Status](#compiler-status)
     * [OpenCoarrays-Aware (OCA) Coarray Fortran (CAF) Compilers]
     * [Non-OCA CAF Compilers]
     * [Non-CAF Compilers]
 *  [Library Status](#libary-status)
     *  [libcaf_mpi]
     *  [libcaf_x]
     *  [libcaf_gasnet]
     *  [libcaf_armci]
     *  [libcaf_single]
 *  [Known Issues](#known-issues)
     * [Library Issues](#library-issues)
     * [Compiler Issues](#compiler-issues)
         * [GNU (gfortran)]
         * [Cray (ftn)]
         * [Intel (ifort)]
         * [Numerical Algorithms Group (nagfor)]
         * [Portland Group (pgfortran)]
         * [IBM (xlf)]
 *  [To-Do List](#to-do-list)

Feature Coverage
----------------

 * Except as noted under [Known Issues], [libcaf_mpi] supports the following features as described
   in the Fortran 2008 standard:
     * allocatable and non-allocatable coarrays of intrinsic or derived type
     * synchronization statements
     * atomics
     * locks
     * critical
 * Except as noted under [Known Issues], [libcaf_mpi] supports the collective
   subroutines proposed for Fortran 2015 in the draft Technical Specification
   [TS 18508] _Additional Parallel Features in Fortra_ subroutines for a limited
   set of intrinsic types and kinds.  Adding additional types and kinds is
   straightforward.  Please submit a request via the [Issues] page or consider
   adding the requisite code by [forking the OpenCoarrays repository] and submitting
   [pull request via GitHub]. Also see [CONTRIBUTING.md] for more information.

Compiler Status
---------------

The OpenCoarrays CMake build and test scripts detect the compiler identity, version, and operating system (OS).  The scripts use this information to build and test the approproiate functionality for the compiler and OS. Each current compilers' status falls into one of three categories:

<a name="oca-caf-compilers">
 * **OpenCoarrays-Aware (OCA) Coarray Fortran (CAF) Compilers**</a>
     * _Definition:_ The compiler translates CAF statements into OpenCoarrays application binary interface ([ABI]) calls.
     * _Example_: GNU Fortran 5.1 or later (see <https://gcc.gnu.org/wiki/Coarray> for the compiler's CAF status..)
     * _Use case_: compile most Fortran 2008 coarray programs and some programs that use proposed Fortran 2015 features.
<a name="non-oca-caf-compilers">
 * **Non-OCA CAF Compilers**</a>
     * _Definition:_ The compiler supports CAF but does not generate calls to the OpenCoarrays [ABI].
     * _Examples_: Cray compiler (except on CS Series clusters), Intel compiler (except on OS X).
     * _Use case_: extend the compiler's native CAF using the [opencoarrays module] types and procedures.
<a name="non-caf-compilers">
 * **Non-CAF Compilers**</a>
     * _Definition_: The compiler provides no direct support for CAF, but the user can access a subset of CAF features via use association with the [opencoarrays module], e.g., `use opencoarrays, only : co_sum`.
     * _Examples_: GNU Fortran 4.9 or any compiler not mentioned above.
     * _Use case_: Use the OpenCoarrays `caf` compiler wrapper to compile those CAF  programs for which the proposed Fortran 2015 collective subroutines cover all of the application's communication requirements.

We have encountered several research applications that match the latter use case.  If you encounter difficulties, please submit a bug report or feature request via the [Issues] page. Also submit a feature request to the relevant compiler technical support.

The OpenCoarrays team offers contract development and support for making compilers OpenCoarrays-aware.  If this is of interest, please inform the compiler's technical support as well as the OpenCoarrays team.   To contribute code, including documentation and tests, see the [CONTRIBUTING.md] file.  To contribute funding, including funding in support of feature reqeusts, see the [Sourcery Store].

Library Status
--------------

<a name="libcaf-mpi">
* **libcaf_mpi**</a> (Default CMake build): Production transport layer that uses
  the Message Passing Interface ([MPI]) 3.0 one-sided communication, which
  exploits a hardware platform's native support for Remote Direct Memory
  Access (RDMA) if available.
<a name="libcaf-x">
* **libcaf_x**</a> (where x = [CUDA], [OpenMP], [Pthreads], [OpenSHMEM], etc.): the
  OpenCoarrays [ABI] design facilitates implementation atop any one of several
  low-level parallel programming models, vectorization APIs, or combination
  thereof. We have performed limited evaluations and research development of
  versions based on multiple APIs.  Please email the [OpenCoarrays Google Group]
  for support with targeting other APIs and hardware, including, for example,
  graphics processing units (GPUs) and heterogeneous CPU/GPU platforms.
<a name="libcaf-gasnet">
* **libcaf_gasnet**</a> (Advanced Make build): Experimental transport layer that
  is currently out-of-date but might exhibit higher performance than [MPI] on
  platforms for which [GASNet] provides a tuned conduit.  Contact the
  [OpenCoarrays Google Group] for further information.
<a name="libcaf-armci">
* **libcaf_armci**</a> (Unsupported): developed for research purposes and evaluation.
<a name="libcaf-single">
* **libcaf_single**</a> (Unsupported): developed to mirror the like-named library that
  is included in GNU Fortran to facilitate compiling single-image (sequential)
  executables from CAF programs in the absence of a parallel communication library.

Known Issues
------------

### Library Issues ###

* The [opencoarrays module] and `caf` compiler wrapper do not support the square-bracket
  syntax required for point-to-point communication.  This limitation only impacts
  [non-CAF compilers]. For a list of other limitations with non-CAF compilers, execute
  the `caf` bash script with no arguments.  The `caf` script is installed in the `bin`
  subdirectory of the installation path.
* Efficient strided array transfer works only for intrinsic types.
* Efficient strided array transfer is not supported for remote-to-remote transfers.
* Overwriting a coarray with itself is not managed efficiently for strided transfers.
* Communication
     * Vector subscripts are not yet supported
     * For character assignments, some issues with padding and character kind conversions exist.
     * For array assignments, some issues with numeric type conversion exist.


### Compiler Issues ###

<a name="compiler-issues-gnu">
* **GNU** (gfortran)</a>
     * Derived-type coarrays with allocatable/pointer components are not yet handled
       properly.
     * Problems exist with combining array access to a corray with a scalar component
        access as in `coarray(:,:)[i]%comp`.
     * An internal compiler error (ICE) occurs with non-allocatable, polymorphic coarrays
       in `associate` or `select type` statements.
     * `co_reduce` with GCC 5 and 6 requires patches committed on 17 July 2015 and will
       work with the GCC 5.3.0 and 6.1.0 releases.
     * `co_reduce` only supports arguments of intrinsic type.
     * No support for type finalization or allocatable components of derived-type coarrays
       passed to the collective subroutines (e.g., `co_sum`, `co_reduce`, etc.).
	 * Optimization levels other than `-O0` introduce correctness errors
	   in the compiled binaries. A patch has been submitted by @afanfa
	   to the GFortran team. See #28 for some more context.
<a name="compiler-issues-intel">
* **Intel** (ifort)</a>
     * Supported via the [opencoarrays module]  only.
<a name="compiler-issues-cray">
* **Cray** (ftn) </a>
     * Supported via the [opencoarrays module] only.
<a name="compiler-issues-nag">
* **Numerical Algorithms Group** (nagfor)</a>
     * Supported via the [opencoarrays module] only.
<a name="compiler-issues-pg">
* **Portland Group** (pgfortran)</a>
     * Supported via the [opencoarrays module] only.
<a name="compiler-issues-ibm">
* **IBM** (xlf)</a>
     * Supported via the [opencoarrays module] only.

To-Do List
----------

* [ ] Additional tests and documentation.
* [ ] Improvement of error handling and diagnostics, including but not
      limited to filling the `ERRMSG=` variable in case of errors.
* [ ] Providing a diagnostic mode with run-time consistency checks.
* [ ] Better integration with the test cases of GCC.  For more information,
      see the GCC source code files in `gcc/testsuite/gfortran.dg/`,
      in particular, the `dg-do run` tests in `coarray*f90` and `coarray/`).


[Hyperlinks]:#
   [OpenMP]: http://openmp.org
   [CUDA]: http://www.nvidia.com/object/cuda_home_new.html
   [Pthreads]: https://computing.llnl.gov/tutorials/pthreads/
   [MPI]: http://www.mpi-forum.org
   [OpenSHMEM]: http://openshmem.org
   [GASNet]: https://gasnet.lbl.gov
   [CONTRIBUTING.md]: ./CONTRIBUTING.md
   [OpenCoarrays-Aware (OCA) Coarray Fortran (CAF) Compilers]: #oca-caf-compilers
   [Known Issues]: #known-issues
   [Non-OCA CAF Compilers]: #non-oca-caf-compilers
   [Non-CAF Compilers]: #non-caf-compilers
   [libcaf_mpi]: #libcaf-mpi
   [libcaf_x]: #libcaf-x
   [libcaf_gasnet]:  #libcaf-gasnet
   [libcaf_single]: #libcaf-single
   [libcaf_armci]: #libcaf-armci
  [GNU (gfortran)]: #compiler-issues-gnu
  [Cray (ftn)]: #compiler-issues-cray
  [Intel (ifort)]: #compiler-issues-intel
  [Numerical Algorithms Group (nagfor)]: #compiler-issues-nag
  [Portland Group (pgfortran)]: #compiler-issues-pg
  [IBM (xlf)]: #compiler-issues-ibm
  [forking the OpenCoarrays repository]: https://github.com/sourceryinstitute/opencoarrays/blob/master/STATUS.md#fork-destination-box

[TS 18508]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open
[opencoarrays module]: ./src/extensions/opencoarrays.F90
[ABI]: https://gcc.gnu.org/onlinedocs/gfortran/Function-ABI-Documentation.html#Function-ABI-Documentation
[pull requests via GitHub]: https://github.com/sourceryinstitute/opencoarrays/compare
[pull request via GitHub]: https://github.com/sourceryinstitute/opencoarrays/compare
[OpenCoarrays Google Group]: https://groups.google.com/forum/#!forum/opencoarrays
[Sourcery Store]: http://www.sourceryinstitute.org/store
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues
[pdf img]: https://img.shields.io/badge/PDF-STATUS.md-6C2DC7.svg?style=flat-square "Download as PDF"
