cmake_minimum_required(VERSION 3.0)

#Name project and specify source languages
project(opencoarrays C)

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

add_definitions(-DPREFIX_NAME=_gfortran_caf_)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#Specify all the subdirectories containing codes that produces executable files:
set(directories_to_build single mpi)
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
  VERSION 0.1
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

#Report untested Fortran compiler unless explicitly directed to build all examples.
#if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
#else()
#  message("\n*** Untested Fortran compiler detected: ${CMAKE_Fortran_COMPILER_ID}.  ***\n")
#  message("\n*** Attempting to build all examples. Please report any failures to sourcery@rouson.net***\n")
#endif()

#enable_testing()

#add_test(coarrayBurgersDynamicMemory pde_solvers/coarrayBurgers/burgers_caf)
#set_tests_properties(coarrayBurgersDynamicMemory PROPERTIES PASS_REGULAR_EXPRESSION "Test passed.")
