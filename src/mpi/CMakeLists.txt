set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod)

find_package(MPI REQUIRED)

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

add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c ../extensions/opencoarrays.F90)

target_include_directories(caf_mpi PRIVATE ${MPI_C_INCLUDE_PATH})
if (gfortran_compiler)
  target_compile_options(caf_mpi INTERFACE -fcoarray=lib)
endif()

include_directories(${CMAKE_BINARY_DIR}/mod)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
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

# This could be needed to produce shared libraries:
#target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES})

#set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${exe_dir}/cafrun;${exe_dir}/caf;${exe_dir}/test-caf-tally.sh")
