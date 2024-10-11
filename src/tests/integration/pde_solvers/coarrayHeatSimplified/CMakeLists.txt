set(CMAKE_Fortran_COMPILER "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/caf")
add_library(local_field STATIC local_field.f90)
add_library(global_field STATIC global_field.f90)
add_executable(co_heat
  main.f90
)
target_link_libraries(global_field PUBLIC local_field caf_mpi_static)
target_link_libraries(co_heat PRIVATE global_field)
set_target_properties(local_field global_field co_heat
  PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}"
  Fortran_MODULE_DIRECTORY "${CMAKE_BINARY_DIR}"
)
