set(directories_to_build mpi tests)
foreach(directory ${directories_to_build})
  add_subdirectory(${directory})
endforeach()

set(N_CPU ${N_CPU} PARENT_SCOPE)
set(HOSTNAME ${HOSTNAME} PARENT_SCOPE)

if(openmpi)
  set(openmpi ${openmpi} PARENT_SCOPE)
endif()

install( FILES libcaf.h libcaf-gfortran-descriptor.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
