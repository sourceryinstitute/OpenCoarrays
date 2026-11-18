# Install the build with `cmake --install --prefix` into a scratch directory,
# and check that every installed file is under that prefix. Usage:
#   cmake -DBUILD_DIR=<build dir> -DOUT_DIR=<scratch dir> -DBINDIR=<bin dir>
#         -P InstallPrefixTest.cmake
# Leaves the install, and its manifest as <scratch dir>/install_manifest.txt.
foreach(var IN ITEMS BUILD_DIR OUT_DIR BINDIR)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "${var} is not set; see the usage at the top of ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()
file(REMOVE_RECURSE "${OUT_DIR}")
set(prefix "${OUT_DIR}/prefix")
# Under DESTDIR, even files installed to an absolute path stay in the scratch directory
set(ENV{DESTDIR} "${OUT_DIR}/destdir")
# Install each component on its own, so the build's own install_manifest.txt is left
# alone: `documentation` holds the man pages, `Unspecified` everything else
set(files "")
foreach(component IN ITEMS Unspecified documentation)
  execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
                          --component ${component}
    RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "cmake --install --component ${component} failed with exit status ${rc}")
  endif()
  file(STRINGS "${BUILD_DIR}/install_manifest_${component}.txt" component_files)
  list(APPEND files ${component_files})
  # Leave no manifests from this test in the build tree
  file(REMOVE "${BUILD_DIR}/install_manifest_${component}.txt"
              "${BUILD_DIR}/install_manifest_${component}.SHA256.txt")
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
