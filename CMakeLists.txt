cmake_minimum_required(VERSION 3.0)

#Name project and specify source languages
project(opencoarrays C Fortran)

#Print an error message on an attempt to build inside the source directory tree:
IF ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  MESSAGE(FATAL_ERROR "ERROR! "
    "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    " == CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "\nThis archive does not support in-source builds:\n"
    "You must now delete the CMakeCache.txt file and the CMakeFiles/ directory under"
    "the 'src' source directory or you will not be able to configure correctly!"
    "\nYou must now run something like:\n"
    "  $ rm -r CMakeCache.txt CMakeFiles/"
    "\n"
    "Please create a different directory and configure the tests under that different directory such as\n"
    "  $ mkdir MY_BUILD\n"
    "  $ cd MY_BUILD\n"
    "  $ cmake [OPTIONS] .."
    )
ENDIF()

#Report untested Fortran compiler unless explicitly directed to build all examples.
if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  add_definitions(-DPREFIX_NAME=_gfortran_caf_)
else()
  message("\n*** Attempting to build with untested Fortran compiler: ${CMAKE_Fortran_COMPILER_ID}.  ***\n")
  message("\n*** Please report any failures to damian@sourceryinstitute.org    ***\n")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
#set(directories_to_build src/single src/mpi src/tests)
#foreach(directory ${directories_to_build})
#  add_subdirectory(${directory})
#endforeach()

install(EXPORT OpenCoarraysTargets 
  NAMESPACE
    OpenCoarrays::
  DESTINATION
    lib/cmake/opencoarrays
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/OpenCoarraysConfigVersion.cmake"
  VERSION 1.0
  COMPATIBILITY AnyNewerVersion
)
configure_file(OpenCoarraysConfig.cmake.in CMakeFiles/OpenCoarraysConfig.cmake @ONLY)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/OpenCoarraysConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/OpenCoarraysConfigVersion.cmake"
  DESTINATION
    lib/cmake/opencoarrays
)

add_library(OpenCoarrays INTERFACE)
target_compile_options(OpenCoarrays INTERFACE -fcoarray=lib)
target_link_libraries(OpenCoarrays INTERFACE caf_mpi)

enable_testing()

function(add_mpi_test name num_mpi_proc path)
   set(test_parameters ${MPIEXEC_NUMPROC_FLAG} ${num_mpi_proc} )
   add_test(NAME ${name} COMMAND ${MPIEXEC} ${test_parameters} "${path}")
   set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endfunction(add_mpi_test)

set(tests_root ${CMAKE_CURRENT_BINARY_DIR}/src/tests)

# Unit tests targeting each libcaf_mpi function, argument, and branch of code
add_mpi_test(initialize_mpi 2 ${tests_root}/unit/init_register/initialize_mpi)
add_mpi_test(register 2 ${tests_root}/unit/init_register/register)
add_mpi_test(register_rename_me 2 ${tests_root}/unit/init_register/register_rename_me)
add_mpi_test(register_rename_me_too 2 ${tests_root}/unit/init_register/register_rename_me_too)

# Integration tests verifying the use of libcaf_mpi in applications
add_mpi_test(hello_multiverse 2 ${tests_root}/integration/coarrayHelloWorld/hello_multiverse)
add_mpi_test(coarray_burgers_pde 2 ${tests_root}/integration/pde_solvers/coarrayBurgers/coarray_burgers_pde)
add_mpi_test(co_heat 2 ${tests_root}/integration/pde_solvers/coarrayHeatSimplified/co_heat)

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" )
  add_mpi_test(coarray_navier_stokes 2 ${tests_root}/integration/pde_solvers/navier-stokes/coarray_navier_stokes)
  set_property(TEST coarray_navier_stokes PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endif()
