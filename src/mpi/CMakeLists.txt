set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod)

if (NOT MPI_C_FOUND)
  find_package(MPI REQUIRED)

  set(CMAKE_C_COMPILE_FLAGS ${CMAKE_C_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS})
  set(CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS} ${MPI_C_LINK_FLAGS})
  set(CMAKE_Fortran_COMPILE_FLAGS ${CMAKE_Fortran_COMPILE_FLAGS} ${MPI_Fortran_COMPILE_FLAGS})
  set(CMAKE_Fortran_LINK_FLAGS ${CMAKE_Fortran_LINK_FLAGS} ${MPI_Fortran_LINK_FLAGS})
  include_directories(BEFORE ${MPI_C_INCLUDE_PATH} ${MPI_Fortran_INCLUDE_PATH})
endif()

if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
  set(gfortran_compiler true)
elseif("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "Cray")
  set(cray_compiler true)
elseif("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "PGI")
  set(portland_group_compiler true)
endif()

if(gfortran_compiler AND (NOT opencoarrays_aware_compiler))
  # This applied to gfortran 4.9 and some earlier versions (FIX ME: find out which)
  add_definitions(-DCOMPILER_SUPPORTS_CAF_INTRINSICS)
endif()

option(CAF_EXPOSE_INIT_FINALIZE "Expose caf_init and caf_finalize in opencoarrays module" FALSE)
if(CAF_EXPOSE_INIT_FINALIZE)
  add_definitions(-DEXPOSE_INIT_FINALIZE)
endif()

# Determine whether and how to include OpenCoarrays module based on if the Fortran MPI compiler:
#   - workds
#   - is compatible with the fortran compiler used to build the MPI implementation
if (MPI_Fortran_MODULE_COMPILES)
  # likely the same compiler compiled MPI
  set(MPI_CAF_FORTRAN_FILES ../extensions/opencoarrays.F90)
endif()

add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c ${MPI_CAF_FORTRAN_FILES})
target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES} ${MPI_Fortran_LIBRARIES})

set_target_properties ( caf_mpi
  PROPERTIES
  SOVERSION ${PROJECT_VERSION.MAJOR}.${PROJECT_VERSION.MINOR}
  VERSION ${PROJECT_VERSION}
  )


if (gfortran_compiler)
  target_compile_options(caf_mpi INTERFACE -fcoarray=lib)
endif()

include_directories(${CMAKE_BINARY_DIR}/mod)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
install(DIRECTORY  ${CMAKE_BINARY_DIR}/mod DESTINATION .)

# Now we write the script that passes CAF source to the compiler with the necessary arguments
# and, if necessary, performs some code transformations prior to invoking the compiler.
set(exe_dir ${CMAKE_BINARY_DIR}/bin_staging)
set(compiler_wrapper ${exe_dir}/caf)
install(
    FILES "${compiler_wrapper}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/caf-head CAF_HEADER)
file(WRITE  "${compiler_wrapper}" "${CAF_HEADER}\n")
file(APPEND "${compiler_wrapper}"  "caf_mod_dir=${CMAKE_INSTALL_PREFIX}/mod\n")
file(APPEND "${compiler_wrapper}"  "caf_lib_dir=${CMAKE_INSTALL_PREFIX}/lib\n")
file(APPEND "${compiler_wrapper}"  "caf_version=${PROJECT_VERSION}\n")
if(gfortran_compiler)
  file(APPEND "${compiler_wrapper}"  "link_args='-fcoarray=lib -lcaf_mpi'\n")
elseif(portland_group_compiler)
  file(APPEND "${compiler_wrapper}"  "link_args=-lcaf_mpi\n")
endif()
if(opencoarrays_aware_compiler)
  file(APPEND "${compiler_wrapper}"  "caf_compiler=true\n")
else()
  file(APPEND "${compiler_wrapper}"  "caf_compiler=false\n")
endif()

if ( DEFINED ENV{FC} )
  file(APPEND "${compiler_wrapper}"  "CAFC=$ENV{FC}\n")
endif ()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  file(APPEND "${compiler_wrapper}"  "linux=true\n")
endif()
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/caf-foot FOOTER)
file(APPEND "${compiler_wrapper}" "${FOOTER}")

# Now we write the script that launches executable files produced from CAF codes
set(caf_launcher ${exe_dir}/cafrun)
install(
    FILES "${caf_launcher}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/cafrun-head CAFRUN_HEADER)
file(WRITE  "${caf_launcher}" "${CAFRUN_HEADER}\n")
file(APPEND "${caf_launcher}"  "caf_version=${PROJECT_VERSION}\n")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/cafrun-foot FOOTER)
file(APPEND "${caf_launcher}" "${FOOTER}")

#set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${exe_dir}/cafrun;${exe_dir}/caf;${exe_dir}/test-caf-tally.sh")
