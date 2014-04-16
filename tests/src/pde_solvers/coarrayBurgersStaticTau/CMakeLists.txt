set(include_path  ${CMAKE_CURRENT_SOURCE_DIR}/../include-files/)
set(library_path  ${CMAKE_CURRENT_SOURCE_DIR}/../library/)
add_executable(burgers_caf_tau 
  main.F90 
  field.F90 
  periodic_2nd_order.F90 
  kind_parameters.F90 
  math_constants.F90 
  ${library_path}/co_object_interface.F90 
  ${library_path}/object_interface.F90 
  ${library_path}/ForTrilinos_assertion_utility.F90 
  ${library_path}/ForTrilinos_error.F90
)

if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "Cray")
  configure_file(${include_path}/cray_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_path}/cray_capabilities.txt ${library_path}/compiler_capabilities.txt copyonly)
elseif ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "Intel")
  configure_file(${include_path}/intel_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_path}/intel_capabilities.txt ${library_path}/compiler_capabilities.txt copyonly)
elseif ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
  configure_file(${include_path}/gfortran_capabilities.txt ${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt copyonly)
  configure_file(${include_path}/gfortran_capabilities.txt ${library_path}/compiler_capabilities.txt copyonly)
endif()
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/compiler_capabilities.txt;${library_path}/compiler_capabilities.txt")
