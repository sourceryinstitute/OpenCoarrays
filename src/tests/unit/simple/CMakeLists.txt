# Fortran tests
add_executable(sync_images syncimages.f90)
target_link_libraries(sync_images OpenCoarrays)

add_executable(is_there_a_better_name syncimages2.f90)
target_link_libraries(is_there_a_better_name OpenCoarrays)

add_executable(increment_my_neighbor test1Caf.f90)
target_link_libraries(increment_my_neighbor OpenCoarrays)

add_executable(atomics testAtomics.f90)
target_link_libraries(atomics OpenCoarrays)

# C tests
#include(CMakeForceCompiler)
#CMAKE_FORCE_C_COMPILER(mpicc GNU)
#add_executable(C_sync_images syncimages2.c ../../../mpi/mpi_caf.c ../../../common/caf_auxiliary.c)
