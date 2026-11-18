# Install the build with `cmake --install --prefix` into a scratch directory,
# and check that every installed file is under that prefix. Usage:
#   cmake -DBUILD_DIR=<build dir> -DOUT_DIR=<scratch dir> -DBINDIR=<bin dir>
#         [-DCONFIG=<build configuration>] -P InstallPrefixTest.cmake
# Leaves the install, and its manifest as <scratch dir>/install_manifest.txt,
# for UninstallTest.cmake.
foreach(var IN ITEMS BUILD_DIR OUT_DIR BINDIR)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "${var} is not set; see the usage at the top of ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()
# Install each component on its own, so the build's own install_manifest.txt is left
# alone: `documentation` holds the man pages, `Unspecified` everything else
set(components Unspecified documentation)
# Put back records that a killed earlier run left moved aside, before deleting its files.
# Name them, rather than use `file(GLOB)`, which would read `[`, `*` or `?` in the path
# as patterns, miss the records, and let them be deleted.
foreach(component IN LISTS components)
  foreach(record IN ITEMS install_manifest_${component}.txt install_manifest_${component}.SHA256.txt)
    if(EXISTS "${OUT_DIR}/saved-${record}")
      file(RENAME "${OUT_DIR}/saved-${record}" "${BUILD_DIR}/${record}")
    endif()
  endforeach()
endforeach()
file(REMOVE_RECURSE "${OUT_DIR}")
file(MAKE_DIRECTORY "${OUT_DIR}")
set(prefix "${OUT_DIR}/prefix")
# Install the configuration CTest runs; single-config builds may have none
set(config_args "")
if(NOT "${CONFIG}" STREQUAL "")
  set(config_args --config "${CONFIG}")
endif()
# Under DESTDIR, even files installed to an absolute path stay in the scratch directory
set(ENV{DESTDIR} "${OUT_DIR}/destdir")
set(files "")
foreach(component IN LISTS components)
  # Move aside the build's records of a real install of this component, to restore them
  set(records install_manifest_${component}.txt install_manifest_${component}.SHA256.txt)
  foreach(record IN LISTS records)
    if(EXISTS "${BUILD_DIR}/${record}")
      file(RENAME "${BUILD_DIR}/${record}" "${OUT_DIR}/saved-${record}")
    endif()
  endforeach()
  execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
                          --component ${component} ${config_args}
    RESULT_VARIABLE rc)
  if(rc EQUAL 0)
    file(STRINGS "${BUILD_DIR}/install_manifest_${component}.txt" component_files)
    list(APPEND files ${component_files})
  endif()
  # Remove this test's manifests from the build tree, and put the saved records back
  foreach(record IN LISTS records)
    file(REMOVE "${BUILD_DIR}/${record}")
    if(EXISTS "${OUT_DIR}/saved-${record}")
      file(RENAME "${OUT_DIR}/saved-${record}" "${BUILD_DIR}/${record}")
    endif()
  endforeach()
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "cmake --install --component ${component} failed with exit status ${rc}")
  endif()
endforeach()
list(JOIN files "\n" manifest)
file(WRITE "${OUT_DIR}/install_manifest.txt" "${manifest}")
set(outside "")
foreach(file IN LISTS files)
  string(FIND "${file}" "${prefix}/" pos)
  if(NOT pos EQUAL 0)
    string(APPEND outside "\n  ${file}")
  endif()
endforeach()
if(outside)
  message(FATAL_ERROR "Installed outside the --prefix ${prefix}:${outside}")
endif()
foreach(program IN ITEMS caf cafrun)
  if(NOT EXISTS "$ENV{DESTDIR}${prefix}/${BINDIR}/${program}")
    message(FATAL_ERROR "${program} is not in ${prefix}/${BINDIR}")
  endif()
endforeach()
