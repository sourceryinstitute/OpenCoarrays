set(directories_to_build single mpi tests)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()
