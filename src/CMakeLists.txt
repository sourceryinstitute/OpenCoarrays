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
else()
  message("\n*** Attempting to build with untested Fortran compiler: ${CMAKE_Fortran_COMPILER_ID}.  ***\n")
  message("\n*** Please report any failures to damian@sourceryinstitute.org    ***\n")
endif()

add_definitions(-DPREFIX_NAME=_gfortran_caf_)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(directories_to_build single mpi tests)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()

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

add_test(hello_world tests/integration/coarrayHelloWorld/hello_multiverse)
set_tests_properties(hello_world PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(distributed_transpose tests/integration/dist_transpose/coarray_distributed_transpose)
set_tests_properties(distributed_transpose PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(burgers_pde tests/integration/pde_solvers/coarrayBurgers/coarray_burgers_pde)
set_tests_properties(burgers_pde PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(heat_equation tests/integration/pde_solvers/coarrayHeatSimplified/co_heat)
set_tests_properties(heat_equation PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(navier-stokes tests/integration/pde_solvers/navier-stokes/coarray_navier_stokes)
set_tests_properties(navier-stokes PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(sync_images tests/unit/simple/sync_images)
set_tests_properties(sync_images PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(is_there_a_better_name tests/unit/simple/is_there_a_better_name)
set_tests_properties(is_there_a_better_name PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(increment_my_neighbor tests/unit/simple/increment_my_neighbor)
set_tests_properties(increment_my_neighbor PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(co_broadcast tests/unit/simple/co_broadcast)
set_tests_properties(co_broadcast PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(atomics tests/unit/simple/atomics)
set_tests_properties(atomics PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

# C tests
add_test(C_hello_world tests/unit/simple/C_hello_world)
set_tests_properties(C_hello_world PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(C_sync_images tests/unit/simple/C_sync_images)
set_tests_properties(C_sync_images PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")
