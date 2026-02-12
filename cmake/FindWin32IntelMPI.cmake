# FindWin32IntelMPI: Find Intel oneAPI MPI for Win32

# Here we assume Intel ONEAPI and the environment is loaded
set( MPI_ASSUME_NO_BUILTIN_MPI TRUE )
set( MPI_CXX_SKIP_MPICXX TRUE )
cmake_path(SET MPI_ROOT NORMALIZE "$ENV{I_MPI_ROOT}")
set (IMPI_LIB_DIR "${MPI_ROOT}/lib")
set (IMPI_DLL_DIR "${MPI_ROOT}/bin")

message(STATUS "Looking in IMPI_LIB_DIR=${IMPI_LIB_DIR}")
message(STATUS "Looking in IMPI_DLL_DIR=${IMPI_DLL_DIR}")
find_library(IMPI_LIB
  "impi.lib"
  HINTS "${IMPI_LIB_DIR}"
  PATH_SUFFIXES "${IMPI_BUILD}"
  DOC "Location of the Intel MPI impi.lib file"
  REQUIRED
  NO_DEFAULT_PATH)

find_file(IMPI_DLL
  "impi.dll"
  HINTS "${IMPI_DLL_DIR}"
  PATH_SUFFIXES "${IMPI_BUILD}"
  DOC "Location of the Intel MPI impi.dll file"
  REQUIRED
  NO_DEFAULT_PATH)

set( MPI_C_COMPILER ${CMAKE_C_COMPILER} CACHE FILEPATH "MPI C compiler" FORCE)
set( MPI_C_COMPILER_INCLUDE_DIRS "${MPI_ROOT}/include" CACHE FILEPATH "MPI C include dir")
set( MPI_C_LIBRARIES ${IMPI_LIB};${IMPI_DLL} CACHE FILEPATH "MPI C libs to link" )
set( MPI_C_LIB_PATHS "${MPI_ROOT}" CACHE FILEPATH "MPI C lib's paths to link" )

set( MPI_Fortran_COMPILER ${CMAKE_Fortran_COMPILER} CACHE FILEPATH "MPI Fortran compiler" FORCE)
set( MPI_Fortran_LIBRARIES ${IMPI_LIB};${IMPI_DLL} CACHE FILEPATH "MPI Fortran libs to link" FORCE)
set( MPI_Fortran_COMPILER_INCLUDE_DIRS "${MPI_ROOT}/include" CACHE FILEPATH "MPI Fortran include dir")
set( MPI_Fortran_MODULE_DIRS "${MPI_ROOT}/include/mpi" CACHE FILEPATH "MPI Fortran module dir")
set( MPI_Fortran_LIB_PATHS "${MPI_ROOT}" CACHE FILEPATH "MPI libraries paths for Fortran")

set( MPIEXEC_EXECUTABLE "${MPI_ROOT}/bin/mpiexec.exe" CACHE FILEPATH "Path to mpiexec")
set( MPI_impi_LIBRARY "${IMPI_LIB}" CACHE FILEPATH "MPI lib to link" )

set(MPI_Fortran_HAVE_F90_MODULE FALSE)
set(MPI_Fortran_HAVE_F08_MODULE FALSE)

#--------------------------------------------------------
# Make sure a simple "hello world" C mpi program compiles
#--------------------------------------------------------
set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS ${MPI_C_COMPILE_OPTIONS} ${MPI_C_LINK_FLAGS})
set(OLD_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_DEFINITIONS ${MPI_C_COMPILE_DEFINITIONS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_C_COMPILER_INCLUDE_DIRS})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
if(WIN32)
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_C_LIBRARIES})
else()
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_C_LIBRARIES})
endif()
set(OLD_LINK_DIRECTORIES ${CMAKE_REQUIRED_LINK_DIRECTORIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${MPI_C_LIB_PATHS})
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
  printf(\"Hello world from processor %s, rank %d out of %d processors\",
         processor_name, world_rank, world_size);
  MPI_Finalize();
}"
MPI_C_COMPILES)
set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_DEFINITIONS ${OLD_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${OLD_LINK_DIRECTORIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)
unset(OLD_LINK_LIBRARIES)
unset(OLD_LINK_DIRECTORIES)

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
set(CMAKE_REQUIRED_FLAGS "-ffree-form" ${MPI_Fortran_COMPILE_OPTIONS} ${MPI_Fortran_LINK_FLAGS})
set(OLD_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_DEFINITIONS ${MPI_Fortran_COMPILE_DEFINITIONS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_Fortran_COMPILER_INCLUDE_DIRS};${MPI_Fortran_MODULE_DIRS})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
if(WIN32)
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
else()
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
endif()
set(OLD_LINK_DIRECTORIES ${CMAKE_REQUIRED_LINK_DIRECTORIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${MPI_Fortran_LIB_PATHS})
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
set(CMAKE_REQUIRED_DEFINITIONS ${OLD_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${OLD_LINK_DIRECTORIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)
unset(OLD_LINK_LIBRARIES)
unset(OLD_LINK_DIRECTORIES)

#--------------------------------
# If that failed try using mpif.h
#--------------------------------
set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "-ffree-form" ${MPI_Fortran_COMPILE_OPTIONS} ${MPI_Fortran_LINK_FLAGS})
set(OLD_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_DEFINITIONS ${MPI_Fortran_COMPILE_DEFINITIONS})
set(OLD_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MPI_Fortran_COMPILER_INCLUDE_DIRS};${MPI_Fortran_MODULE_DIRS})
set(OLD_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
if(WIN32)
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
else()
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_Fortran_LIBRARIES})
endif()
set(OLD_LINK_DIRECTORIES ${CMAKE_REQUIRED_LINK_DIRECTORIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${MPI_Fortran_LIB_PATHS})
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
set(CMAKE_REQUIRED_DEFINITIONS ${OLD_REQUIRED_DEFINITIONS})
set(CMAKE_REQUIRED_INCLUDES ${OLD_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES ${OLD_LIBRARIES})
set(CMAKE_REQUIRED_LINK_DIRECTORIES ${OLD_LINK_DIRECTORIES})
unset(OLD_REQUIRED_FLAGS)
unset(OLD_INCLUDES)
unset(OLD_LIBRARIES)
unset(OLD_LINK_LIBRARIES)
unset(OLD_LINK_DIRECTORIES)

if ( (NOT MPI_Fortran_MODULE_COMPILES) AND (NOT MPI_Fortran_INCLUDE_COMPILES) )
  message ( WARNING "It appears that the Fortran MPI compiler is not working. "
    "For OpenCoarrays Aware compilers, this may be irrelavent: "
    "  The src/extensions/opencoarrays.F90 module will be disabled, but it is "
    "  possible that the build will succeed, despite this fishy circumstance."
    )
else()
  set(MPI_C_FOUND TRUE)
  set(MPI_Fortran_FOUND TRUE)
endif()
