# OpenCoarrays #

1. [Overview](#overview)
2. [Compatibility](#compatibility)
3. [Prerequisites](#prerequisites)
4. [Installation](#installation)
5. [Contributing](#contributing)
6. [Acknowledgements](#acknowledgements)

## <a name="overview">Overview</a> ##
[OpenCoarrays](http://www.opencoarrays.org) is an open-source software project that supports the coarray Fortran (CAF) parallel programming features of the Fortran 2008 standard and several features proposed for Fortran 2015 in the draft Technical Specification [TS18508 Additional Parallel Features in Fortran](http://isotc.iso.org/livelink/livelink?func=ll&objId=16769292&objAction=Open).

OpenCoarrays provides a compiler wrapper (named "caf"), a runtime library (named "libcaf_mpi.a" by default), and an executable file launcher (named "cafrun").  With OpenCoarrays-aware compilers, the compiler wrapper passes the provided source code to the chosen compiler ("mpif90" by default).  For non-OpenCoarrays-aware compilers, the wrapper transforms CAF syntax into OpenCoarrys procedure calls before invoking the chosen compiler on the transformed code.  The runtime library supports compiler communication and synchronization requests by invoking a lower-level communication library -- the Message Passing Interface ([MPI](http://www.mpi-forum.org)) by default.  The launcher passes execution to the chosen communication library's parallel program launcher ("mpirun" by default). 

OpenCoarrays defines an application binary interface ([ABI](https://gcc.gnu.org/onlinedocs/gfortran/Function-ABI-Documentation.html#Function-ABI-Documentation)) that translates high-level communication and synchronization requests into low-level calls to a user-specified communication library.  This design decision liberates compiler teams from hardwiring communication-library choice into their compilers and it frees Fortran programmers to express parallel algorithms once and reuse identical CAF source with whichever communication library is most efficient for a given hardware platform.  The communication substrate for OpenCoarrays built with the preferred build system, CMake, is the Message Passing Interface ([MPI](http://www.mpi-forum.org)) and the Global Address Space Network ([GASNet](http://gasnet.lbl.gov)).

OpenCoarrays enables CAF application developers to express parallel algorithms without hardwiring a particular version of a particular communication library into their codes.  Such abstraction makes application code less sensitive to the evolution of the underlying communication libraries and hardware platforms.

## <a name="compatibility">Compatibility</a> ##
The GNU Compiler Collection ([GCC](http://gcc.gnu.org)) Fortran front ([gfortran](https://gcc.gnu.org/wiki/GFortran)) release versions 5.1.0 or later are OpenCoarrays-aware.  Users of other compilers, including earlier versions of gfortran, can access a limited subset of CAF features via the provided ``opencoarrays'' [module](src/extensions/opencoarrays.F90).  After installation, please execute the "caf" script (which is installed in the "bin" directory of the installation path) with no arguments to see a list of the corresponding limitations.  Please also notify the corresponding compiler vendor and the OpenCoarrays team that you would like for a future version of the compiler to be OpenCoarrays-aware.

## <a name="prerequisites">Prerequisites</a> ##
We expect our LIBCAF_MPI library to be the default OpenCoarrays library.  LIBCAF_MPI is the most straightforward to install and use, the most robust in terms of its internal complexity, and the most frequently updated and maintained.  Building LIBCAF_MPI requires prior installation of an MPI implementation.  We recommend [MPICH](http://www.mpich.org) generally or, if available, [MVAPICH](http://mvapich.cse.ohio-state.edu/) for better performance. [OpenMPI](http://www.open-mpi.org) is another option.

We offer an unsupported LIBCAF_GASNet alternative.  We intend for LIBCAF_GASNet to be an "expert" alternative capable of outperforming MPI for some applications on some platforms.  LIBCAF_GASNet requires greater care to configure and use and building LIBCAF_GASNet requires prior installation of [GASNet](http://gasnet.lbl.gov).

## <a name="installation">Installation</a> ##

Please see the [INSTALL](INSTALL) file.

## <a name="contributing">Contributing</a> ##

Please see the [CONTRIBUTING](CONTRIBUTING) file.

## <a name="support">Support</a> ##

* Please submit bug reports and feature requests via our [Issues](https://github.com/sourceryinstitute/opencoarrays/issues) page or [Google Group](https://groups.google.com/forum/#!forum/opencoarrays).  To subscribe to the Google Group, log in with your Google account or [subscribe](https://groups.google.com/forum/#!forum/opencoarrays/join) with any email address.
* We offer an additional [menu](http://opencoarrays.org/services) of services on a contract basis.

## <a name="acknowledgements">Acknowledgements</a> ##
We gratefully acknowledge support from the following institutions:

* [National Center for Atmospheric Research](http://ncar.ucar.edu) for access to the Yellowstone/Caldera supercomputers and for logistics support during the initial development of OpenCoarrays.
* [CINECA](http://www.cineca.it/en) for access to Eurora/PLX for the project HyPS- BLAS under the ISCRA grant program for 2014.
* [Google](http://google.com) for support of a related [Google Summer of Code](https://www.google-melange.com) 2014 project.
* The National Energy Research Scientific Computing Center ([NERSC](http://www.nersc.gov)), which is supported by the Office of Science of the U.S. Department of Energy under Contract No. DE-AC02-05CH11231, for access to the Hopper and Edison supercomputers under the OpenCoarrays project start allocation.
* [Sourcery, Inc.](http://www.sourceryinstitute.org), for financial support for the domain registration, web hosting, advanced development, and conference travel.
