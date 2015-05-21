add_executable(coarray_distributed_transpose
  coarray_distributed_transpose.F90
  walltime.o
  walltime.x64
)
target_link_libraries(coarray_distributed_transpose OpenCoarrays)
