---
title: 'OpenCoarrays'
tags:
 - Fortran 2018
 - Parallel Programming
authors:
 - name: Damian Rouson
 - orcid:
 - affiliation: 1
 - name: Alessandro Fanfarillo
 - orcid:
 - affiliation:
 - name: Izaak "Zaak" Beekman
 - orcid:
 - affiliation: 1
 - name: Soren Rasmussen
 - orcid:
 - affiliation: 2
 - name: Daniel Celis Garza
 - orcid:
 - affiliation: 3
affiliations:
 - name: Sourcery Institute
  index: 1
 - name: Cranfield Universty
  index: 2
 - name: University of Oxford
  index: 3
date: 29 March 2018
---

# Summary

OpenCoarrays is an open-source software project
that supports the coarray Fortran (CAF) parallel programming features
of the Fortran 2008 standard and several features proposed for Fortran
2015 in the draft Technical Specification [TS 18508] _Additional
Parallel Features in Fortran_.

OpenCoarrays provides a compiler wrapper (named `caf`), a runtime
library (named `libcaf_mpi.a` by default), and an executable file
launcher (named `cafrun`). With OpenCoarrays-aware compilers, the
compiler wrapper passes the provided source code to the chosen
compiler (`mpifort` by default). For non-OpenCoarrays-aware compilers,
the wrapper transforms CAF syntax into OpenCoarrays procedure calls
before invoking the chosen compiler on the transformed code. The
runtime library supports compiler communication and synchronization
requests by invoking a lower-level communication library--the Message
Passing Interface (MPI) by default. The launcher passes execution
to the chosen communication library's parallel program launcher
(`mpiexec` by default).

OpenCoarrays defines an application binary interface (ABI) that
translates high-level communication and synchronization requests into
low-level calls to a user-specified communication library. This
design decision liberates compiler teams from hardwiring
communication-library choice into their compilers and it frees Fortran
programmers to express parallel algorithms once and reuse identical
CAF source with whichever communication library is most efficient for
a given hardware platform. The communication substrate for
OpenCoarrays built with the preferred build system, CMake, is the
Message Passing Interface (MPI).

OpenCoarrays enables CAF application developers to express parallel
algorithms without hardwiring a particular version of a particular
communication library or library version into their codes. Such
abstraction makes application code less sensitive to the evolution of
the underlying communication libraries and hardware platforms.

# Compatibility

The GNU Compiler Collection (GCC) Fortran front end (gfortran) is
OpenCoarrays-aware for release versions 5.1.0 and higher. Users of
other compilers, including earlier versions of gfortran, can access a
limited subset of CAF features via the provided opencoarrays module.
After installation, please execute the `caf` script (which is
installed in the `bin` directory of the installation path) with no
arguments to see a list of the corresponding limitations. Please also
notify the corresponding compiler vendor and the OpenCoarrays team
that you would like for a future version of the compiler to be
OpenCoarrays-aware.

# Prerequisites

We expect our LIBCAF_MPI library to be the default OpenCoarrays
library. LIBCAF_MPI is the most straightforward to install and use,
the most robust in terms of its internal complexity, and the most
frequently updated and maintained. Building LIBCAF_MPI requires prior
installation of an MPI implementation. We recommend MPICH generally
or, if available, MVAPICH for better performance. OpenMPI is
another option.

We offer an unsupported LIBCAF_GASNet alternative. We intend for
LIBCAF_GASNet to be an "expert" alternative capable of outperforming
MPI for some applications on some platforms. LIBCAF_GASNet requires
greater care to configure and use and building LIBCAF_GASNet requires
prior installation of GASNet.

# Acknowledgements

# References
