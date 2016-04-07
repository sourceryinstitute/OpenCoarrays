# Write the script that downloads, builds, and installs OpenCoarrays prerequisites from source
set(exe_dir ${CMAKE_BINARY_DIR}/bin_staging)
set(build_script ${exe_dir}/build)
install(
    FILES "${build_script}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/build BUILD_SCRIPT)
file(WRITE  "${build_script}" "${BUILD_SCRIPT}\n")
