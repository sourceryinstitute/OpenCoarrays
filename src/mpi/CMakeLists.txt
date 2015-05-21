message("\n\n\n")
message("************************ FIX ME *******************************")
message("* Building against a version of mpi_caf.c after the following commit fails ")
message("* on undefined references to locking_atomic_op in function mutex_lock: ")
message("*")
message("* commit 3b716a9ef7206a5002aa1d6f8a7c8d6b768a437b")
message("* Author: Alessandro Fanfarillo <fanfarillo@ing.uniroma2.it>")
message("* Date:   Tue Apr 21 11:47:16 2015 +0200")
message("*")
message("* Locks implementation (it needs cleanup)")
message("*")
message("* Signed-off-by: Alessandro Fanfarillo <fanfarillo@ing.uniroma2.it>")
message("*")
message("* Revert mpi_caf.c to the above commit to build with CMake.    ")
message("***************************************************************")
message("\n\n\n")

find_package(MPI REQUIRED)
add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c)
target_include_directories(caf_mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_compile_options(caf_mpi INTERFACE -fcoarray=lib)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
)

# This could be needed to produce shared libraries:
#target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES})

