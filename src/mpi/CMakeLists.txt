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

include(CheckIncludeFile)
CHECK_INCLUDE_FILE("alloca.h" HAVE_ALLOCA)
if(NOT HAVE_ALLOCA)
  add_definitions(-DALLOCA_MISSING)
  message(WARNING "Could not find <alloca.h>. Assuming functionality is provided elsewhere.")
endif()

#----------------------------------------------------------------------
# Test if MPI implementation provides features needed for failed images
#----------------------------------------------------------------------
set(MPI_HAS_FAULT_TOL_EXT YES)

CHECK_INCLUDE_FILE("signal.h" HAVE_SIGNAL_H)
if (NOT HAVE_SIGNAL_H)
  set(MPI_HAS_FAULT_TOL_EXT NO)
  message( FATAL_ERROR "Currently, OpenCoarrays cannot build without signal.h")
endif()

include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(SIGKILL "signal.h" HAVE_SIGKILL)
if(NOT HAVE_SIGKILL) # try -D_POSIX, needed for mingw-w64, maybe others, see #435
                     # https://github.com/sourceryinstitute/OpenCoarrays/issues/435#issuecomment-323592433
  list( APPEND CMAKE_REQUIRED_DEFINITIONS -D_POSIX)
  CHECK_SYMBOL_EXISTS(SIGKILL "signal.h" HAVE_SIGKILL)
  if(HAVE_SIGKILL)
    add_definitions(-D_POSIX)
  endif()
endif()

if (NOT HAVE_SIGKILL)
  set(MPI_HAS_FAULT_TOL_EXT NO)
  message (FATAL_ERROR "Currently, OpenCoarrays cannot build without SIGKILL from signal.h")
endif()

set(NEEDED_SYMBOLS MPIX_ERR_PROC_FAILED;MPIX_ERR_REVOKED;MPIX_Comm_failure_ack;MPIX_Comm_failure_get_acked;MPIX_Comm_shrink;MPIX_Comm_agree)

set(old_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
if(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES};${MPI_C_INCLUDE_PATH})
else()
  set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_PATH})
endif()
set(old_cmake_required_flags "${CMAKE_REQUIRED_FLAGS}")
if(CMAKE_REQUIRED_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS};${MPI_C_COMPILE_FLAGS};${MPI_C_LINK_FLAGS})
else()
  set(CMAKE_REQUIRED_FLAGS ${MPI_C_COMPILE_FLAGS};${MPI_C_LINK_FLAGS})
endif()
set(old_cmake_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
if(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES};${MPI_C_LIBRARIES})
else()
  set(CMAKE_REQUIRED_LIBRARIES ${MPI_C_LIBRARIES})
endif()

set(MPI_HEADERS mpi.h)
CHECK_INCLUDE_FILE("mpi-ext.h" HAVE_MPI_EXT)
if(HAVE_MPI_EXT)
  add_definitions(-DHAVE_MPI_EXT_H)
  set(MPI_HEADERS ${MPI_HEADERS};mpi-ext.h)
endif()

foreach(symbol ${NEEDED_SYMBOLS})
  CHECK_SYMBOL_EXISTS(${symbol} ${MPI_HEADERS} HAVE_${symbol})
  if(NOT HAVE_${symbol})
    message( STATUS "\${HAVE_${symbol}} = ${HAVE_${symbol}}")
    message( WARNING "Disabling Failed Image support due to lack of support in the current MPI implementation.")
    set(MPI_HAS_FAULT_TOL_EXT NO)
    break() # no need to keep looking
  endif()
endforeach(symbol)
set(CMAKE_REQUIRED_INCLUDES ${old_cmake_required_includes})
set(CMAKE_REQUIRED_FLAGS ${old_cmake_required_flags})
set(CMAKE_REQUIRED_LIBRARIES ${old_cmake_required_libraries})

if(MPI_HAS_FAULT_TOL_EXT)
  option(CAF_ENABLE_FAILED_IMAGES "Enable failed images support" TRUE)
else()
  set(CAF_ENABLE_FAILED_IMAGES FALSE CACHE BOOL "Enable failed images support" FORCE)
endif()

if(CAF_ENABLE_FAILED_IMAGES)
  add_definitions(-DUSE_FAILED_IMAGES)
endif()

#---------------------------------------------------
# Windows Intel MPI compatibility, see GH issue #435
#---------------------------------------------------
CHECK_SYMBOL_EXISTS(I_MPI_VERSION mpi.h HAVE_Intel_MPI)
if(HAVE_Intel_MPI AND WIN32)
  add_definitions(-DUSE_GCC)
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
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# Install modules to standard include dir, but namespace them with compiler/version
set (mod_install "OpenCoarrays/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_COMPILER_VERSION}")
install(DIRECTORY  "${CMAKE_BINARY_DIR}/mod/"
  DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${mod_install}"
  FILES_MATCHING PATTERN "*.mod"
)

# Now add a link in standard include dir so that compilers will find by default... this may or may not actually be a good idea...
if ( "${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
  INSTALL(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${mod_install}/opencoarrays.mod \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_INCLUDEDIR}/opencoarrays.mod )"
  )
endif ()

# Now we write the script that passes CAF source to the compiler with the necessary arguments
# and, if necessary, performs some code transformations prior to invoking the compiler.
set(exe_dir ${CMAKE_BINARY_DIR}/bin_staging)
set(compiler_wrapper ${exe_dir}/caf)
install(
    FILES "${compiler_wrapper}"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/caf-head CAF_HEADER)
file(WRITE  "${compiler_wrapper}" "${CAF_HEADER}\n")
file(APPEND "${compiler_wrapper}"  "caf_mod_dir=\"${CMAKE_INSTALL_FULL_INCLUDEDIR}/mod\"\n")
file(APPEND "${compiler_wrapper}"  "caf_lib_dir=\"${CMAKE_INSTALL_FULL_LIBDIR}\"\n")
file(APPEND "${compiler_wrapper}"  "caf_version=${full_git_describe}\n")
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
  file(APPEND "${compiler_wrapper}"  "CAFC=${MPI_Fortran_COMPILER}\n")
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
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/cafrun-head CAFRUN_HEADER)
file(WRITE  "${caf_launcher}" "${CAFRUN_HEADER}\n")
file(APPEND "${caf_launcher}"  "caf_version=${full_git_describe}\n")
file(APPEND "${caf_launcher}"  "CAFRUN=${MPIEXEC}\n")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/cafrun-foot FOOTER)
file(APPEND "${caf_launcher}" "${FOOTER}")

#set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${exe_dir}/cafrun;${exe_dir}/caf;${exe_dir}/test-caf-tally.sh")
