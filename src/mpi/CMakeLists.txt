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
  # This applied to gfortran < 5.2
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


add_library(opencoarrays_mod OBJECT ../extensions/opencoarrays.F90)
set_target_properties(opencoarrays_mod
  PROPERTIES
  Fortran_MODULE_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${mod_dir_tail}"
  POSITION_INDEPENDENT_CODE TRUE)


add_library(caf_mpi SHARED mpi_caf.c ../common/caf_auxiliary.c $<TARGET_OBJECTS:opencoarrays_mod>)
add_library(caf_mpi_static STATIC  mpi_caf.c ../common/caf_auxiliary.c $<TARGET_OBJECTS:opencoarrays_mod>)
target_link_libraries(caf_mpi PRIVATE ${MPI_C_LIBRARIES} ${MPI_Fortran_LIBRARIES})
target_link_libraries(caf_mpi_static PRIVATE ${MPI_C_LIBRARIES} ${MPI_Fortran_LIBRARIES})

set_target_properties ( caf_mpi
  PROPERTIES
  SOVERSION ${PROJECT_VERSION.MAJOR}.${PROJECT_VERSION.MINOR}
  VERSION ${PROJECT_VERSION}
  )


if (gfortran_compiler)
  target_compile_options(caf_mpi INTERFACE -fcoarray=lib)
  target_compile_options(caf_mpi_static INTERFACE -fcoarray=lib)
endif()

install(TARGETS caf_mpi EXPORT OpenCoarraysTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(TARGETS caf_mpi_static EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )


############### FIXME!!!!!   set_target_properties(caf_mpi PROPERTIES SOVERSION 1 SONAME "libcaf_mpi.so.${OpenCoarraysVersion}")


##############################################
# Configure `caf` and `cafrun` wrapper scripts
##############################################
# List of caf.in variables needing configuration:
#
# @CAF_VERSION@ @opencoarrays_aware_compiler@ @Fortran_COMPILER@ @CAF_MODDIR@
# @MPI_Fortran_LINK_FLAGS@ @MPI_Fortran_COMPILE_FLAGS@
# @CAF_LIBS@ @MPI_LIBS@
#

set(CAF_VERSION "${full_git_describe}")
set(Fortran_COMPILER "${CMAKE_Fortran_COMPILER}")
set(CAF_MODDIR "${CMAKE_INSTALL_INCLUDEDIR}/${mod_dir_tail}")
set(MOD_DIR_FLAG "${CMAKE_Fortran_MODDIR_FLAG}")
set(MPI_LIBS "")
foreach( lib IN LISTS MPI_Fortran_LIBRARIES)
  set(MPI_LIBS "${MPI_LIBS} \"${lib}\"")
endforeach()
string(STRIP "${MPI_LIBS}" MPI_LIBS)
set_target_properties(caf_mpi_static
  PROPERTIES OUTPUT_NAME caf_mpi)
get_target_property(libcaf_static caf_mpi_static OUTPUT_NAME)
set(CAF_LIBS "${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${libcaf_static}${CMAKE_STATIC_LIBRARY_SUFFIX}")

configure_file("${CMAKE_SOURCE_DIR}/src/extensions/caf.in" "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/caf"
  @ONLY)


# List of carrun.in variables needing configuration:
#
# @CAF_VERSION@ @MPIEXEC@ @MPIEXEC_NUMPROC_FLAG@ @MPIEXEC_PREFLAGS@ @MPIEXEC_POSTFLAGS@
# @HAVE_FAILED_IMG@
#

if(CAF_ENABLE_FAILED_IMAGES)
  set(HAVE_FAILED_IMG "true")
else()
  set(HAVE_FAILED_IMG "false")
endif()

configure_file("${CMAKE_SOURCE_DIR}/src/extensions/cafrun.in" "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/cafrun"
  @ONLY)

install(PROGRAMS "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/cafrun"
  "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/caf"
  DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}")
