if (HIGH_RESOLUTION_TIMER)
  add_definitions(-DHAVE_WALLTIME)
  set(walltime_o walltime.o)
endif()
caf_compile_executable(coarray_distributed_transpose
  coarray_distributed_transpose.F90
  ${walltime_o}
  )
