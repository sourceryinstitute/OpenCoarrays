set(directories_to_build mpi tests)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()
