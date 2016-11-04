# Fortran tests

add_executable(increment_my_neighbor test1Caf.f90)
target_link_libraries(increment_my_neighbor OpenCoarrays)

add_executable(atomics testAtomics.f90)
target_link_libraries(atomics OpenCoarrays)

# C tests
#include(CMakeForceCompiler)
#CMAKE_FORCE_C_COMPILER(mpicc GNU)
#add_executable(C_sync_images syncimages2.c ../../../mpi/mpi_caf.c ../../../common/caf_auxiliary.c)
