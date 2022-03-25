add_subdirectory(mpi)

set(N_CPU ${N_CPU} PARENT_SCOPE)
set(HOST_NAME ${HOST_NAME} PARENT_SCOPE)

if(openmpi)
  set(openmpi ${openmpi} PARENT_SCOPE)
endif()

install( FILES libcaf.h libcaf-gfortran-descriptor.h libcaf-version-def.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
