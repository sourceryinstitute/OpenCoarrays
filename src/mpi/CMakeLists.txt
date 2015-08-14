set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod)

find_package(MPI REQUIRED)

if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
  set(gfortran_compiler true)
elseif("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "Cray")
  set(cray_compiler true)
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.3.1) 
  # Detect Fortran compiler version directly
  if(gfortran_compiler AND (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 5))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
else()
  # Use the C compiler version as a proxy for the Fortran compiler version (won't work with NAG)
  if(gfortran_compiler AND (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
endif()

if(opencoarrays_aware_compiler)
else()
  if(gfortran_compiler)
    # This applied to gfortran 4.9 and some earlier versions (FIX ME: find out which)
    add_definitions(-DCOMPILER_SUPPORTS_CAF_INTRINSICS)
  elseif (cray_compiler)
    # This applies to CCE 8.4 beta or earlier
    add_definitions(-DCOMPILER_LACKS_C_PTRDIFF_T)
    add_definitions(-DCOMPILER_LACKS_C_SIZEOF_ASSUMED_RANK)
  endif()
endif()
add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c ../extensions/opencoarrays.F90)

target_include_directories(caf_mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_compile_options(caf_mpi INTERFACE -fcoarray=lib)

include_directories(${CMAKE_BINARY_DIR}/mod)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
)
install(DIRECTORY  ${CMAKE_BINARY_DIR}/mod DESTINATION .)

set(compiler_wrapper ${CMAKE_BINARY_DIR}/exe/caf)
set(exe_dir ${CMAKE_BINARY_DIR}/bin_staging)
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
