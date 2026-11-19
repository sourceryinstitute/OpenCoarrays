# Configure the uninstall script for the scratch install that
# InstallPrefixTest.cmake made, run it, and check what it removed. Usage:
#   cmake -DUNINSTALL_TEMPLATE=<uninstall.cmake.in> -DOUT_DIR=<scratch dir>
#         -DCMAKE_INSTALL_BINDIR=<dir> -DCMAKE_INSTALL_INCLUDEDIR=<dir>
#         -DCMAKE_INSTALL_LIBDIR=<dir> -DCMAKE_INSTALL_DATADIR=<dir>
#         -Dmod_dir_tail=<module dir name> -DPROJECT_NAME=<name>
#         -P UninstallTest.cmake
foreach(var IN ITEMS UNINSTALL_TEMPLATE OUT_DIR CMAKE_INSTALL_BINDIR CMAKE_INSTALL_INCLUDEDIR
                     CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATADIR mod_dir_tail PROJECT_NAME)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "${var} is not set; see the usage at the top of ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()
set(prefix "${OUT_DIR}/prefix")
set(ENV{DESTDIR} "${OUT_DIR}/destdir")
set(root "$ENV{DESTDIR}${prefix}")
# The template reads @CMAKE_BINARY_DIR@/install_manifest.txt
set(CMAKE_BINARY_DIR "${OUT_DIR}")
configure_file("${UNINSTALL_TEMPLATE}" "${OUT_DIR}/uninstall.cmake" @ONLY)

# Run the uninstall script; set `out`, `err` and `log` in the caller. Stop if it fails,
# unless given a variable name: then set that variable to its exit status in the caller.
function(uninstall)
  execute_process(COMMAND "${CMAKE_COMMAND}" -P "${OUT_DIR}/uninstall.cmake"
    OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
  set(log "\n${out}${err}")
  if(ARGC GREATER 0)
    set(${ARGV0} "${rc}" PARENT_SCOPE)
  elseif(NOT rc EQUAL 0)
    message(FATAL_ERROR "uninstall failed with exit status ${rc}:${log}")
  endif()
  set(out "${out}" PARENT_SCOPE)
  set(err "${err}" PARENT_SCOPE)
  set(log "${log}" PARENT_SCOPE)
endfunction()

# A file that is already gone must be skipped and reported
set(missing "${root}/${CMAKE_INSTALL_BINDIR}/caf")
file(REMOVE "${missing}")
uninstall()
string(FIND "${out}" "File ${missing} does not exist." pos)
if(pos EQUAL -1)
  message(FATAL_ERROR "uninstall did not report the missing ${missing}:${log}")
endif()
file(GLOB_RECURSE left "${root}/*")
if(left)
  list(JOIN left "\n  " left)
  message(FATAL_ERROR "uninstall left files:\n  ${left}${log}")
endif()
foreach(dir IN ITEMS "${CMAKE_INSTALL_INCLUDEDIR}/${mod_dir_tail}" "${CMAKE_INSTALL_LIBDIR}/cmake/opencoarrays"
                     "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  if(EXISTS "${root}/${dir}")
    message(FATAL_ERROR "uninstall did not remove the empty directory ${root}/${dir}${log}")
  endif()
endforeach()
# Shared directories that held installed files must stay
foreach(dir IN ITEMS "${CMAKE_INSTALL_BINDIR}" "${CMAKE_INSTALL_INCLUDEDIR}" "${CMAKE_INSTALL_LIBDIR}")
  if(NOT IS_DIRECTORY "${root}/${dir}")
    message(FATAL_ERROR "uninstall removed the shared directory ${root}/${dir}${log}")
  endif()
endforeach()

# A directory of OpenCoarrays' own that holds someone else's file must stay
set(kept "${root}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/not-installed")
file(WRITE "${kept}" "")
uninstall()
if(NOT EXISTS "${kept}")
  message(FATAL_ERROR "uninstall removed ${kept}, which it did not install${log}")
endif()
# CTest runs all fixture repeats first, so this test's reruns must find no files
file(REMOVE "${kept}")

# The same holds when the path has glob characters, which `file(GLOB)` would misread
set(ENV{DESTDIR} "${OUT_DIR}/destdir [1]")
set(own_dir "$ENV{DESTDIR}${prefix}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
set(kept "${own_dir}/not-installed")
file(WRITE "${kept}" "")
uninstall()
if(NOT EXISTS "${kept}")
  message(FATAL_ERROR "uninstall removed ${kept}, which it did not install${log}")
endif()
# Once empty, the directory must go
file(REMOVE "${kept}")
uninstall()
if(EXISTS "${own_dir}")
  message(FATAL_ERROR "uninstall did not remove the empty directory ${own_dir}${log}")
endif()
file(REMOVE_RECURSE "$ENV{DESTDIR}")

# A file that cannot be removed must make uninstall fail and name it, once it has
# removed the rest. Make it undeletable by putting it in a read-only directory.
set(ENV{DESTDIR} "${OUT_DIR}/destdir-read-only")
set(read_only_dir "$ENV{DESTDIR}${prefix}/${CMAKE_INSTALL_BINDIR}")
set(writable OWNER_READ OWNER_WRITE OWNER_EXECUTE)
# A killed earlier run may have left the directory read-only
if(IS_DIRECTORY "${read_only_dir}")
  file(CHMOD "${read_only_dir}" PERMISSIONS ${writable})
endif()
file(REMOVE_RECURSE "$ENV{DESTDIR}")
set(stuck "${read_only_dir}/caf")
set(config_dir "$ENV{DESTDIR}${prefix}/${CMAKE_INSTALL_LIBDIR}/cmake/opencoarrays")
set(removable "${config_dir}/OpenCoarraysConfig.cmake")
file(WRITE "${stuck}" "")
file(WRITE "${read_only_dir}/probe" "")
file(WRITE "${removable}" "")
file(CHMOD "${read_only_dir}" PERMISSIONS OWNER_READ OWNER_EXECUTE)
# Root, for one, can still remove files there; then this case cannot run
file(REMOVE "${read_only_dir}/probe")
if(EXISTS "${read_only_dir}/probe")
  uninstall(rc)
endif()
# Restore the permissions before any check can stop the test, so cleanup works
file(CHMOD "${read_only_dir}" PERMISSIONS ${writable})
if(NOT EXISTS "${read_only_dir}/probe")
  message(STATUS "Skipping the undeletable file case: this user can remove files in a"
                 " read-only directory.")
else()
  if(rc EQUAL 0)
    message(FATAL_ERROR "uninstall succeeded, but could not remove ${stuck}${log}")
  endif()
  # Its progress messages go to stdout, so an error naming the file is on stderr
  string(FIND "${err}" "${stuck}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "uninstall failed without naming ${stuck}${log}")
  endif()
  if(EXISTS "${removable}" OR EXISTS "${config_dir}")
    message(FATAL_ERROR "uninstall stopped before it removed ${removable} and its directory${log}")
  endif()
endif()
file(REMOVE_RECURSE "$ENV{DESTDIR}")
