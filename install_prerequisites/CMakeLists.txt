# Write the script that builds GCC from source 
set(exe_dir ${CMAKE_BINARY_DIR}/bin_staging)
set(gcc_build_script ${exe_dir}/buildgcc)
install(
    FILES "${gcc_build_script}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/buildgcc BUILDGCC_SCRIPT)
file(WRITE  "${gcc_build_script}" "${BUILDGCC_SCRIPT}\n")

# Write the script that builds MPICH from source 
set(mpich_build_script ${exe_dir}/buildmpich)
install(
    FILES "${mpich_build_script}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/buildmpich BUILDMPICH_SCRIPT)
file(WRITE  "${mpich_build_script}" "${BUILDMPICH_SCRIPT}\n")

# Write the script that builds CMake from source 
set(cmake_build_script ${exe_dir}/buildcmake)
install(
    FILES "${cmake_build_script}"
    PERMISSIONS WORLD_EXECUTE WORLD_READ WORLD_WRITE OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ GROUP_WRITE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/buildcmake BUILDCMAKE_SCRIPT)
file(WRITE  "${cmake_build_script}" "${BUILDCMAKE_SCRIPT}\n")
