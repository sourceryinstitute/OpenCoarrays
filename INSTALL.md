<a name="top"> </a>

[This document is formatted with GitHub-Flavored Markdown.               ]:#
[For better viewing, including hyperlinks, read it online at             ]:#
[https://github.com/sourceryinstitute/opencoarrays/blob/master/INSTALL.md]:#

Installing OpenCoarrays
=======================

[![Download as PDF][pdf img]](http://md2pdf.herokuapp.com/sourceryinstitute/opencoarrays/blob/master/INSTALL.pdf)

Download this file as a PDF document
[here](http://md2pdf.herokuapp.com/sourceryinstitute/opencoarrays/blob/master/INSTALL.pdf).

 *  [End-User Installation]
     * [Installation Script]
     * [OS X]
     * [Windows]
     * [Linux]
 *  [Advanced Installation from Source]
     *  [Prerequisites]
     *  [CMake]
     *  [Make]
 *  [Obtaining GCC]

End-User Installation
---------------------

### Installation Script###

As of release 1.2.0, users might consider installing by downloading and uncompressing
a file from our [Releases] page and running the installation script in the top-level
source directory:

```
tar xvzf opencoarrays-x.y.z.tar.gz
cd opencoarrays
./install.sh
```

Before installing OpenCoarrays, the above bash script will attempt to detect the presence
of the default prequisite packages: [GCC], [MPICH] , and [CMake].  For additional details, see the [Prerequisites] section. If any of the
aforementioned packages appear to be absent from the user's PATH environment variable,
the [install.sh] script will attempt to download, build, and install any missing packages
after asking permission to do so.  The script has been tested on Linux and OS X.  Please
submit any related problems or questions to our [Issues] page.

A complete installation should result in the creation of the following directories
inside the installation path (.e.g, inside `build` in the above example):

* `bin`: contains the compiler wrapper (`caf`), program launcher (`cafun`), and prerequisites builder (`build`)
* `mod`: contains the `opencoarrays.mod` module file for use with non-OpenCoarrays-aware compilers
* `lib`: contains the `libcaf_mpi.a` static library to which codes link for CAF support

The remainder of this document explains other options that many end users will find
simplest to obtain OpenCoarrays on OS X, Windows, or Linux without building OpenCoarrays
from its source code.

### OS X ###

OS X users might find it easiest to install OpenCoarrays using the [MacPorts]
package management system.  After installing [MacPorts], type the following:

```
sudo port selfupdate
sudo port upgrade outdated
sudo port install opencoarrays
```

where the `sudo` command requires administrator privileges and where the first
two steps above are required only if the [MacPorts] ports were last updated prior
to 30 September 2015, when the OpenCoarrays port was incorporated into [MacPorts].
Repeating the first two steps above will also update OpenCoarrays to the latest
release.

Please also install the `mpstats` port as follows:

```
sudo port install mpstats
```

which supports future OpenCoarrays development by providing download data the
OpenCoarrays team can use in proposals for research grants and development
contracts.

### Windows ###

Windows users will find it easiest to download the Lubuntu Linux virtual
machine from the [Sourcery Institute Store].  The virtual machine boots inside
the open-source [VirtualBox] virtualization package.  In addition to containing
GCC 4.9, 5.2, and 6.0, MPICH, OpenMPI, and OpenCoarrays, the virtual machine
contains dozens of other open-source software packages that support software
development in modern Fortran.  See the [download and installation instructions]
for a partial list of the included packages.

Alternatively, if you desire to use OpenCoarrays under Cygwin, please submit a
feature request via our [Issues] page.

### Linux ###

The [Arch Linux] distribution provides an [aur package] for installing OpenCoarrays.
Users of other Linux distributions who prefer not to build OpenCoarrays from source might
access OpenCoarrays via the the Lubuntu Linux virtual machine from the
[Sourcery Institute Store] after installing the version of [VirtualBox] that is suitable
for the relevant Linux distribution.  Alternatively, if you desire to install using other
Linux package management software such as [yum] or [apt-get], please submit a feature
request via our [Issues] page.

Advanced Installation from Source
--------------------

### Prerequisites: ###

The prerequisites below and their dependencies are recommended for the broadest coverage of CAF features. If a prerequisite is missing or outdated, the [install.sh] script will prompt the user for permission to download, compile, and install it. Before doing so, [install.sh] will verify whether that prerequisite's prerequisites are present and will recursively traverse the dependency tree until reaching an acceptable prerequisite or reaching the end of a branch.

```
opencoarrays
├── cmake-3.4.0
└── mpich-3.1.4
    └── gcc-5.3.0
        ├── flex-2.6.0
        │   └── bison-3.0.4
        │       └── m4-1.4.17
        ├── gmp
        ├── mpc
        └── mpfr
```

If using the advanced [CMake] or [Make] builds detailed below, please ensure that these dependencies are met before attempting to build and install OpenCoarrays.

### CMake ###

[CMake] is the preferred build system.   CMake is a cross-platform Makefile generator that
includes with the testing tool CTest.  To avoid cluttering or clobbering the source tree,
our CMake setup requires that your build directory be any directory other than the top-level
OpenCoarrays source directory.  In a bash shell, the following steps should build
OpenCoarrays, install OpenCoarrays, build the tests, run the tests, and report the test results:

```
tar xvzf opencoarrays.tar.gz
cd opencoarrays
mkdir opencoarrays-build
cd opencoarrays-build
CC=mpicc FC=mpif90 cmake .. -DCMAKE_INSTALL_PREFIX=${PWD}
make
ctest
make install
```

where the the first part of the cmake line sets the CC and FC environment variables
and the final part of the same line defines the installation path as the present
working directory (`opencoarrays-build`).  Please report any test failures via the
OpenCoarrays [Issues] page.

Advanced options (most users should not use these):

    -DLEGACY_ARCHITECTURE=OFF enables the use of FFT libraries that employ AVX instructions
    -DHIGH_RESOLUTION_TIMER=ON enables timers that tick once per clock cycle
    -DCOMPILER_SUPPORTS_ATOMICS enables support for the proposed Fortran 2015 events feature
    -DUSE_EXTENSIONS builds the [opencoarrays] module for use with non-OpenCoarrays-aware compilers
    -DCOMPILER_PROVIDES_MPI is set automatically when building with the Cray Compiler Environment

The first two flags above are not portable and the third enables code that is incomplete as
of release 1.0.0.  The fourth is set automatically by the CMake scripts based on the compiler
identity and version.

### Make ###

Unlike the Makefiles that CMake generates automatically for the chosen platform, static
Makefiles require a great deal more maintenance and are less portable.  Also, the static
Makefiles provided with OpenCoarrays lack several important capabilities.  In particular,
they will not build the tests;  they will not build any of the infrastructure for compiling
CAF source with non-OpenCoarrays-aware compilers (that infrastructure includes the
[opencoarrays] module, the `caf` compiler wrapper, and the `cafrun` program launcher);
nor do the static Makefiles provide a `make install` option so you will need to manually
move the desired library from the corresponding source directory to your intended installation
location as shown below.

If CMake is unavailable, build and install with Make using steps such as the following:

```
tar xvzf opencoarrays.tar.gz
cd opencoarray/src
make
mv mpi/libcaf_mpi.a <installation-path>
```

For the above steps to succeed, you might need to edit the [make.inc] file to match your
system settings.  For example, you might need to remove the `-Werror` option from the
compiler flags or name a different compiler.  In order to activate efficient strided-array
transfer support, uncomment the `-DSTRIDED` flag inside the [make.inc] file.

Obtaining GCC, MPICH, and CMake
-------------------------------

[GFortran Binaries] 5 binary builds are available at <https://gcc.gnu.org/wiki/GFortranBinaries>.  Also,
the Lubuntu Linux virtual machine available for download in the [Sourcery Store] includes
builds of GCC 4.9, 5.2, and 6.0.

To build all prerequisites from source, including the current development branch of GCC,
you might first try the running the provided [install.sh] script as described above in
the [Installation Script] section.  Or try building each prerequisite from source as
follows:

```
cd install_prerequisites
CC=gcc FC=gfortran CXX=g++ ./build flex
./build gcc
CC=gcc FC=gfortran CXX=g++ ./build mpich
./build cmake
```

where the second line builds the flex package that is required for building gcc from source.

[Links]: #

[End-User Installation]: #end-user-installation
[Installation Script]: #installation-script
[install.sh]: ./install.sh
[OS X]: #os-x
[ticket]: https://trac.macports.org/ticket/47806
[Windows]: #windows
[Linux]: #linux
[Advanced Installation from Source]: #advanced-installation-from-source
[Prerequisites]: #prerequisites
[CMake]: #cmake
[Make]: #make
[Obtaining GCC]: #obtaining-gcc
[Sourcery Store]: http://www.sourceryinstitute.org/store
[Sourcery Institute Store]: http://www.sourceryinstitute.org/store
[VirtualBox]: http://www.virtualbox.org
[download and installation instructions]: http://www.sourceryinstitute.org/uploads/4/9/9/6/49967347/overview.pdf
[yum]: http://yum.baseurl.org
[apt-get]: https://en.wikipedia.org/wiki/Advanced_Packaging_Tool
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues
[make.inc]: ./src/make.inc
[opencoarrays]: ./src/extensions/opencoarrays.F90
[install_prerequisites]: ./install_prerequisites
[MPICH]: http://www.mpich.org
[MVAPICH]:http://mvapich.cse.ohio-state.edu
[MacPorts]: http://www.macports.org
[GCC]: http://gcc.gnu.org
[TS18508 Additional Parallel Features in Fortran]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open
[GFortran Binaries]:  https://gcc.gnu.org/wiki/GFortranBinaries#FromSource
[Installing GCC]: https://gcc.gnu.org/install/
[Arch Linux]: https://www.archlinux.org
[aur package]: https://aur.archlinux.org/packages/opencoarrays/
[Releases]: https://github.com/sourceryinstitute/opencoarrays/releases
[pdf img]: https://img.shields.io/badge/PDF-INSTALL.md-6C2DC7.svg?style=flat-square "Download as PDF"
