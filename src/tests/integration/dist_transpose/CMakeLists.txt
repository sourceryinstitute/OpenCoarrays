IF (("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" ) AND (${HIGH_RESOLUTION_TIMER}))
  add_definitions(-DHAVE_WALLTIME)
  message("\n\nUsing assembly-language timers that tick once per clock cycle.")
  message("WARNING: walltime.o is designed for an x86 CPU with a 3.6 GHz clock.\n\n")
  message("Because modern processors can throttle their clock spees, do not \n\n")
  message("rely on this timer for an absolute value, but it can be useful for \n\n")
  message("comparison or relative execution execution times on the same platform.\n\n")
  add_executable(coarray_distributed_transpose
    coarray_distributed_transpose.F90
    walltime.o
  )
else()
  add_executable(coarray_distributed_transpose
    coarray_distributed_transpose.F90
  )
endif()
target_link_libraries(coarray_distributed_transpose OpenCoarrays)
