[This document is formatted with GitHub-Flavored Markdown.               ]:#
[For better viewing, including hyperlinks, read it online at             ]:#
[https://github.com/sourceryinstitute/opencoarrays/blob/master/INSTALL.md]:#

# [Installing OpenCoarrays](#installing-opencoarrays)

 *  [End-User Installation]
     * [OS X]
     * [Windows]
     * [Linux]
 *  [Building from Source]
     *  [Prerequisites]
     *  [CMake]
     *  [Make]
 *  [Obtaining GCC]

<a name="end-user-installation">
## End-User Installation ##
</a>

This section explains the options most end users will find simplest to obtain 
OpenCoarrays on OS X, Windows, or Linux without building OpenCoarrays from its 
source code.

<a name="os-x">
### OS X###
</a> 

For now, OS X users will find it easiest to use OpenCoarrays inside the 
Lubuntu virtual machine available in the [Sourcery Institute Store].

In hopes of offering a straightforward, native OS X installation option, we 
have submitted a Portfile  to the [MacPorts] package management software
developers.  Please add yourself to the "cc" list on the corresponding [ticket]
for updates.  Once Macports incorporates the OpenCoarrays Portfile, users will be
to install OpenCoarrays by typing the following:

    sudo port selfupdate  
    sudo port upgrade outdated  
    sudo port install opencoarrays

where the "sudo" command requires Administrator privileges and where the first 
two steps above are required only if the MacPorts ports were last updated prior 
to the incorporation of the OpenCoarrays Portfile into MacPorts.  Repeating the
above steps will also install future OpenCoarrays updates.

<a name="windows">
### Windows ###
</a> 
Windows users will find it easiest to download the Lubuntu Linux virtual 
machine from the [Sourcery Institute Store].  The virtual machine boots inside 
the open-source [VirtualBox] virtualization package.  In addition to containing 
GCC 4.9, 5.2, and 6.0, MPICH, OpenMPI, and OpenCoarrays, the virtual machine 
contains dozens of other open-source software packages that support software 
development in modern Fortran.  See the [download and installation instructions]
for a partial list of the included packages.  

<a name="linux">
### Linux ###
</a>

Linux users who prefer not to build OpenCoarrays from source might access 
OpenCoarrays via the the Lubuntu Linux virtual machine from the [Sourcery Institute Store]
after installing the version of [VirtualBox] that is suitable for the relevant 
Linux distribution.  

Alternatively, if you desire to install using Linux package management software 
such as [yum] or [apt-get], please submit a feature request via our [Issues] page.

<a name="buildingfromsource">
## Building from source ##
</a>

<a name="prerequisits">
### Prerequisites: ###
</a>

For broad coverage of CAF features, ease of installation, and ease of use, first 
install the following:

* An OpenCoarrays-aware Fortran compiler: currently [GCC] 5.1 or later,
* The Fortran compiler's companion C compiler that supports the C99 standard, and
* An MPI implementation that supports MPI 3.0 and is built by the aforementioned  
  Fortran and C compilers (preferably the [MPICH] or [MVAPICH] implementations for 
  robustness and high performance)

The [install_prerequisites] directory contains experimental [buildgcc] and [buildmpich]
bash shell scripts that can download and build any one of several versions of the [GCC] 
C, C++, and Fortran compilers, and [MPICH].  (Because newer parts of the GNU Fortran compiler 
(gfortran) are written in C++, installing the GNU Fortran compiler from source requires 
also installing the GNU C++ compiler (g++).)  The CMAke scripts that build OpenCoarrays
also copy [buildgcc] and [buildmpich] into the "bin" directory of the OpenCoarrays
installation for later use.

We have the buildgcc and buildmpich scripts tested on OS X and Linux. Please submit 
suggestions for improving the scripts to our [Issues] page or preferably suggested edits by 
forking a copy of the [OpenCoarrays] repository, making the suggested edits, and submitting 
a pull request.

If installing the above prerequisites is infeasible, then a limited coverage of CAF 
features is available via the OpenCoarrays "caf" compiler wrapper and the 
[opencoarrays] module, for which the installation prerequisites are the following:

* A Fortran compiler that supports the C-interoperability features of Fortran 2003,
* The Fortran compiler's companion C compiler that supports the C99 standard, and
* An MPI implementation that supports MPI 3.0 and is built by the aforementioned  
  Fortran and C compilers (preferably the [MPICH] or [MVAPICH] implementations for 
  robustness and high performance)

<a name="cmake">
### CMake ###
</a> 
------------------------------------------
[CMake] is the preferred build system.   CMake is a cross-platform Makefile generator that 
includes with the testing tool CTest.  To avoid cluttering or clobbering the source tree, 
our CMake setup requires that your build directory be any directory other than the top-level 
OpenCoarrays source directory.  In a bash shell, the following steps should build 
OpenCoarrays, install OpenCoarrays, build the tests, run the tests, and report the test results:

    tar xvzf opencoarrays.tar.gz
    cd opencoarrays
    mkdir build
    cd build
    CC=mpicc FC=mpif90 cmake .. -DCMAKE_INSTALL_PREFIX=${PWD}
    make 
    make install
    ctest

where the the first part of the cmake line sets the CC and FC environment variables
and the final part of the same line defines the installation path as the present 
working directory ("build").  Please report any test failures to via the OpenCoarrays
[Issues] page.  

A complete installation should result in the creation of the following directories
inside the installation path (.e.g, inside "build" in the above example):

* bin: contains the "caf" compiler wrapper and the "cafrun" launcher bash scripts
* mod: contains the "opencoarrays.mod" module file for use with non-OpenCoarrays-aware compilers
* lib: contains the "libcaf_mpi.a" static library to which codes link for CAF support

Advanced options (most users should not use these):

  -DLEGACY_ARCHITECTURE=OFF enables the use of FFT libraries that employ AVX instructions
  -DHIGH_RESOLUTION_TIMER=ON enables timers that tick once per clock cycle
  -DCOMPILER_SUPPORTS_ATOMICS enables support for the proposed Fortran 2015 events feature
  -DUSE_EXTENSIONS builds the [opencoarrays] module for use with non-OpenCoarrays-aware compilers

The first two flags above are not portable and the third enables code that is incomplete as 
of release 1.0.0.  The fourth is set automatically by the CMake scripts based on the compiler
identity and version.

<a name="make">
### Make ###
</a>

Unlike the Makefiles that CMake generates automatically for the chosen platform, static 
Makefiles require a great deal more maintenance and are less portable.  Also, the static
Makefiles provided with OpenCoarrays lack several important capabilities.  In particular,
they will not build the tests;  they will not build any of the infrastructure for compiling
CAF source with non-OpenCoarrays-aware compilers (that infrastructure includes the 
[opencoarrays] module, the "caf" compiler wrapper, and the "cafrun" program launcher); 
nor do the static Makefiles provide a "make install" option so you will need to manually 
move the desired library from the corresponding source directory to your intended installation 
location as shown below.

If CMake is unavailable, build and install with Make using steps such as the following: 

    tar xvzf opencoarrays.tar.gz
    cd opencoarray/src
    make
    mv mpi/libcaf_mpi.a <installation-path>

For the above steps to succeeed, you might need to edit the [make.inc] file to match your 
system settings.  For example, you might need to remove the "-Werror" option from the 
compiler flags or name a different compiler.  In order to activate efficient strided-array 
transfer support, uncomment the -DSTRIDED flag inside the [make.inc] file.

## <a name="obtaingcc">Obtaining GCC</a> ##

[GCC] 5 binary builds are available at [https://gcc.gnu.org/wiki/GFortranBinaries].  Also,
the Lubuntu Linux virtual machine available for download in the [Sourcery Store] includes 
builds of GCC 4.9, 5.2, and 6.0 as well as a rudimentary script (/opt/sourcery/bin/buildgcc)
that builds [GCC] from source on Linux and OS X. 

To build the most up-to-date version of GCC from source manually, you might first try the 
steps employed in the buildgcc script:

    svn co svn://gcc.gnu.org/svn/gcc/trunk
    cd trunk
    ./contrib/download_prerequisites
    cd ..
    mkdir -p trunk-build
    cd trunk-build
    ../trunk/configure --prefix=${PWD} --enable-languages=c,c++,fortran,lto --disable-multilib --disable-werror
    make -j 2 bootstrap
    make install

where the "2" in the penultimate line launches a multi-threaded build with 2 threads.  Use more
threads for additional speedup, depending on your platform.

See the [GFortran Binaries] web page for additional details and the [Installing GCC] page
for an exhaustive description of the build process and options.

[End-User Installation]: #end-user-installation
[OS X]: #os-x
[ticket]: https://trac.macports.org/ticket/47806
[Windows]: #windows
[Linux]: #linux
[Building from Source]: #building-from-source
[Prerequisites]: #prerequisites
[CMake]: #cmake
[Make]: #make
[Obtaining GCC]: #obtaining-gcc
[Sourcery Store]: http://www.sourceryinstitute.org/store
[Sourcery Institute Store]: http://www.sourceryinstitute.org/store
[Virtualbox]: http://www.virtualbox.org
[download and installation instructions]: http://www.sourceryinstitute.org/uploads/4/9/9/6/49967347/overview.pdf
[yum]: http://yum.baseurl.org
[apt-get]: https://en.wikipedia.org/wiki/Advanced_Packaging_Tool
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues
[make.inc]: ./src/make.inc
[opencoarrays]: ./src/extensions/opencoarrays.F90
[install_prerequisites]: ./install_prerequisites
[buildgcc]: ./install_prerequisites/buildgcc
[buildmpich]: ./install_prerequisites/buildmpich
[MPICH]: http://www.mpich.org
[MVAPICH]:http://mvapich.cse.ohio-state.edu
[Macports]: http://www.macports.org
[GCC]: http://gcc.gnu.org
[TS18508 Additional Parallel Features in Fortran]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open
[GFortran Binaries]:  https://gcc.gnu.org/wiki/GFortranBinaries#FromSource
[Installing GCC]: https://gcc.gnu.org/install/

