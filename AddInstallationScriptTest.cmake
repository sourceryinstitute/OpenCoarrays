macro(add_installation_script_test name path)

  # Copy the source to the binary tree
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${path}/${name}
    ${CMAKE_CURRENT_BINARY_DIR}/${path}/${name}
    COPYONLY
  )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${path}/${name}-usage
    ${CMAKE_CURRENT_BINARY_DIR}/${path}/${name}-usage
    COPYONLY
  )
  add_test(NAME test-${name} COMMAND "${CMAKE_BINARY_DIR}/${path}/${name}")
  set_property(TEST test-${name} PROPERTY WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${path}")
  set_property(TEST test-${name} PROPERTY ENVIRONMENT "OPENCOARRAYS_SRC_DIR=${CMAKE_SOURCE_DIR}")
endmacro(add_installation_script_test)
