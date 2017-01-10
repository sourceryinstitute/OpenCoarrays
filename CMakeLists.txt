cmake_minimum_required(VERSION 3.0)

# Set the type/configuration of build to perform
set ( CMAKE_CONFIGURATION_TYPES "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "CodeCoverage" )
set ( CMAKE_BUILD_TYPE "Release"
  CACHE STRING "Select which configuration to build." )
set_property ( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

#Name project and specify source languages
project(opencoarrays VERSION 1.8.3 LANGUAGES C Fortran)

#Print an error message on an attempt to build inside the source directory tree:
if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "ERROR! "
    "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    " == CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "\nThis archive does not support in-source builds:\n"
    "You must now delete the CMakeCache.txt file and the CMakeFiles/ directory under "
    "the 'src' source directory or you will not be able to configure correctly!"
    "\nYou must now run something like:\n"
    "  $ rm -r CMakeCache.txt CMakeFiles/"
    "\n"
    "Please create a directory outside the opencoarrays source tree and build under that outside directory "
    "in a manner such as\n"
    "  $ mkdir build-opencarrays\n"
    "  $ cd build-opencoarrays\n"
    "  $ CC=mpicc FC=mpif90 cmake <path-to-opencoarrays-source-directory> -DCMAKE_INSTALL_PREFIX=<path-to-install-directory>\n"
    "\nsubstituting the appropriate syntax for your shell (the above line assumes the bash shell)."
    )
endif()

#Report untested Fortran compiler unless explicitly directed to build all examples.
if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  set(gfortran_compiler true)
  # add_definitions(-DPREFIX_NAME=_gfortran_caf_)
  set ( CMAKE_C_FLAGS_CODECOVERAGE "-fprofile-arcs -ftest-coverage -O0"
    CACHE STRING "Code coverage C compiler flags")
  set ( CMAKE_Fortran_FLAGS_CODECOVERAGE "-fprofile-arcs -ftest-coverage -O0"
    CACHE STRING "Code coverage C compiler flags")
else()
  message(WARNING
    "\n"
    "Attempting to build with untested Fortran compiler: ${CMAKE_Fortran_COMPILER_ID}. "
    "Please report any failures to opencoarrays@googlegroups.com\n\n"
  )
endif()

if (NOT (CMAKE_VERSION VERSION_LESS 3.3.1))
  # Detect Fortran compiler version directly
  if(gfortran_compiler AND (CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER 5.0.0))
    set(opencoarrays_aware_compiler true)
    add_definitions(-DPREFIX_NAME=_gfortran_caf_)
  else()
    set(opencoarrays_aware_compiler false)
    add_definitions(-DPREFIX_NAME=_caf_extensions_)
  endif()
  if(gfortran_compiler AND (CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 5.4))
    # GCC patch to fix issue accepted for 5.4 release
    # See https://github.com/sourceryinstitute/opencoarrays/issues/28 and
    # https://groups.google.com/forum/#!msg/opencoarrays/RZOwwYTqG80/46S9eL696dgJ
    message( STATUS "Disabling optimization flags due to GCC < 5.4 bug")
    set(CMAKE_Fortran_FLAGS_RELEASE -O0
      CACHE STRING "Flags used by the compiler during release builds." FORCE)
    set(CMAKE_Fortran_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O0"
      CACHE STRING "Flags used by the compiler during release builds with debug info" FORCE)
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -O0")
  endif()
else()
  # Use the C compiler version as a proxy for the Fortran compiler version (won't work with NAG)
  if(gfortran_compiler AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 5.0.0))
    set(opencoarrays_aware_compiler true)
    add_definitions(-DPREFIX_NAME=_gfortran_caf_)
  else()
    set(opencoarrays_aware_compiler false)
    add_definitions(-DPREFIX_NAME=_caf_extensions_)
  endif()
  if(gfortran_compiler AND (CMAKE_C_COMPILER_VERSION VERSION_LESS 5.4))
    # GCC patch to fix issue accepted for the 5.4 release
    # See https://github.com/sourceryinstitute/opencoarrays/issues/28 and
    # https://groups.google.com/forum/#!msg/opencoarrays/RZOwwYTqG80/46S9eL696dgJ
    message( STATUS "Disabling optimization flags due to GCC < 5.4 bug")
    set(CMAKE_Fortran_FLAGS_RELEASE -O0
      CACHE STRING "Flags used by the compiler during release builds." FORCE)
    set(CMAKE_Fortran_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O0"
      CACHE STRING "Flags used by the compiler during release builds with debug info" FORCE)
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -O0")
  endif()
