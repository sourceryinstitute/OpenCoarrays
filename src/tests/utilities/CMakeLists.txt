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

if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  set(CMAKE_REQUIRED_FLAGS ${OLD_REQUIRED_FLAGS})
endif()

add_library( oc_test_interfaces STATIC
  opencoarrays_object_interface.f90
  oc_assertions_interface.F90
  )
add_library( opencoarrays_test_utilities STATIC
  $<TARGET_OBJECTS:oc_test_interfaces>
  oc_assertions_implementation.F90
  )
target_link_libraries(opencoarrays_test_utilities PUBLIC oc_test_interfaces)
set_target_properties(oc_test_interfaces opencoarrays_test_utilities
  PROPERTIES
  Fortran_MODULE_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${mod_dir_tail}"
  INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${mod_dir_tail}"
  COMPILE_OPTIONS "-fcoarray=lib"
  )

if(HAVE_ERROR_STOP_IN_PURE)
  target_compile_definitions(oc_test_interfaces
    PUBLIC -DHAVE_ERROR_STOP_IN_PURE)
  target_compile_definitions(opencoarrays_test_utilities
    PUBLIC -DHAVE_ERROR_STOP_IN_PURE)
endif()
if(HAVE_VARIABLE_STOP_CODE)
  target_compile_definitions(oc_test_interfaces
    PUBLIC -DHAVE_VARIABLE_STOP_CODE)
  target_compile_definitions(opencoarrays_test_utilities
    PUBLIC -DHAVE_VARIABLE_STOP_CODE)
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
