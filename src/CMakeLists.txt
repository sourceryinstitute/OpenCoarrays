add_subdirectory(application-binary-interface)
add_subdirectory(runtime-libraries)

if (BUILD_TESTING)
  add_subdirectory(tests)
endif()

set(N_CPU ${N_CPU} PARENT_SCOPE)
set(HOST_NAME ${HOST_NAME} PARENT_SCOPE)

if(openmpi)
  set(openmpi ${openmpi} PARENT_SCOPE)
endif()
