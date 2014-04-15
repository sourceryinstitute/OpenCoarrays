set(mofo_root ../../../../../)
set(examples_root ${mofo_root}/Examples/serial_examples/src/)
set(library_root  ${mofo_root}/utilities/src/library/)
add_executable(burgers_caf 
  main.F90 
  local_field.F90 
  global_field.F90 
  burgers.F90 
  ${examples_root}/chapter01/fireworks/kind_parameters.F90 
  ${examples_root}/chapter01/fireworks/math_constants.F90 
  ${library_root}/object_interface.F90
  ${library_root}/co_object_interface.F90
  ${library_root}/ForTrilinos_assertion_utility.F90
  ${library_root}/ForTrilinos_error.F90
)
set(include_directory ${mofo_root}/include-files)
if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "Cray")
  #configure_file(prm_cce.pbs prm_cce.pbs copyonly)
  configure_file(${include_directory}/cray_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_directory}/cray_capabilities.txt ${library_root}/compiler_capabilities.txt copyonly)
elseif ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "Intel")
  #configure_file(prm_ifort.pbs prm_ifort.pbs copyonly)
  configure_file(${include_directory}/intel_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_directory}/intel_capabilities.txt ${library_root}/compiler_capabilities.txt copyonly)
elseif ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
  configure_file(${include_directory}/gfortran_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_directory}/gfortran_capabilities.txt ${library_root}/compiler_capabilities.txt copyonly)
endif()
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt;${library_root}/compiler_capabilities.txt")

configure_file(coarrayBurgers_cce.pbs coarrayBurgers_cce.pbs copyonly)
configure_file(coarrayBurgers_ifort.pbs coarrayBurgers_intel.pbs copyonly)