endif()

if(gfortran_compiler)
  set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "-fcoarray=single -ffree-form")
endif()
include(CheckFortranSourceCompiles)
CHECK_Fortran_SOURCE_COMPILES("
  program main
    implicit none
    integer :: i
    i = this_image()
  end program
" Check_Simple_Coarray_Fortran_Source_Compiles)
if(gfortran_compiler)
  set (CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
  unset(OLD_REQUIRED_FLAGS)
endif()


#----------------------------------------------------------------------------
# Find MPI and set some flags so that FC and CC can point to gfortran and gcc
#----------------------------------------------------------------------------

# If the user passes FC=mpif90 etc. check and prefer that location
get_filename_component( FTN_COMPILER_NAME "${CMAKE_Fortran_COMPILER}"
  NAME )
get_filename_component( C_COMPILER_NAME "${CMAKE_C_COMPILER}"
  NAME )
get_filename_component( FTN_COMPILER_DIR "${CMAKE_Fortran_COMPILER}"
  REALPATH )
get_filename_component( C_COMPILER_DIR "${CMAKE_C_COMPILER}"
  REALPATH )

if (FTN_COMPILER_NAME MATCHES '^[mM][pP][iI]')
  set (MPI_Fortran_COMPILER "${CMAKE_Fortran_COMPILER}")
endif()
if (C_COMPILER_NAME MATCHES '^[mM][pP][iI]')
  set (MPI_C_COMPILER "${CMAKE_C_COMPILER}")
endif()

find_package( MPI )

if ( (NOT MPI_C_FOUND) OR (NOT MPI_Fortran_FOUND) )
  find_program (MY_MPI_EXEC NAMES mpirun mpiexec lamexec srun
    PATHS "${CMAKE_SOURCE_DIR/prerequisites/installations/mpich/3.1.4}" "${CMAKE_SOURCE_DIR}/prerequisites/installations/mpich/*" ENV PATH
    HINTS "${FTN_COMPILER_DIR}" "${C_COMPILER_DIR}"
    PATH_SUFFIXES bin)
  set ( MPI_HOME "${MPI_HOME}" "${MY_MPI_EXEC}" "${MY_MPI_EXEC}/.." )
  find_package( MPI REQUIRED )
endif()

#--------------------------------------------------------
# Make sure a simple "hello world" C mpi program compiles
#--------------------------------------------------------
set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS ${MPI_C_COMPILE_FLAGS} ${MPI_C_LINK_FLAGS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_PATH})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${MPI_C_LIBRARIES})
include (CheckCSourceCompiles)
CHECK_C_SOURCE_COMPILES("
#include <mpi.h>
#include <stdio.h>
int main(int argc, char** argv) {
  MPI_Init(NULL, NULL);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  printf('Hello world from processor %s, rank %d out of %d processors',
         processor_name, world_rank, world_size);
  MPI_Finalize();
}"
MPI_C_COMPILES)
set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)

if (NOT MPI_C_COMPILES)
  message(FATAL_ERROR "MPI_C is missing! "
    "Try setting MPI_C_COMPILER to the appropriate C compiler wrapper script and reconfigure. "
    "i.e., `cmake -DMPI_C_COMPILER=/path/to/mpicc ..` or set it by editing the cache using "
    "cmake-gui or ccmake."
    )
endif()

