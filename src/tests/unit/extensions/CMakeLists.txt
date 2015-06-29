if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
  set(gfortran_compiler true)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.2.3) 
  # Detect Fortran compiler version directly
  if(gfortran_compiler AND (CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER 4.9.2))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
else()
  # Use the C compiler version as a proxy for the Fortran compiler version (won't work with NAG)
  if(gfortran_compiler AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.9.2))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
endif()

if("${gfortran_compiler}" AND (NOT "${opencoarrays_aware_compiler}"))
  # This applied to gfortran 4.9 and some earlier versions (TODO: find out which)
  add_definitions(-DCOMPILER_SUPPORTS_CAF_INTRINSICS)
endif()

set(source collectives_extensions.F90)
# collectives_tests.f90 is for testing the caf compiler wrapper so 
# the corresponding executable will be built as part of the process of
# running the test.  We therefore copy the uncompiled source to the 
# build tree:
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../collectives/collectives_tests.f90 
  ${CMAKE_CURRENT_BINARY_DIR}/${source}
  COPYONLY
)

# Now we write the script that compiles and runs the test

set(test_harness ${CMAKE_BINARY_DIR}/bin_staging/test-collectives-extensions.sh)
install(
    FILES "${test_harness}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
file(WRITE  "${test_harness}" "#!/bin/bash\n")
file(APPEND "${test_harness}" "cd ${CMAKE_CURRENT_BINARY_DIR}\n")
set(executable collectives_extensions)
if (opencoarrays_aware_compiler) 
  # Explicitly include the directory containing the .mod file because the caf script won't pass it 
  # automatically when the script wraps an OpenCoarrays-aware compiler
  file(APPEND 
    "${test_harness}" 
    "FC=mpif90 ${CMAKE_INSTALL_PREFIX}/bin/caf ${source} -o ${executable} -I${CMAKE_BINARY_DIR}/mod -DUSE_EXTENSIONS\n"
  )
else()
  file(APPEND "${test_harness}" "FC=mpif90 ${CMAKE_INSTALL_PREFIX}/bin/caf ${source} -o ${executable} -DUSE_EXTENSIONS\n")
endif()
set(num_images 4)
file(APPEND "${test_harness}" "${CMAKE_INSTALL_PREFIX}/bin/cafrun -np ${num_images} ./${executable}\n")
