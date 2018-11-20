# Check support Fortran 2018 error termination in a pure procedure
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

# Check support Fortran 2018 variable stop code
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

#Toggle C preprocessor macro for turning assertions on or off
if(NO_ASSERTIONS)
  set_source_files_properties(assertions_interface.f90 PROPERTIES COMPILE_FLAGS "-cpp -DUSE_ASSERTIONS=.false.")
else()
  set_source_files_properties(assertions_interface.f90 PROPERTIES COMPILE_FLAGS "-cpp -DUSE_ASSERTIONS=.true.")
endif()

add_library( OpenCoarraysTestUtilities
  object_interface.f90
  assertions_interface.F90
  assertions_implementation.F90
)

