#Specify all the subdirectories containing codes that produces executable files:
set(directories_to_build coarrayHelloWorld dist_transpose pde_solvers)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()
