# [Coarray Fortran Support Status](#coarray-fortran-support-status) #

 *  [Compiler Status]
     * [OpenCoarrays-Aware Compilers]
     * [Non-OpenCoarrays-Aware CAF Compilers]
     * [Non-CAF Compilers]
 *  [Library Status]
     *  [libcaf_mpi]
     *  [libcaf_gasnet]
     *  [libcaf_x]
     *  [libcaf_single]
 *  [Known Issues]
 *  [To-Do List]

<a name="compiler-status">
## Compiler Status ##</a>

The OpenCoarrays CMake build and test scripts detect the compiler identity, version, and operating system (OS).  The scripts use this information to build and test the approproiate functionality for the compiler and OS. Each current compilers' status falls into one of three categories:

<a name="opencoarrays-aware-caf-compilers">
 * **OpenCoarrays-Aware (OCA) CAF Compilers**</a> 
     * _Definition:_ The compiler translates CAF statements into OpenCoarrays application binary interface ([ABI]) calls.
     * _Example_: GNU Fortran 5.1 or later (see [https://gcc.gnu.org/wiki/Coarray] for the compiler's CAF status..
     * _Use case_: compile most Fortran 2008 coarray programs and some programs that use proposed Fortran 2015 features.
<a name="non-opencoarrays-aware-caf-compilers">
 * **Non-OCA CAF Compilers**</a> 
     * _Definition:_ The compiler supports CAF but does not generate calls to the OpenCoarrays ABI.  
     * _Examples_: Cray compiler (except on CS Series clusters), Intel compiler (except on OS X).  
     * _Use case_: extend the compiler's native CAF using the [opencoarrys module] types and procedures.
<a name="non-caf-compilers">
 * **Non-CAF Compilers**</a> 
     * _Definition_: The compiler does not support CAF itself, but user can.
     * _Examples_: GNU Fortran 4.9 or any compiler not mentioned above.
     * _Use case_: Use the OpenCoarrays "caf" compiler wrapper to compile those CAF  programs for which the proposed Fortran 2015 collective subroutines cover all of the application's communication requirements.
  
We have encountered several research applications that match the latter use case.  If you encounter difficulties, please submit a bug report or feature request via the [Issues] page. Also submit a feature request to the relevant compiler technical support.

The OpenCoarrays team offers contract development and support for making compilers OpenCoarrays-aware.  If this is of interest, please inform the compiler's technical support as well as the OpenCoarrays team.   To contribute code, including documentation and tests, see the [CONTRIBUTIONS] file.  To contribute funding, including funding in support of feature reqeusts, see the [Sourcery Store].

<a name="library-status">
## Library Status ##</a>

* **libcaf_mpi** (Default CMake build): Production transport layer that uses
  the Message Passing Interface (MPI) 3.0 one-sided communication, which 
  exploits a hardware platform's native support for Remote Direct Memory 
  Access (RDMA) if available.
* **libcaf_x** (where x = [CUDA], [OpenMP], [Pthreads], [OpenSHMEM], etc.): the 
  OpenCoarrays [ABI] design facilitates implementation atop any one of several
  low-level parallel programming models, vectorization APIs, or combination 
  thereof. We have performed limited evaluations and research development of 
  versions based on multiple APIs.  Please email the [OpenCoarrays Google Group]
  for contract support on targeting other APIs and hardware, including, for example,
  graphics processing units (GPUs) and heterogeneous CPU/GPU platforms.
* **libcaf_gasnet** (Advanced Make build): Experimental transport layer that 
  is currently out-of-date but might exhibit higher performance than MPI on
  platforms for which GASNet provides a tuned conduit.  Contact the 
  [OpenCoarrays Google Group] for further information.
* **libcaf_armci** (Unsupported): developed for research purposes and evaluation.
* **libcaf_single** (Unsupported): developed to mirror the like-named library that
  is included in GNU Fortran to facilitate compiling single-image (sequential)
  executables from CAF programs in the absence of a parallel communication library.

<a name="known-issues">
## Known Issues ##</a>

 * The [opencoarrays module] and "caf" compiler wrapper do not support the square-bracket
  syntax required for point-to-point communication.  This limitation only impacts 
  [non-CAF compilers]. For a list of other limitations with non-CAF compilers, execute
  the "caf' bash script with no arguments.  The "caf" script is installed in the "bin" 
  subdirectory of the installation path.
* Efficient strided array transfer support is available only for intrinsic types.
* Efficient strided array transfer support is not available for remote-to-remote transfers.
* Overwriting a coarray with itself is not managed efficiently for strided transfers.
* The `co_reduce` collective subroutine requires either GCC 4.9, 5.3, or 6.0 and only 
  supports intrinsic types.
* Communication
     * Vector subscripts are not yet supported
     * For character assignments, some issues with padding and character kind conversions exist.
     * For array assignments, some issues with numeric type conversion exist.

<a name="compiler-side-issues">
## Compiler-side (GCC) Issues ##</a>

 * Allocatable coarrays and derived-type coarrays are supported but 
   derived-type coarrays containing allocatable/pointer components are not 
   yet handled properly.
 * Problems exist with array access to a corray in combination with a scalar
   component access: `coarray(:,:)[i]%comp`.
 * Internal compiler error (ICE) with non-allocatable, polymorphic coarrays and
   `associate` or `select type`

<a name="other-language-features">
## Other Language Features ##</a>
 
 * **Atomics**: Implemented in libcaf_mpi.
 * **Locking**: Implemented in libcaf_mpi.
 * **Critical Sections**:  Implemented in libcaf_mpi.
 * **Collective Subroutines**:
     * Implemented in libcaf_mpi without compiler-side (GCC) support for 
       type finalization or allocatable components of derived-type coarrays.
     * A limited set of intrinsic types and kinds are supported, e.g., 
       integer(c_int), real(c_double), character(kind=c_kind), logical, 
       and complex(c_double).  Users are encouraged to fork the OpenCoarrays
       repository and submit pull requests via [GitHub] with support for 
       additional kinds.  See [CONTRIBUTING] for more information.
  
<a name="to-do-list">
## To-Do List ##</a>

* Additional tests and documentation.
* Improvement of error handling and diagnostics, including but not
  limited to filling the ERRMSG= variable in case of errors.
* Providing a diagnostic mode with run-time consistency checks.
* Better integration with the test cases of GCC.  For more information,
  see the GCC source code files in gcc/testsuite/gfortran.dg/, 
  in particular, the "dg-do run" tests in coarray*f90 and coarray/).

[GitHub]: https://github.com/sourceryinstitute/opencoarrays.git
[To-Do List]: #to-do-list
[foMPI]: http://spcl.inf.ethz.ch/Research/Parallel_Programming/foMPI/
[OpenCoarrays Google Group]: https://groups.google.com/forum/#!forum/opencoarrays
[Sourcery Store]: http://www.sourceryinstitute.org/store
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues 
[Non-OpenCoarrays-Aware CAF Compilers]: #non-opencoarrays-aware
[opencoarrays module]: ./src/extensions/opencoarrays
[ABI]: https://gcc.gnu.org/onlinedocs/gfortran/Function-ABI-Documentation.html#Function-ABI-Documentation
[Compilers]: #compilers
[OpenCoarrays-Aware Compilers]: #opencoarrays-aware-compilers
[Other CAF Compilers]: #other-caf-compilers
[Non-CAF Compilers]: #non-caf-compilers
[non-CAF compilers]: #non-caf-compilers
[OpenCoarrays Libraries]: #opencoarrays-libraries
[libcaf_mpi]: #libcaf-mpi
[libcaf_gasnet]: #libcaf-gasnet
[libcaf-x]: #libcaf-x
[Known Issues]: #known-issues
[Basic Communication Support]: #basic-communication-support
[Other Feature Coverage]: #other-feature-coverage
[TS 18508]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open
