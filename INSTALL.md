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

We have developed a  Portfile that we will submit for inclusion into the
[MacPorts] package management software after posting the OpenCoarrays 1.0.0 
tar ball online.  Once the OpenCoarrays Portfile has been incorporated into 
MacPorts, users can install OpenCoarrays on OS X by installing MacPorts and 
then typing the following:

    sudo port selfupdate  
    sudo port upgrade outdated  
    sudo port install opencoarrays

Administrator privileges are required for issuing the above "sudo" commands.  
Also, the first two steps above are required only if your MacPorts ports 
were last updated prior to the incorporation of the OpenCoarrays Portfile
into MacPorts.  The above process can be repeated to obtain updates in the
form of future OpenCoarrays releases.

<a name="windows">
### Windows ###
</a> 
Windows users will find it easiest to download the Lubuntu Linux virtual 
machine from the [Sourcery Store].  The virtual machine boots inside the 
open-source [VirtualBox] virtualization package.  In addition to containing 
GCC 4.9, 5.2, and 6.0, MPICH, OpenMPI, and OpenCoarrays, the virtual machine 
contains dozens of other open-source software packages that support software 
development in modern Fortran.  See the [download and installation instructions]
for a partial list of the included packages.  

<a name="linux">
### Linux ###
</a>

Linux users who prefer not to build OpenCoarrays from source might access 
OpenCoarrays via the the Lubuntu Linux virtual machine from the [Sourcery Store]
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

* An OpenCoarrays-aware Fortran compiler: [GCC Fortran] 5.1 or later,
* The Fortran compiler's companion C compiler that supports the C99 standard, and
* An MPI implementation that supports the MPI 3.0 standard and was built by the Fortran 
  compiler's companion C compiler (preferably [MPICH] or [MVAPICH] for robustness and performance)

If installing the above prerequisites is infeasible, then a limited coverage of CAF 
features is available via the "caf" compiler wrapper and the [opencoarrays] module,  
for which the installation prerequisites are the following:

* A Fortran compiler that supports the C-interoperability features of Fortran 2003
* The Fortran compiler's companion C compiler that supports the C99 standard.
* An MPI implementation that supports the MPI 3.0 standard and was built by the Fortran 
  compiler's companion C compiler (preferably [MPICH] or [MVAPICH] for robustness and performance)


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
  -DUSE_EXTENSIONS builds the [opencoarrys] module for use with non-OpenCoarrays-aware compilers

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

<a name="compilingcaf">
## Compiling and Executing a CAF Program ##
</a>

