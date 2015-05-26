#if (${LEGACY_ARCHITECTURE})
   set(fft_library fft_sse)
#else()
#  set(fft_library fft_avx)
#endif()

IF (("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" ) AND (${HIGH_RESOLUTION_TIMER}))
  add_definitions(-DHAVE_WALLTIME)

  message("\n\nUsing assembly-language timers that tick once per clock cycle.")
  message("WARNING: walltime.o is designed for an x86 CPU with a 3.6 GHz clock.\n\n")
  message("Because modern processors can throttle their clock spees, do not \n\n")
  message("rely on this timer for an absolute value, but it can be useful for \n\n")
  message("comparison or relative execution execution times on the same platform.\n\n")

  add_executable( mpi_navier_stokes 
    mpi-shear.F90 
    walltime.o
  )
  add_executable( coarray_navier_stokes
    coarray-shear_coll.F90
    walltime.o
  )
  target_link_libraries( coarray_navier_stokes OpenCoarrays ${CMAKE_CURRENT_SOURCE_DIR}/lib${fft_library}.a )
else()
  message("\nSkipping Navier-Stokes test on non-Linux/x86 platform.")
  message("Substitute an appropriate FFT library if it desired to build the Navier-Stokes solver.\n")
# add_executable( coarray_navier_stokes
#   coarray-shear_coll.F90
# )
endif()
