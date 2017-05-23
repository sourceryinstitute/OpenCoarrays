# For tests that cause a compile-time error instead of a runtime error, use this
# macro to delay the building of the test until the testing phase (ctest)
macro(add_compile_time_test name path base ext)

  set(compiler ${CMAKE_Fortran_COMPILER_ID})

  if (CMAKE_VERSION VERSION_GREATER 3.2.3) 
    # Detect Fortran compiler version directly
    set(compiler_version "${CMAKE_Fortran_COMPILER_VERSION}")
  else()
    # Use the C compiler version as a proxy for the Fortran compiler version (won't work with NAG)
    set(compiler_version "${CMAKE_C_COMPILER_VERSION}")
  endif()

  set(executable base)

  # Copy the source to the binary tree 
  configure_file(
    ${CMAKE_SOURCE_DIR}/${path}/${base}.${ext} 
    ${CMAKE_BINARY_DIR}/${path}/${base}.${ext} 
    COPYONLY
  )
  # Write a script to compile the code at test time instead of at build time so 
  # CMake doesn't bail during the build process because of the compilation failure
  set(bug_harness "${CMAKE_BINARY_DIR}/staging/test-${base}.sh")
  install(
      FILES "${bug_harness}"
      PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
      DESTINATION ${CMAKE_BINARY_DIR}/${path}
  )
  file(WRITE  "${bug_harness}" "#!/bin/bash\n")
  file(APPEND "${bug_harness}" "cd ${CMAKE_BINARY_DIR}/${path}\n")
  file(APPEND "${bug_harness}" 
              "source ${CMAKE_BINARY_DIR}/bin_staging/caf ${base}.${ext} -o ${base} \n")
  add_test(NAME ${name} COMMAND "${path}/test-${base}.sh")
  set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endmacro(add_compile_time_test)
