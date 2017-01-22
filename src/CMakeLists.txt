set(directories_to_build mpi tests)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()

install( FILES libcaf.h libcaf-gfortran-descriptor.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
