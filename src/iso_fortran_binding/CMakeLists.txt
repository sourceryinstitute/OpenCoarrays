add_library(ISO_Fortran_binding OBJECT ISO_Fortran_binding.c)
add_library(ISO_Fortran_binding SHARED ISO_Fortran_binding.c $<TARGET_OBJECTS:ISO_Fortran_binding>)
add_library(ISO_Fortran_binding STATIC ISO_Fortran_binding.c $<TARGET_OBJECTS:ISO_Fortran_binding>)

set(CFI_SO_VERSION 0)
if(gfortran_compiler)
  if(NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 9.0.0)
    set(CFI_SO_VERSION 2)
  if(NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 8.0.0)
    set(CFI_SO_VERSION 1)
  endif()
endif()

set_target_properties( ISO_Fortran_binding
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${CFI_SO_VERSION}
    PUBLIC_HEADER ISO_Fortran_Binding.h
)

install(DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h")

install( TARGETS ISO_Fortran_binding
  EXPORT OpenCoarraysTargets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
