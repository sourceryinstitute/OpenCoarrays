#if (${LEGACY_ARCHITECTURE})
   set(fft_library fft_sse)
#else()
#  set(fft_library fft_avx)
#endif()

add_executable( coarray_navier_stokes
  coarray-shear_coll.f90
  walltime.o
)
target_link_libraries( coarray_navier_stokes OpenCoarrays ${CMAKE_CURRENT_SOURCE_DIR}/lib${fft_library}.a )

add_executable( mpi_navier_stokes 
  mpi-shear.f90 
  walltime.o
)
target_link_libraries( mpi_navier_stokes OpenCoarrays ${CMAKE_CURRENT_SOURCE_DIR}/lib${fft_library}.a )
