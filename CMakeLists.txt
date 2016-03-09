cmake_minimum_required(VERSION 3.0)

# Set the type/configuration of build to perform
set ( CMAKE_CONFIGURATION_TYPES "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "CodeCoverage" )
set ( CMAKE_BUILD_TYPE "Release"
  CACHE STRING "Select which configuration to build." )
set_property ( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

#Name project and specify source languages
project(opencoarrays VERSION 1.3.8 LANGUAGES C Fortran)

#Print an error message on an attempt to build inside the source directory tree:
if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "ERROR! "
    "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    " == CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "\nThis archive does not support in-source builds:\n"
    "You must now delete the CMakeCache.txt file and the CMakeFiles/ directory under"
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
  unset(CMAKE_REQUIRED_FLAGS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(install_prerequisites)

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

function(add_mpi_test name num_mpi_proc path)
   set(test_parameters ${MPIEXEC_NUMPROC_FLAG} ${num_mpi_proc} )
   add_test(NAME ${name} COMMAND ${MPIEXEC} ${test_parameters} "${path}")
   set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endfunction(add_mpi_test)

set(tests_root ${CMAKE_CURRENT_BINARY_DIR}/src/tests)

if(opencoarrays_aware_compiler)
  # Unit tests targeting each libcaf_mpi function, argument, and branch of code
  add_mpi_test(initialize_mpi 2 ${tests_root}/unit/init_register/initialize_mpi)
  add_mpi_test(register 2 ${tests_root}/unit/init_register/register)
  add_mpi_test(register_rename_me 2 ${tests_root}/unit/init_register/register_rename_me)
  add_mpi_test(register_rename_me_too 2 ${tests_root}/unit/init_register/register_rename_me_too)
  add_mpi_test(allocate_as_barrier 2 ${tests_root}/unit/init_register/allocate_as_barrier)
  if (NOT ( APPLE AND ( DEFINED ENV{TRAVIS} ) ) )
    add_mpi_test(allocate_as_barrier_proc 2 ${tests_root}/unit/init_register/allocate_as_barrier_proc)
  endif ()
  add_mpi_test(get_array 2 ${tests_root}/unit/send-get/get_array)
  add_mpi_test(send_array 2 ${tests_root}/unit/send-get/send_array)
  add_mpi_test(get_with_offset_1d 2 ${tests_root}/unit/send-get/get_with_offset_1d)
  add_mpi_test(whole_get_array 2 ${tests_root}/unit/send-get/whole_get_array)
  add_mpi_test(strided_get 2 ${tests_root}/unit/send-get/strided_get)
  add_mpi_test(co_sum 4 ${tests_root}/unit/collectives/co_sum_test)
  add_mpi_test(co_broadcast 4 ${tests_root}/unit/collectives/co_broadcast_test)
  add_mpi_test(co_min 4 ${tests_root}/unit/collectives/co_min_test)
  add_mpi_test(co_max 4 ${tests_root}/unit/collectives/co_max_test)
  add_mpi_test(syncall 32 ${tests_root}/unit/sync/syncall)
  add_mpi_test(syncimages 32 ${tests_root}/unit/sync/syncimages)
  add_mpi_test(duplicate_syncimages 8 ${tests_root}/unit/sync/duplicate_syncimages)
  add_mpi_test(co_reduce 4 ${tests_root}/unit/collectives/co_reduce_test)
#  add_mpi_test(syncimages_status 32 ${tests_root}/unit/sync/syncimages_status)

  # Integration tests verifying the use of libcaf_mpi in applications
  add_mpi_test(hello_multiverse 2 ${tests_root}/integration/coarrayHelloWorld/hello_multiverse)
  add_mpi_test(coarray_burgers_pde 2 ${tests_root}/integration/pde_solvers/coarrayBurgers/coarray_burgers_pde)
  add_mpi_test(co_heat 2 ${tests_root}/integration/pde_solvers/coarrayHeatSimplified/co_heat)
  if ( ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64") AND ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") )
    if ( NOT (DEFINED ENV{TRAVIS}))
      add_mpi_test(coarray_navier_stokes 2 ${tests_root}/integration/pde_solvers/navier-stokes/coarray_navier_stokes)
      set_property(TEST coarray_navier_stokes PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
    endif()
  endif()
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
