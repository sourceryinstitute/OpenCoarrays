<a name="top"> </a>

[This document is formatted with GitHub-Flavored Markdown.               ]:#
[For better viewing, including hyperlinks, read it online at             ]:#
[https://github.com/sourceryinstitute/OpenCoarrays/blob/master/INSTALL.md]:#

# Installing OpenCoarrays #

[![GitHub release](https://img.shields.io/github/release/sourceryinstitute/OpenCoarrays.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/releases/latest)
[![Github All Releases](https://img.shields.io/github/downloads/sourceryinstitute/OpenCoarrays/total.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/releases/latest)
[![Download as PDF][pdf img]][INSTALL.pdf]

Download this file as a PDF document
[here][INSTALL.pdf].

* [End-User Installation]
  * [macOS]
  * [Windows]
  * [Linux]
  * [FreeBSD]
  * [Virtual machine]
  * [Installation Script]
* [Advanced Installation from Source]
  * [Prerequisites]
  * [CMake scripts]
  * [Make]
* [Obtaining GCC, MPICH, and CMake]

## End-User Installation ##

Most users will find it easiest and fastest to use package management
software to install OpenCoarrays.  Package management options for
macOS (formerly known as OS X), Windows, and Linux are described first
below. Also described below are options for installing via the
Sourcery Institute virtual machine or the OpenCoarrays installation
script.

[top]

### macOS ###

[![homebrew](https://img.shields.io/homebrew/v/opencoarrays.svg?style=flat-square)](http://braumeister.org/formula/opencoarrays)

OS X users may use the [Homebrew] or [MacPorts] package management
systems to install OpenCoarrays.  We recommend [Homebrew].

Basic Homebrew installation steps:

```bash
brew update
brew install opencoarrays
```

OpenCoarrays also ships with a
[`Brewfile`][Brewfile]
that will make it easier to install opencoarrays using MPICH built
with GCC. To install using the
[`Brewfile`][Brewfile]
with MPICH wrapping GCC, follow these steps:

```bash
brew tap homebrew/bundle
brew update
brew bundle
```

MacPorts installation steps:

```bash
sudo port selfupdate
sudo port upgrade outdated
sudo port install opencoarrays
```

where the `sudo` command requires administrator privileges.  If you
install using MacPorts, please also install the `mpstats` port as
follows:

```bash
sudo port install mpstats
```

which supports future OpenCoarrays development by providing download
data the OpenCoarrays team uses in proposals for research grants and
development contracts.

[top]

### Windows ###

Windows users may run the windows-install.sh script inside the Windows
Subsystem for Linux (WSL).

Requirements:

* WSL release 14936 or later,
* Windows Insider Preview, and
* "Fast" updates option.

Steps:

```bash
 do-release-upgrade
./windows-install.sh
```

where the first command above updates the default Ubuntu 14.04 to
16.04 and the second command must be executed with the present working
directory set to the top level of the OpenCoarrays source tree.

The `windows-install.sh` installation script uses Ubuntu's `apt-get`
package manager to build [GCC] 5.4.0, [CMake], and [MPICH].  Windows
users who desire a newer version of GCC are welcome to submit a
request via our [Issues] page and suggest a method for updating.
Previously attempted upgrade methods are described in the discussion
thread starting with [commit comment 20539810].

[top]

### Linux ###

The [Arch Linux] distribution provides an [aur package] for installing
OpenCoarrays.  Users of other Linux distributions may install the
[Virtual machine] or use the [Installation Script].  Alternatively, if
you desire to install using other Linux package Linux package
management software such as [dnf] or [apt-get], please submit a
feature request via our [Issues] page.

[top]

### FreeBSD ###

A FreeBSD Port is available for installing OpenCoarrays and can be
located via [port search].  See the FreeBSD OpenCoarrays [port details] 
page for installation instructions.

[top]

## Virtual machine ##

Users of macOS, Windows, or Linux have the option to use OpenCoarrays
by installing the Lubuntu Linux virtual machine from the
[Sourcery Institute Store].  The virtual machine boots inside the
open-source [VirtualBox] virtualization package.  In addition to
containing [GCC], [MPICH], and OpenCoarrays, the virtual machine
contains dozens of other open-source software packages that support
modern Fortran software development.  See the
[download and installation instructions] for a partial list of the
included packages.

[top]

## Installation Script ##

If the above package management or virtualization options are
infeasible or unavailable, Linux and macOS users may also install
OpenCoarrays by downloading and uncompressing our [latest release] and
running our installation script in the top-level OpenCoarrays source
directory (see above for the corresponding [Windows] script):

```bash
tar xvzf OpenCoarrays-x.y.z.tar.gz
cd OpenCoarrays-x.y.z
./install.sh
```

where `x.y.z` should be replaced with the appropriate version numbers.
For a scripted or unattended build, use `./install.sh -y` or
equivalently `./install.sh --yes-to-all`, which will assume
affirmative answers to all user prompts and will only prompt the user
if an installation directory is chosen that requires `sudo`
privelenges (by passing `-i` or equivalently `--install-prefix`).

Before installing OpenCoarrays, the above bash script will attempt to
detect the presence of the default prerequisite packages: [GCC],
[MPICH] , and [CMake].  For additional details, see the
[Prerequisites] section. If any of the installation script cannot find
the prerequisite packages, the [install.sh] script will attempt to
download, build, and install any missing packages after asking
permission to do so.  The script has been tested on Linux and OS X.
Please submit any related problems or questions to our [Issues] page.

A complete installation should result in the creation of the following
directories inside the installation path (.e.g, inside `build` in the
above example):

* `bin`: contains the compiler wrapper (`caf`) and program launcher
  (`cafun`).
* `mod`: contains the `opencoarrays.mod` module file for use with
  non-OpenCoarrays-aware compilers
* `lib`: contains the `libcaf_mpi.a` static library to which codes
  link for CAF support

[top]

## Advanced Installation from Source ##

### Prerequisites ###

The prerequisites below and their dependencies are recommended for the
broadest coverage of CAF features.  If a prerequisite is missing or
outdated, the [install.sh] script will prompt the user for permission
to download, compile, and install it. Before doing so, [install.sh]
will verify whether that prerequisite's prerequisites are present and
will recursively traverse the dependency tree until reaching an
acceptable prerequisite or reaching the end of a branch.

```text
opencoarrays
├── cmake-3.4.0
└── mpich-3.2
    └── gcc-6.1.0
        ├── flex-2.6.0
        │   └── bison-3.0.4
        │       └── m4-1.4.17
        ├── gmp
        ├── mpc
        └── mpfr
```

If using the advanced [CMake] or [Make] builds detailed below, please
ensure that these dependencies are met before attempting to build and
install OpenCoarrays.

[top]

### CMake scripts ###

#### N.B. ####

__As of OpenCoarrays 1.7.6, passing `FC=mpi_fortran_wrapper` and
`CC=mpi_c_wrapper` is *DEPRECATED*. Please pass `FC=/path/to/gfortran`
and `CC=/path/to/gcc`. If you are experimenting with the source to
source translation capabilities, then please point `FC` and `CC` to
your Fortran and C compilers of choice. In the case of Cray, or a
compiler in which MPI is *built-in* to the compiler, you still pass
the `FC` and `CC` as the Fortran and C compiler, even though MPI is
built-in.__

[CMake] is the preferred build system.  CMake is a cross-platform
Makefile generator that includes the testing tool CTest.  To avoid
cluttering or clobbering the source tree, our CMake setup requires
that your build directory be any directory other than the top-level
OpenCoarrays source directory.  In a bash shell, the following steps
should build OpenCoarrays, install OpenCoarrays, build the tests, run
the tests, and report the test results:

```bash
tar xvzf opencoarrays.tar.gz
cd opencoarrays
mkdir opencoarrays-build
cd opencoarrays-build
CC=gcc FC=gfortran cmake .. -DCMAKE_INSTALL_PREFIX=${HOME}/packages/
make
ctest
make install
```

where the the first part of the cmake line sets the CC and FC
environment variables and the final part of the same line defines the
installation path as the `packages` directory in the current user's
`$HOME` directory.  Please report any test failures via the
OpenCoarrays [Issues] page. Please note that you need a recent
GCC/GFortran, and a recent MPI-3 implementation. If CMake is having
trouble finding the MPI implementation, or is finding the wrong MPI
implementation, you can try setting the `MPI_HOME` environment
variable to point to the installation you wish to use. If that fails,
you can also try passing the
`-DMPI_Fortran_COMPILER=/path/to/mpi/fortran/wrapper/script` and
`-DMP_C_COMPILER=/path/to/mpi/c/wrapper/script` options to CMake.

Advanced options (most users should not use these):

```CMake
-DMPI_HOME=/path/to/mpi/dir  # try to force CMake to find your preferred MPI implementation
        # OR
-DMPI_C_COMPILER=/path/to/c/wrapper
-DMPI_Fortran_COMPILER=/path/to/fortran/wrapper

-DLEGACY_ARCHITECTURE=OFF    # enables the use of FFT libraries that employ AVX instructions
-DHIGH_RESOLUTION_TIMER=ON   # enables timers that tick once per clock cycle
-DCOMPILER_SUPPORTS_ATOMICS  # enables support for the proposed-Fortran 2015 events
                             # feature
-DUSE_EXTENSIONS             # builds the opencoarrays module for use-with non-
                             # OpenCoarrays-aware compilers
-DCOMPILER_PROVIDES_MPI      # is set automatically when building with-the Cray
                             # Compiler Environment
```

The fourth and fifth flags above are not portable and the sixth
enables code that is incomplete as of release 1.0.0.  The eighth is
set automatically by the CMake scripts based on the compiler identity
and version.

[top]

### Make ###

Unlike the Makefiles that CMake generates automatically for the chosen
platform, static Makefiles require a great deal more maintenance and
are less portable.  Also, the static Makefiles provided with
OpenCoarrays lack several important capabilities.  In particular, they
will not build the tests; they will not build any of the
infrastructure for compiling CAF source with non-OpenCoarrays-aware
compilers (that infrastructure includes the [opencoarrays] module, the
`caf` compiler wrapper, and the `cafrun` program launcher); nor do the
static Makefiles provide a `make install` option so you will need to
manually move the desired library from the corresponding source
directory to your intended installation location as shown below.

If CMake is unavailable, build and install with Make using steps such as the following:

```bash
tar xvzf opencoarrays.tar.gz
cd opencoarray/src
make
mv mpi/libcaf_mpi.a <installation-path>
```

For the above steps to succeed, you might need to edit the [make.inc]
file to match your system settings.  For example, you might need to
remove the `-Werror` option from the compiler flags or name a
different compiler.  In order to activate efficient strided-array
transfer support, uncomment the `-DSTRIDED` flag inside the [make.inc]
file.

[top]

## Obtaining GCC, MPICH, and CMake ##

[GFortran Binaries] binary builds are available at <https://gcc.gnu.org/wiki/GFortranBinaries>.

To build all prerequisites from source, including the current
development branch of GCC, you might first try the running the
provided [install.sh] script as described above in the
[Installation Script] section or try building each prerequisite from
source inside a bash shell as follows:

```bash
export gcc_install_path=/desired/installation/destination
./install.sh --package gcc --install-prefix "${gcc_install_path}"
./install.sh --package mpich \
   --with-fortran "${gcc_install_path}"/bin/gfortran \
   --with-c       "${gcc_install_path}"/bin/gcc      \
   --with-cxx     "${gcc_install_path}"/bin/g++
./install.sh --pacakge cmake \
   --with-fortran "${gcc_install_path}"/bin/gfortran \
   --with-c       "${gcc_install_path}"/bin/gcc      \
   --with-cxx     "${gcc_install_path}"/bin/g++
```

[top]

---

<div align="center">

[![GitHub forks](https://img.shields.io/github/forks/sourceryinstitute/OpenCoarrays.svg?style=social&label=Fork)](https://github.com/sourceryinstitute/OpenCoarrays/fork)
[![GitHub stars](https://img.shields.io/github/stars/sourceryinstitute/OpenCoarrays.svg?style=social&label=Star)](https://github.com/sourceryinstitute/OpenCoarrays)
[![GitHub watchers](https://img.shields.io/github/watchers/sourceryinstitute/OpenCoarrays.svg?style=social&label=Watch)](https://github.com/sourceryinstitute/OpenCoarrays)
[![Twitter URL](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?hashtags=HPC,Fortran,PGAS&related=zbeekman,gnutools,HPCwire,HPC_Guru,hpcprogrammer,SciNetHPC,DegenerateConic,jeffdotscience,travisci&text=Stop%20programming%20w%2F%20the%20%23MPI%20docs%20in%20your%20lap%2C%20try%20Coarray%20Fortran%20w%2F%20OpenCoarrays%20%26%20GFortran!&url=https%3A//github.com/sourceryinstitute/OpenCoarrays)

</div>

[Internal document links]: #

[top]: #top
[End-User Installation]: #end-user-installation
[macOS]: #macos
[Windows]: #windows
[Linux]: #linux
[FreeBSD]: #freebsd
[Virtual machine]: #virtual-machine
[Installation Script]: #installation-script

[Advanced Installation from Source]: #advanced-installation-from-source
[Prerequisites]: #prerequisites
[CMake scripts]: #cmake-scripts
[Make]: #make

[Obtaining GCC, MPICH, and CMake]: #obtaining-gcc-mpich-and-cmake

[Links to source]: #

[install.sh]: ./install.sh

[URLs]: #

[Brewfile]: https://github.com/sourceryinstitute/OpenCoarrays/blob/master/Brewfile
[INSTALL.pdf]: https://md2pdf.herokuapp.com/sourceryinstitute/OpenCoarrays/blob/master/INSTALL.pdf
[CMake]: https://cmake.org
[Sourcery Institute Store]: http://www.sourceryinstitute.org/store/c1/Featured_Products.html
[VirtualBox]: https://www.virtualbox.org
[download and installation instructions]: http://www.sourceryinstitute.org/uploads/4/9/9/6/49967347/overview.pdf
[yum]: http://yum.baseurl.org
[apt-get]: https://en.wikipedia.org/wiki/Advanced_Packaging_Tool
[Issues]: https://github.com/sourceryinstitute/OpenCoarrays/issues
[make.inc]: ./src/make.inc
[opencoarrays]: ./src/extensions/opencoarrays.F90
[prerequisites]: #prerequisites
[MPICH]: http://www.mpich.org
[MVAPICH]:http://mvapich.cse.ohio-state.edu
[MacPorts]: https://www.macports.org
[GCC]: http://gcc.gnu.org
[TS18508 Additional Parallel Features in Fortran]: http://isotc.iso.org/livelink/livelink/nfetch/-8919044/8919782/8919787/17001078/ISO%2DIECJTC1%2DSC22%2DWG5_N2056_Draft_TS_18508_Additional_Paralle.pdf?nodeid=17181227&vernum=0
[GFortran Binaries]:  https://gcc.gnu.org/wiki/GFortranBinaries#FromSource
[Installing GCC]: https://gcc.gnu.org/install/
[Arch Linux]: https://www.archlinux.org
[aur package]: https://aur.archlinux.org/packages/opencoarrays/
[latest release]: https://github.com/sourceryinstitute/OpenCoarrays/releases/latest
[pdf img]: https://img.shields.io/badge/PDF-INSTALL.md-6C2DC7.svg?style=flat-square "Download as PDF"
[commit comment 20539810]: https://github.com/sourceryinstitute/OpenCoarrays/commit/26e99919fe732576f7277a0e1b83f43cc7c9d749#commitcomment-20539810
[Homebrew]: https://brew.sh
[dnf]: https://github.com/rpm-software-management/dnf
[port details]: http://www.freshports.org/lang/opencoarrays
[port search]: https://www.freebsd.org/cgi/ports.cgi?query=opencoarrays
