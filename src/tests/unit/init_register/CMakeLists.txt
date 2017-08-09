add_executable(initialize_mpi initialize_mpi.F90)
target_link_libraries(initialize_mpi OpenCoarrays)

add_executable(register register.f90)
target_link_libraries(register OpenCoarrays)

add_executable(register_vector register_vector.f90)
target_link_libraries(register_vector OpenCoarrays)

add_executable(register_alloc_vector register_alloc_vector.f90)
target_link_libraries(register_alloc_vector OpenCoarrays)

add_executable(allocate_as_barrier allocate_as_barrier.f90)
target_link_libraries(allocate_as_barrier OpenCoarrays)

add_executable(allocate_as_barrier_proc allocate_as_barrier_proc.f90)
target_link_libraries(allocate_as_barrier_proc OpenCoarrays)

if (gfortran_compiler AND (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 7))
  add_executable(async_comp_alloc async_comp_alloc.f90)
  target_link_libraries(async_comp_alloc OpenCoarrays)

  add_executable(async_comp_alloc_2 async_comp_alloc_2.f90)
  target_link_libraries(async_comp_alloc_2 OpenCoarrays)

  add_executable(register_alloc_comp_1 register_alloc_comp_1.f90)
  target_link_libraries(register_alloc_comp_1 OpenCoarrays)

  add_executable(register_alloc_comp_2 register_alloc_comp_2.f90)
  target_link_libraries(register_alloc_comp_2 OpenCoarrays)

  add_executable(register_alloc_comp_3 register_alloc_comp_3.f90)
  target_link_libraries(register_alloc_comp_3 OpenCoarrays)

  add_executable(comp_allocated_1 comp_allocated_1.f90)
  target_link_libraries(comp_allocated_1 OpenCoarrays)

  add_executable(comp_allocated_2 comp_allocated_2.f90)
  target_link_libraries(comp_allocated_2 OpenCoarrays)
endif()
