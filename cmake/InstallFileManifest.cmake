# This script will create and install an install manifest, including SHA256 hashes of each installed file
# Variables passed from CMake must be set with `install(CODE "set(...)")`

message(STATUS "Generating SHA256 checksums for all installed assets")

# Mimic cmake_install.cmake's handlin of components
if(CMAKE_INSTALL_COMPONENT)
  set(HASHED_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.SHA256.txt")
else()
  set(HASHED_INSTALL_MANIFEST "install_manifest.SHA256.txt")
endif()

# Clean out any old install manifest on re-installation, if it exists
file(REMOVE "${CMAKE_BINARY_DIR}/${HASHED_INSTALL_MANIFEST}")

# Loop over files computing hashes
foreach(file IN LISTS CMAKE_INSTALL_MANIFEST_FILES)
  file(SHA256 "${file}" _file_sha256)
  file(APPEND "${CMAKE_BINARY_DIR}/${HASHED_INSTALL_MANIFEST}" "${_file_sha256}  ${file}\n")
endforeach()

file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
  "${CMAKE_BINARY_DIR}/${HASHED_INSTALL_MANIFEST}")

file(SHA256 "${CMAKE_BINARY_DIR}/${HASHED_INSTALL_MANIFEST}" MANIFEST_SHA256)
message(STATUS
  "Global checksum for OpenCoarrays installation:\n${MANIFEST_SHA256}  ${HASHED_INSTALL_MANIFEST}")
message(STATUS "${PROJECT_NAME} was configured with SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH}")
message(STATUS "The current environment has SOURCE_DATE_EPOCH set to: $ENV{SOURCE_DATE_EPOCH}")