#--------------------------------------------------------------
# Make sure a simple "hello world" Fortran mpi program compiles
# Try using mpi.mod first then fall back on includ 'mpif.h'
#--------------------------------------------------------------
set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "-ffree-form" ${MPI_Fortran_COMPILE_FLAGS} ${MPI_Fortran_LINK_FLAGS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_Fortran_INCLUDE_PATH})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
include (CheckFortranSourceCompiles)
CHECK_Fortran_SOURCE_COMPILES("
program mpi_hello
use mpi
implicit none
integer :: ierr, mpi_world_size, mpi_world_rank, res_len
character*(MPI_MAX_PROCESSOR_NAME) :: proc
call mpi_init(ierr)
call mpi_comm_size(MPI_COMM_WORLD,mpi_world_size,ierr)
call mpi_comm_rank(MPI_COMM_WORLD,mpi_world_rank,ierr)
call mpi_get_processor_name(proc,res_len,ierr)
write(*,*) 'Hello from processor ', trim(proc), ' rank ', mpi_world_rank, ' out of ', mpi_world_size, '.'
call mpi_finalize(ierr)
end program
"
MPI_Fortran_MODULE_COMPILES)
set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)

#--------------------------------
# If that failed try using mpif.h
#--------------------------------
set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "-ffree-form" ${MPI_Fortran_COMPILE_FLAGS} ${MPI_Fortran_LINK_FLAGS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_Fortran_INCLUDE_PATH})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
include (CheckFortranSourceCompiles)
CHECK_Fortran_SOURCE_COMPILES("
program mpi_hello
implicit none
include 'mpif.h'
integer :: ierr, mpi_world_size, mpi_world_rank, res_len
character*(MPI_MAX_PROCESSOR_NAME) :: proc
call mpi_init(ierr)
call mpi_comm_size(MPI_COMM_WORLD,mpi_world_size,ierr)
call mpi_comm_rank(MPI_COMM_WORLD,mpi_world_rank,ierr)
call mpi_get_processor_name(proc,res_len,ierr)
write(*,*) 'Hello from processor ', trim(proc), ' rank ', mpi_world_rank, ' out of ', mpi_world_size, '.'
call mpi_finalize(ierr)
end program
"
  MPI_Fortran_INCLUDE_COMPILES)
set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)

if ( (NOT MPI_Fortran_MODULE_COMPILES) AND (NOT MPI_Fortran_INCLUDE_COMPILES) )
  message ( WARNING "It appears that the Fortran MPI compiler is not working. "
    "For OpenCoarrays Aware compilers, this may be irrelavent: "
    "  The src/extensions/opencoarrays.F90 module will be disabled, but it is "
    "  possible that the build will succeed, despite this fishy circumstance."
    )
endif()

if ( MPI_Fortran_MODULE_COMPILES )
  add_definitions(-DMPI_WORKING_MODULE)
else()
  message ( WARNING "It appears that MPI was built with a different Fortran compiler. "
    "It is possible that this may cause unpredictable behavior. The build will continue "
    "using `mpif.h` BUT please report any suspicious behavior to the OpenCoarrays "
    "developers."
    )
endif()

#----------------
# Setup MPI flags
#----------------
set(CMAKE_C_COMPILE_FLAGS ${CMAKE_C_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS})
set(CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS} ${MPI_C_LINK_FLAGS})
set(CMAKE_Fortran_COMPILE_FLAGS ${CMAKE_Fortran_COMPILE_FLAGS} ${MPI_Fortran_COMPILE_FLAGS})
set(CMAKE_Fortran_LINK_FLAGS ${CMAKE_Fortran_LINK_FLAGS} ${MPI_Fortran_LINK_FLAGS})
include_directories(BEFORE ${MPI_C_INCLUDE_PATH} ${MPI_Fortran_INCLUDE_PATH})

#-------------------------------
# Recurse into the src directory
#-------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)

#-----------------------------------------------------
# Publicize installed location to other CMake projects
#-----------------------------------------------------
install(EXPORT OpenCoarraysTargets
  NAMESPACE
    OpenCoarrays::
  DESTINATION
    lib/cmake/opencoarrays
)
include(CMakePackageConfigHelpers) # standard CMake module
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/OpenCoarraysConfigVersion.cmake"
  VERSION "${opencoarrays_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
configure_file("${CMAKE_SOURCE_DIR}/cmake/pkg/OpenCoarraysConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/OpenCoarraysConfig.cmake" @ONLY)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/OpenCoarraysConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/OpenCoarraysConfigVersion.cmake"
  DESTINATION
    lib/cmake/opencoarrays
)

