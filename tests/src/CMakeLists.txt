#Name project and specify source languages
project(fortran-caf-tests Fortran C)

# Some compilers will work with earlier versions of CMake. (The Cray Compiler Environment requires CMake 2.8.5)
cmake_minimum_required(VERSION 2.8.5)

#Print an error message on an attempt to build inside the source directory tree:
IF ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  MESSAGE(FATAL_ERROR "ERROR! "
    "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    " == CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "\nThis archive does not support in-source builds:\n"
    "You must now delete the CMakeCache.txt file and the CMakeFiles/ directory under"
    "the 'tests' source directory or you will not be able to configure correctly!"
    "\nYou must now run something like:\n"
    "  $ rm -r CMakeCache.txt CMakeFiles/"
    "\n"
    "Please create a different directory and configure the tests under that different directory such as\n"
    "  $ mkdir MY_BUILD\n"
    "  $ cd MY_BUILD\n"
    "  $ cmake [OPTIONS] .."
    )
ENDIF()

#Specify all the subdirectories containing codes that produces executable files:
set(directories_to_build pde_solvers simple)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()

#Report untested Fortran compiler unless explicitly directed to build all examples.
if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU|Intel|Cray" )
else()
  message("\n*** Untested Fortran compiler detected: ${CMAKE_Fortran_COMPILER_ID}.  ***\n")
  message("\n*** Attempting to build all examples. Please report any failures to sourcery@rouson.net***\n")
endif()

enable_testing()

add_test(coarrayBurgersDynamicMemory pde_solvers/coarrayBurgers/burgers_caf)
set_tests_properties(coarrayBurgersDynamicMemory PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(coarrayBurgersStaticMemoryTau pde_solvers/coarrayBurgersStaticTau/burgers_caf_tau)
set_tests_properties(coarrayBurgersStaticMemoryTau PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(coarrayHeatEquation pde_solvers/coarrayHeatSimplified/co_heat)
set_tests_properties(coarrayHeatEquation PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(increment_my_neighborTest simple/increment_my_neighbor)
set_tests_properties(increment_my_neighborTest PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(send_array_test simple/send_array_test)
set_tests_properties(send_array_test PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(syncimages simple/syncimages)
set_tests_properties(syncimages PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")

add_test(syncimages2 simple/syncimages2)
set_tests_properties(syncimages2 PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")
