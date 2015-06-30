#Specify all the subdirectories containing codes that produces executable files:
if (${opencoarrays_aware_compiler})
  set(directories_to_build coarrayHelloWorld dist_transpose pde_solvers)
endif()

foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()
