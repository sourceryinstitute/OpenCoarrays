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
  set(harness "${CMAKE_BINARY_DIR}/staging/test-${name}.sh")
  install(
      FILES "${harness}"
      PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
      DESTINATION ${CMAKE_BINARY_DIR}/${path}
  )
  file(WRITE  "${harness}" "#!/bin/bash\n")
  file(APPEND "${harness}" "cd ${CMAKE_BINARY_DIR}/${path}\n")
  file(APPEND "${harness}" "OPENCOARRAYS_SRC_DIR=${CMAKE_SOURCE_DIR} ./${name}\n")
  add_test(NAME ${name} COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${path}/test-${name}.sh")
 #set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION "Test passed.")
endmacro(add_installation_script_test)
