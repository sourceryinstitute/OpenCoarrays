if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  set(OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "-fcoarray=single -ffree-form")
endif()

# Check support for Fortran 2018 error termination in a pure procedure
check_fortran_source_compiles("
  program main
  contains
    pure subroutine foo()
      error stop
    end subroutine
  end program
"
  HAVE_ERROR_STOP_IN_PURE
  SRC_EXT ".f90"
  )
if(HAVE_ERROR_STOP_IN_PURE)
  add_definitions(-DHAVE_ERROR_STOP_IN_PURE)
endif()

# Check support for Fortran 2018 variable stop code
check_fortran_source_compiles("
  program main
    integer i
    i = 0
    error stop i
  end program
"
  HAVE_VARIABLE_STOP_CODE
  SRC_EXT ".f90")
if(HAVE_VARIABLE_STOP_CODE)
  add_definitions(-DHAVE_VARIABLE_STOP_CODE)
endif()

if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
endif()

#Toggle C preprocessor macro for turning assertions on or off
if(NO_ASSERTIONS)
  target_compile_definitions( opencoarrays_test_utilities
    PUBLIC "-DUSE_ASSERTIONS=.false."
  )
else()
  target_compile_definitions( opencoarrays_test_utilities
    PUBLIC "-DUSE_ASSERTIONS=.true."
  )
endif()

add_library( opencoarrays_test_utilities
  object_interface.f90
  assertions_interface.F90
  assertions_implementation.F90
)