add_library(OpenCoarrays INTERFACE)
target_compile_options(OpenCoarrays INTERFACE -fcoarray=lib)
target_link_libraries(OpenCoarrays INTERFACE caf_mpi)

install(DIRECTORY  ${CMAKE_BINARY_DIR}/mod DESTINATION .)

#------------------------------------------
# Add portable unistall command to makefile
#------------------------------------------
# Adapted from the CMake Wiki FAQ
configure_file ( "${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake.in" "${CMAKE_BINARY_DIR}/uninstall.cmake"
    @ONLY)

add_custom_target ( uninstall
  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/uninstall.cmake" )

enable_testing()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
# See JSON-Fortran's CMakeLists.txt file to find out how to get the check target to depend
# on the test executables

# Determine if we're using Open MPI
execute_process(COMMAND ${MPIEXEC} --version
  OUTPUT_VARIABLE mpi_version_out)
if (mpi_version_out MATCHES "[Oo]pen[ -][Mm][Pp][Ii]")
  message( STATUS "OpenMPI detected")
  set ( openmpi true )
endif ()

include( ProcessorCount )
ProcessorCount(N)
function(add_mpi_test name num_mpi_proc path)
   if ( ((N LESS num_mpi_proc) OR (N EQUAL 0)) )
     message(STATUS "Test ${name} is oversubscribed: ${num_mpi_proc} ranks requested with ${N} system processor available.")
     if ( openmpi )
       if ( N LESS 2 )
	 set( num_mpi_proc 2 )
	 set (test_parameters --oversubscribe)
       else()
	 set ( num_mpi_proc ${N} )
       endif()
       message( STATUS "Open-MPI detected, over-riding oversubscribed test, ${name}, with ${num_mpi_proc} ranks." )
     endif()
   endif()
   set(test_parameters ${test_parameters} ${MPIEXEC_NUMPROC_FLAG} ${num_mpi_proc} )
   add_test(NAME ${name} COMMAND ${MPIEXEC} ${test_parameters} "${path}")
   set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endfunction(add_mpi_test)

set(tests_root ${CMAKE_CURRENT_BINARY_DIR}/src/tests)


if(opencoarrays_aware_compiler)
  # Unit tests targeting each libcaf_mpi function, argument, and branch of code
  add_mpi_test(initialize_mpi 2 ${tests_root}/unit/init_register/initialize_mpi)
  add_mpi_test(register 2 ${tests_root}/unit/init_register/register)
  add_mpi_test(register_vector 2 ${tests_root}/unit/init_register/register_vector)
  add_mpi_test(register_alloc_vector 2 ${tests_root}/unit/init_register/register_alloc_vector)
  add_mpi_test(allocate_as_barrier 2 ${tests_root}/unit/init_register/allocate_as_barrier)
  add_mpi_test(allocate_as_barrier_proc 32 ${tests_root}/unit/init_register/allocate_as_barrier_proc)
  if (gfortran_compiler AND (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 7))
    add_mpi_test(register_alloc_comp_1 2 ${tests_root}/unit/init_register/register_alloc_comp_1)
    add_mpi_test(register_alloc_comp_2 2 ${tests_root}/unit/init_register/register_alloc_comp_2)
    add_mpi_test(register_alloc_comp_3 2 ${tests_root}/unit/init_register/register_alloc_comp_3)
    add_mpi_test(async_comp_alloc 6 ${tests_root}/unit/init_register/async_comp_alloc)
    # Timeout async_comp_alloc test after 3 seconds to progess past the known failure
    set_property(TEST async_comp_alloc PROPERTY TIMEOUT_AFTER_MATCH 3 "known failure")
    set_property(TEST async_comp_alloc PROPERTY TIMEOUT 6) # in the event old CMake is being used
  endif()
  add_mpi_test(get_array 2 ${tests_root}/unit/send-get/get_array)
  add_mpi_test(get_self 2 ${tests_root}/unit/send-get/get_self)
  add_mpi_test(send_array 2 ${tests_root}/unit/send-get/send_array)
  add_mpi_test(get_with_offset_1d 2 ${tests_root}/unit/send-get/get_with_offset_1d)
  add_mpi_test(whole_get_array 2 ${tests_root}/unit/send-get/whole_get_array)
  add_mpi_test(strided_get 2 ${tests_root}/unit/send-get/strided_get)
  add_mpi_test(strided_sendget 3 ${tests_root}/unit/send-get/strided_sendget)
  add_mpi_test(co_sum 4 ${tests_root}/unit/collectives/co_sum_test)
  add_mpi_test(co_broadcast 4 ${tests_root}/unit/collectives/co_broadcast_test)
  add_mpi_test(co_min 4 ${tests_root}/unit/collectives/co_min_test)
  add_mpi_test(co_max 4 ${tests_root}/unit/collectives/co_max_test)
  add_mpi_test(syncall 32 ${tests_root}/unit/sync/syncall)
  add_mpi_test(syncimages 32 ${tests_root}/unit/sync/syncimages)
  add_mpi_test(syncimages2 32 ${tests_root}/unit/sync/syncimages2)
  add_mpi_test(duplicate_syncimages 8 ${tests_root}/unit/sync/duplicate_syncimages)
  add_mpi_test(co_reduce 4 ${tests_root}/unit/collectives/co_reduce_test)
  add_mpi_test(syncimages_status 32 ${tests_root}/unit/sync/syncimages_status)
  add_mpi_test(syncimages_ring_np3 3 ${tests_root}/unit/sync/syncimages_ring)
  add_mpi_test(syncimages_ring_np13 13 ${tests_root}/unit/sync/syncimages_ring)
  add_mpi_test(syncimages_ring_np23 23 ${tests_root}/unit/sync/syncimages_ring)
  add_mpi_test(simpleatomics 32 ${tests_root}/unit/simple/atomics)
  # possible logic error in the following test
#  add_mpi_test(increment_my_neighbor 32 ${tests_root}/unit/simple/increment_my_neighbor)

  # Integration tests verifying the use of libcaf_mpi in applications
  add_mpi_test(hello_multiverse 2 ${tests_root}/integration/coarrayHelloWorld/hello_multiverse)
  add_mpi_test(coarray_burgers_pde 2 ${tests_root}/integration/pde_solvers/coarrayBurgers/coarray_burgers_pde)
  add_mpi_test(co_heat 2 ${tests_root}/integration/pde_solvers/coarrayHeatSimplified/co_heat)
  add_mpi_test(asynchronous_hello_world 3 ${tests_root}/integration/events/asynchronous_hello_world)
  if ( ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64") AND ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") )
    if ( (NOT (DEFINED ENV{TRAVIS})) AND (NOT SKIP_ASSEMBLY_DEPS) )
      add_mpi_test(coarray_navier_stokes 2 ${tests_root}/integration/pde_solvers/navier-stokes/coarray_navier_stokes)
      set_property(TEST coarray_navier_stokes PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
    endif()
  endif()

  # Regression tests based on reported issues
  add_mpi_test(convert-before-put 3 ${tests_root}/regression/reported/convert-before-put)
  add_mpi_test(event-post 3 ${tests_root}/regression/reported/event-post)
else()
  add_test(co_sum_extension ${tests_root}/unit/extensions/test-co_sum-extension.sh)
  set_property(TEST co_sum_extension PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")

  add_test(co_broadcast_extension ${tests_root}/unit/extensions/test-co_broadcast-extension.sh)
  set_property(TEST co_broadcast_extension PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")

  add_test(co_min_extension ${tests_root}/unit/extensions/test-co_min-extension.sh)
  set_property(TEST co_min_extension PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")

  add_test(co_max_extension ${tests_root}/unit/extensions/test-co_max-extension.sh)
  set_property(TEST co_max_extension PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")

  add_test(co_reduce_extension ${tests_root}/unit/extensions/test-co_reduce-extension.sh)
  set_property(TEST co_reduce_extension PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endif()

include(cmake/AddInstallationScriptTest.cmake )
add_installation_script_test(installation-scripts.sh src/tests/installation/)
