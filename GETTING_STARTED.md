# [Getting Stared](#getting-started)

 *  [Compiling a CAF Program]
     * [The caf compiler wrapper]
     * [A sample basic workflow]
     * [Sample advanced workflows]
 *  [Executing a CAF Program]
     * [The cafrun program launcher]

<a name="compiling-a-caf-program">
## Compiling a CAF Program ##
</a>

<a name="the-caf-compiler-wrapper">
### The caf compiler wrapper ###
</a> 

The preferred method for compiling a CAF program is by invoking the "caf" bash script 
that the OpenCoarrays CMake scripts install in the "bin" subdirectory of the installation
path. This is an experimental script with limited but useful capabilities that will 
grow over time.  Please submit bug reports and feature requests via our [Issues] page.

The "caf" script liberates the source code and workflow from explicit dependence on the
underlying compiler and communication library in the following ways: 

1. "caf" automatically embeds the relevant library path (e.g., libcaf_mpi.a) in the compile command.
2.  With a non-CAF compiler (including gfortran 4.9), "caf" supports a subset of CAF by replacing 
    CAF statements with calls to procedures in the [opencoarrays] module.  
3.  With non-CAF compilers and non-gfortran CAF compilers, "caf" includes in the compile statement
    the .mod file that the CMake scripts install in the "mod" suddierctory of the installation path.  
    This enables uers to access OpenCoarrays capabilities via use association (i.e.  "use opencoarrays"). 
    To avoid inadvertent procedure name overloading with non-gfortra CAF compilers, we recommend using 
    statements of the form "use opencoarrays, only : co_reduce" to reduce the number name classes.
4.  With a non-gfortran CAF compiler (e.g., Cray and Intel), "caf" extends the compiler's capabilities 
    by providing features that are proposed for Fortran 2015 in the draft Technical Specification 
    [TS18508 Additional Parallel Features in Fortran].

The latter use case provides an opportunity to mix a compiler's CAF support with that of OpenCoarrays.  
For example, a non-gfortran CAF compiler might support all of a program's coarray square-bracket syntax, 
while OpenCoarrays supports the same program's calls to collective subrouine such as co_sum and co_reduce.

<a name="basicworkflow">
### A sample basic workflow ###
</a>

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

where "4" is the number of images to be launched at program star-up.

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


## <a name="obtaingcc">Executing a CAF Program</a> ##

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

[Sourcery Store]: http://www.sourceryinstitute.org/store
[Virtualbox]: http://www.virtualbox.org
[Issues]: https://github.com/sourceryinstitute/opencoarrays/issues
[opencoarrays]: ./src/extensions/opencoarrays.F90
[GCC]: http://gcc.gnu.org
[TS18508 Additional Parallel Features in Fortran]: http://isotc.iso.org/livelink/livelink?func=ll&objId=17181227&objAction=Open
[The caf compiler wrapper]: the-caf-compiler-wrapper
[The cafrun program launcher]: the-cafrun-program-launcher
[Compiling a CAF Program]: compiling-a-caf-program
[A sample basic workflow]: a-sample-basic-workflow
[Advanced workflows]: advanced-workflows
[Executing a CAF program]: executing-a-caf-program
