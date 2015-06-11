set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod)

find_package(MPI REQUIRED)
add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c ../extensions/opencoarrays.F90)
target_include_directories(caf_mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_compile_options(caf_mpi INTERFACE -fcoarray=lib)

include_directories(${CMAKE_BINARY_DIR}/mod)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
)
install(DIRECTORY  ${CMAKE_BINARY_DIR}/mod DESTINATION .)


# This could be needed to produce shared libraries:
#target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES})

