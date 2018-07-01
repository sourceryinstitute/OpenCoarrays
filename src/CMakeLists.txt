set(directories_to_build mpi tests)

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" )
  LIST( APPEND directories_to_build iso-fortran-binding )
endif()

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