<a name="wrapperlauncher">
### The "caf" compiler wrapper and "cafrun" program launcher ###
</a> 
The preferred method for compiling and executing a CAF program is by invoking the "caf" and 
"cafun" bash scripts that the compiler installs in the "bin" subdirectory of the installation
installation path. These are new, experimental scripts with limited capabilities but very
useful capabilities that will grow over time.  Please submit bug reports and feature requests 
via our [Issues](https://github.com/sourceryinstitute/opencoarrays/issues) page.

The "caf" and "cafrun" scripts librarate your source code and workflow from explicit
dependence on the underlying compiler and communication library in the following ways: 

1. "caf" automatically embeds the relevant library path (e.g., libcaf_mpi.a) in the compile command.
2. With a non-CAF compiler (including gfortran 4.9), "caf" supports a subset of CAF by replacing 
   by replacing CAF statements with calls to procedures in the ["opencoarrays" module](src/extensions/opencoarrays.F90).  
3. With non-CAF compilers and non-gfortran CAF compilers, "caf" embeds an argument in the compile
   statement to support accessing OpenCoarrays capabilities via use association (i.e.,
   "use opencoarrays"). To avoid inadvertent procedure procedure name overloading with non-gfortran
   CAF compilers, we recommend adding an "only" clause as in "use opencoarrays, only : co_reduce". 
4. With a non-gfortran CAF compiler (e.g., Cray and Intel), "caf" extends the compiler's capabilities 
   by providing features that are proposed for Fortran 2015 in the draft 
   [Technical Specification TS18508 Additional Parallel Features in Fortran](http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open).

The latter use case provides an opportunity to mix a compiler's CAF support with that of OpenCoarrays.  
For example, a non-gfortran CAF compiler might support all of a program's coarray square-bracket syntax, 
while OpenCoarrays supports the same program's calls to collective subrouine such as co_sum and co_reduce.

### <a name="basicworkflow">A sample basic workflow</a> ###
Inserting the "bin" directory of the chosen installation path into the user's PATH enables the following
CAF program compilation and execution workflow:

  $ cat sum_image_numbers.f90
  program main
    use iso_c_binding, only : c_int
    use iso_fortran_env, only : error_unit
    implicit none
    integer(c_int) :: tally
    tally = this_image() ! this image's contribution 
    call co_sum(tally)
    verify: block
      integer(c_int) :: image
      if (tally/=[(image,image=1,num_images())]) then
         write(error_unit,'(a,i5)') "Incorrect tally on image ",this_image()
         erro stop
      end if
    end block verify
    ! Wait for all images to pass the test
    sync all
    if (this_image()==1) print *,"Test passed"
  end program
  
  $ caf sum_image_numbers.f90
  $ cafrun -np 4 ./sum_image_numbers
  Test passed.

where "4" is the number of images you want to use.

### <a name="advancedworkflow">Sample advanced workflows</a> ###

1. <a name="aware">Use with an OpenCoarrays-aware compiler</a> (e.g., GCC 5.1.0):
A hello.f90 program can compiled in a single step using the following string:

mpif90 -fcoarray=lib -L/opt/opencoarrays/ hello.f90 \ -lcaf_mpi -o hello

For running the program:

mpirun -np m ./hello

2. <a name="noncaf">Use with an non-OpenCoarrays-aware compiler</a> (e.g., gfortran 4.9):  
If a user's compiler provides limited or no coarray support, the user can directly reference a
subset of the OpenCoarrays procedures and types through a Fortran module named "opencoararys"
in the "mod" subdirectory of the user's chosen OpenCoarrays installation path.  The module
provides wrappers for some basic Fortran 2008 parallel programming capabilities along with some 
more advanced features that have been proposed for Fortran 2015 in the draft Technical 
Specification TS18508 Additional Parallel Features in Fortran (visit www.opencoarrays.org for a
link to this document, which gets updated periodically).  An example of the use of the opencoarrays
Fortran module is in the tally_images_numbers.F90 file in the test suite.  The following commands
compile that program:

  cd src/tests/integration/extensions/
  mpif90 -L /opt/opencoarrays/lib/ -I /opt/opencoarrays/mod/ -fcoarray=lib tally_image_numbers.f90 -lcaf_mpi -o tally
  mpirun -np 8 ./tally

3. <a name="nonaware">Use with an non-OpenCoarrays-aware compiler</a> (e.g., Intel or Cray):


## <a name="obtaingcc">Obtaining GCC</a> ##

The Lubuntu Linux virtual machine available for download in the [Sourcery Store](http://www.sourceryinstitute.org/store) includes a very simple buildgcc script that works . 

The coarray support on GFortran is available since release GCC 5.1.

GCC 5 can either be built as outlined below; however, there are also unofficial
binary builds https://gcc.gnu.org/wiki/GFortranBinaries ; some Linux
distributions already offer GCC 5 as preview.

For building GCC from source, first download GCC 5 - either as snapshot from
the mirror or using the latest version by using the SVN trunk/GIT master.
We provide quick instructions on how to build a minimal GCC version.
See also https://gcc.gnu.org/wiki/GFortranBinaries#FromSource and the
official and complete instructions at https://gcc.gnu.org/install/.

1) From the main directory create a build directory using:

   mkdir build

2) GCC requires various tools and packages like GMP, MPFR, MPC. These and others
   can be automatically downloaded typing the following command inside the main
   directory:

   ./contrib/download_prerequisites

3) Inside the build directory type the following configure string customizing
   the installation path:

   ../configure --prefix=/your/path --enable-languages=c,c++,fortran \
                --disable-multilib

4) Type

   make -jN

   where N is the number of cores on your machine + 1. (This may take a while).

5) Type:

   make install

[End-User Installation]: #end-user-installation
[OS X]: #os-x
[Windows]: #windows
[Linux]: #linux
[Building from Source]: #building-from-source
[Prerequisites]: #prerequisites
[CMake]: #cmake
[Make]: #make
[Obtaining GCC]: #obtaining-gcc
[Sourcery Store]: http://www.sourceryinstitute.org/store
[Virtualbox]: http://www.virtualbox.org
[download and installation instructions]: http://www.sourceryinstitute.org/uploads/4/9/9/6/49967347/overview.pdf
[yum]: http://yum.baseurl.org
[apt-get]: https://en.wikipedia.org/wiki/Advanced_Packaging_Tool
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues
[make.inc]: ./src/make.inc
[opencoarrays]: ./src/extensions/opencoarrays.F90
[MPICH]: http://www.mpich.org
[MVAPICH]:http://mvapich.cse.ohio-state.edu
[Macports]: http://www.macports.org

