IF (("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" ) AND (HIGH_RESOLUTION_TIMER))
  add_definitions(-DHAVE_WALLTIME)
  message("\nIn ${CMAKE_CURRENT_SOURCE_DIR}:")
  message("WARNING: walltime.o clicks once per clock cycle on an x86 CPU with a 3.6 GHz clock.")
  message("Because modern processors can throttle their clock spees, do not ")
  message("rely on this timer for an absolute value, but it can be useful for ")
  message("comparison or relative execution execution times on the same platform.\n")
  add_executable(coarray_distributed_transpose
    coarray_distributed_transpose.F90
    walltime.o
  )
else()
  message("\nIn ${CMAKE_CURRENT_SOURCE_DIR}: Substituting MPI_WTIME for WALLTIME.")
  add_executable(coarray_distributed_transpose
    coarray_distributed_transpose.F90
  )
endif()
target_link_libraries(coarray_distributed_transpose OpenCoarrays)
