configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Makefile ${CMAKE_BINARY_DIR}/bin_staging/transpiler/Makefile COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/caf-lower.str ${CMAKE_BINARY_DIR}/bin_staging/transpiler/caf-lower.str COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fast-to-ocaf.str ${CMAKE_BINARY_DIR}/bin_staging/transpiler/fast-to-ocaf.str COPYONLY)

install(DIRECTORY ${CMAKE_BINARY_DIR}/bin_staging/transpiler DESTINATION .)
