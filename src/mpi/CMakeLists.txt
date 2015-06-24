set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod)

find_package(MPI REQUIRED)

if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
  set(gfortran_compiler true)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.2.3) 
  # Detect Fortran compiler version directly
  if(gfortran_compiler AND (CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER 5.0.0))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
else()
  # Use the C compiler version as a proxy for the Fortran compiler version (won't work with NAG)
  if(gfortran_compiler AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 5.0.0))
    set(opencoarrays_aware_compiler true)
  else()
    set(opencoarrays_aware_compiler false)
  endif()
endif()

if(opencoarrays_aware_compiler)
  add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c)
else()
  if(gfortran_compiler)
    # This applied to gfortran 4.9 and some earlier versions (FIX ME: find out which)
    add_definitions(-DCOMPILER_SUPPORTS_CAF_INTRINSICS)
  endif()
  add_library(caf_mpi mpi_caf.c ../common/caf_auxiliary.c ../extensions/opencoarrays.F90)
endif()

target_include_directories(caf_mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_compile_options(caf_mpi INTERFACE -fcoarray=lib)

include_directories(${CMAKE_BINARY_DIR}/mod)

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
)
install(DIRECTORY  ${CMAKE_BINARY_DIR}/mod DESTINATION .)

set(compiler_wrapper ${CMAKE_BINARY_DIR}/bin/caf)
install(
    FILES "${compiler_wrapper}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/caf-head HEADER)
file(WRITE  "${compiler_wrapper}" "${HEADER}\n")
file(APPEND "${compiler_wrapper}"  "caf_mod_dir=${CMAKE_INSTALL_PREFIX}/mod\n")
file(APPEND "${compiler_wrapper}"  "caf_lib_dir=${CMAKE_INSTALL_PREFIX}/lib\n")
if(opencoarrays_aware_compiler)
  file(APPEND "${compiler_wrapper}"  "caf_compiler=true\n")
else()
  file(APPEND "${compiler_wrapper}"  "caf_compiler=false\n")
endif()
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/caf-foot FOOTER)
file(APPEND "${compiler_wrapper}" "${FOOTER}")

# This could be needed to produce shared libraries:
#target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES})
