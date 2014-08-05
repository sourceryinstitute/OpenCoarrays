# OpenCoarrays #

## Overview ##
This archive contains the [OpenCoarrays](http://www.opencoarrays.org) source code, tests, and documentation. OpenCoarrays is an open-source software project for developing, porting and tuning transport layers that support coarray Fortran compilers.  We target compilers that conform to the coarray parallel programming feature set specified in the Fortran 2008 standard.  

OpenCoarrays shields compilers from platform- and communication-library dependencies by translating a compiler's communication and synchronization requests into calls to one of several communication libraries.  A user of an OpenCoarray-compatible compiler can link compiler-generated object files to the OpenCoarray library deemed most appropriate and efficient for the application and target platform.  

## Compatible Compilers ##
Currently the pre-release GNU Fortran ([GFortran](https://gcc.gnu.org/svn.html)) [4.10/5.0](https://gcc.gnu.org/svn.html) compiler is OpenCoarray-compilable.  If you would like to use OpenCoarrays with a different compiler, please let both the compiler vendor and the OpenCoarrays project team know. 


## Dependencies ##


We intend for our LIBCAF_MPI implementation to be the default on systems that have the Message Passing Interface ([MPI](http://www.mpi-forum.org)) 3.0.  LIBCAF_MPI is the most straightforward to install and use and the most robust in terms of its internal complexity.

We expect to offer a different version of LIBCAF_MPI on systems that have only MPI 2.0. We also offer a LIBCAF_GASNet that builds atop the Global Address Space Networking ([GASNet](http://gasnet.lbl.gov)) communication library.  We intend for LIBCAF_GASNet to be an ``expert'' option capable of outperforming MPI for some applications on some platforms. 


### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](# https://bitbucket.org/tutorials/markdowndemo #)

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact