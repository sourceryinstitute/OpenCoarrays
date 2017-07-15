#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([OpenCoarrays], [1.9.0], [opencoarrays@googlegroups.com])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror foreign tar-ustar])
AC_CONFIG_SRCDIR([src/libcaf.h])
AC_CONFIG_HEADERS([config.h])
LT_INIT
AC_SUBST([LIBTOOL_DEPS])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_PROG_FC(gfortran)
AC_CHECK_PROG(MPI_EXEC,[mpiexec],[mpiexec],[no])
test "$MPI_EXEC" == "no" && AC_MSG_ERROR([Required program 'mpiexec' not found.])

# Check for working gfortran.
AC_DEFUN([LIBGFOR_WORKING_GFORTRAN], [
AC_MSG_CHECKING([whether the GNU Fortran compiler is working])
AC_LANG_PUSH([Fortran])
AC_COMPILE_IFELSE([[
      program foo
      real, parameter :: bar = sin (12.34 / 2.5)
      end program foo]],
    [AC_MSG_RESULT([yes])
     AC_DEFINE(HAVE_GFORTRAN, 1, [Define if you have gfortran.])],
    [AC_MSG_RESULT([no])
     AC_MSG_ERROR([GNU Fortran is not working; please report a bug in http://gcc.gnu.org/bugzilla, attaching $PWD/config.log])
    ])
AC_LANG_POP([Fortran])
])
LIBGFOR_WORKING_GFORTRAN

# Check for gfortran version >= 7.
AC_DEFUN([LIBGFOR_GFORTRAN_UP], [
AC_MSG_CHECKING([whether the Fortran compiler supports failed images])
AC_LANG_PUSH([Fortran])
AC_COMPILE_IFELSE([[
      program foo
      use iso_fortran_env , only : STAT_STOPPED_IMAGE
      implicit none
      integer :: i
      associate(np => num_images(), me => this_image())
      do i= 1, np
      if (image_status(i) /= 0) error stop 1
      end do
      sync all
      end associate
      end program foo]],
    [AC_MSG_RESULT([yes])
     AC_DEFINE(HAVE_FORTRAN_FAILED_IMAGES, 1, [Define if you have failed images support.])
     FAILED_IMAGES="yes"],
    [AC_MSG_RESULT([no])
    ])
AC_LANG_POP([Fortran])
])
FCFLAGS="-fcoarray=single"
LIBGFOR_GFORTRAN_UP
FCFLAGS=""

# Checks for libraries.
#if pkg-config --atleast-version=3.2 mpich; then
   #MPI_Fortran_LINK_FLAGS=`pkg-config --libs-only-other --libs-only-L mpich`
   #MPI_LIBS=`pkg-config --libs-only-l mpich`
   #MPI_INCLUDE=`pkg-config --cflags-only-I mpich`
#else
   #AC_MSG_ERROR(pkg-config has no mpich package, try module load mpi?)
#fi

AC_SEARCH_LIBS(MPI_Init, mpi, [], AC_MSG_ERROR([Unable to find mpi library - make sure LD path to the library is set]), [])

# Set substitution variables needed to build caf, cafrun, and libcaf_mpi
AC_SUBST(PROJECT_VERSION)
AC_SUBST(opencoarrays_aware_compiler)
AC_SUBST(Fortran_COMPILER)
AC_SUBST(CAF_MODDIR)
AC_SUBST(MOD_DIR_FLAG)
AC_SUBST(CAF_LIBS)
AC_SUBST(MPI_Fortran_LINK_FLAGS)
AC_SUBST(MPI_Fortran_COMPILE_FLAGS)
AC_SUBST(MPI_LIBS)
AC_SUBST(MPI_INCLUDE)
AC_SUBST(FAILED_IMAGES)

PROJECT_VERSION=$PACKAGE_VERSION
Fortran_COMPILER=$(which $FC)
CAF_MODDIR="."
MOD_DIR_FLAG=-I
CAF_LIBS=$libdir/libcaf_mpi.a
MPI_Fortran_COMPILE_FLAGS="-fPIC"
MPI_Fortran_LINK_FLAGS=$LDFLAGS" -Wl,-rpath"
MPI_LIBS="-lmpi"
opencoarrays_aware_compiler="true"

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h pthread.h])
AC_CHECK_HEADERS([mpi.h], [],
  [AC_CHECK_HEADERS([mpich-x86_64/mpi.h mpich/mpi.h],
  [echo Do something here])
  ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memmove memset])
AC_CHECK_FUNCS([MPI_Init])
AC_CHECK_FUNCS([MPIX_Comm_failure_ack MPIX_Comm_failure_get_acked MPIX_Comm_shrink MPIX_Comm_agree], AC_DEFINE([USE_FAILED_IMAGES], 1, [Define if MPI supports FAILED IMAGES]),[FAILED_IMAGES="no"])

AM_CONDITIONAL([COND_OPT], [test "$FAILED_IMAGES" = yes])

# Configure needed files.
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 src/tests/Makefile
 src/tests/integration/Makefile
 src/tests/integration/coarrayHelloWorld/Makefile
 src/tests/integration/dist_transpose/Makefile
 src/tests/integration/events/Makefile
 src/tests/integration/pde_solvers/Makefile
 src/tests/integration/pde_solvers/coarrayBurgers/include-files/Makefile
 src/tests/integration/pde_solvers/coarrayBurgers/Makefile
 src/tests/integration/pde_solvers/coarrayHeatSimplified/Makefile
 src/tests/unit/Makefile
 src/tests/unit/collectives/Makefile
 src/tests/unit/fail_images/Makefile
 src/tests/unit/init_register/Makefile
 src/tests/unit/send-get/Makefile
 src/tests/unit/simple/Makefile
 src/tests/unit/sync/Makefile
 src/extensions/caf
 src/extensions/cafrun
])
AC_OUTPUT

#echo "Variables *********************************"
#echo $PROJECT_VERSION
#echo $FC
#echo $CC
#echo $CFLAGS
#echo $LDFLAGS
#echo "*******************************************"
#echo $opencoarrays_aware_compiler
#echo $Fortran_COMPILER
#echo $CAF_MODDIR
#echo $MOD_DIR_FLAG
#echo $CAF_LIBS
#echo "*******************************************"
#echo $MPI_Fortran_LINK_FLAGS
#echo $MPI_Fortran_COMPILE_FLAGS
#echo $MPI_LIBS
#echo $libdir
#echo $prefix
#echo "End Variables *****************************"
