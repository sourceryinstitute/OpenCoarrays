# Check that cafrun strips every `--reenable-auto-cleanup`, keeps the other
# arguments intact, and warns once when failed images are not supported. A fake
# launcher prints each argument it receives in brackets. Usage:
#   cmake -DCAFRUN_TEMPLATE=<cafrun.in> -DBASH=<bash> -DOUT_DIR=<dir>
#         -P CafrunReenableAutoCleanupTest.cmake
foreach(var IN ITEMS CAFRUN_TEMPLATE BASH OUT_DIR)
  if(NOT ${var})
    message(FATAL_ERROR "Missing -D${var}=<value>. Usage:\n"
      "  cmake -DCAFRUN_TEMPLATE=<cafrun.in> -DBASH=<bash> -DOUT_DIR=<dir>"
      " -P ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()

set(MPIEXEC_EXECUTABLE "${OUT_DIR}/fake-mpiexec")
file(WRITE "${MPIEXEC_EXECUTABLE}" "#!/bin/sh\nprintf '[%s]\\n' \"$@\"\n")
file(CHMOD "${MPIEXEC_EXECUTABLE}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
set(HAVE_FAILED_IMG false)
configure_file("${CAFRUN_TEMPLATE}" "${OUT_DIR}/cafrun" @ONLY)

execute_process(COMMAND "${BASH}" "${OUT_DIR}/cafrun" --reenable-auto-cleanup -np 2 prog
                        "a b" --reenable-auto-cleanup "" last
  OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
string(REGEX MATCHALL "is a noop" noops "${err}")
list(LENGTH noops noop_count)
set(expected "[-np]\n[2]\n[prog]\n[a b]\n[]\n[last]\n")
if(NOT rc EQUAL 0 OR NOT out STREQUAL expected OR NOT noop_count EQUAL 1)
  message(FATAL_ERROR "cafrun mishandled --reenable-auto-cleanup (exit ${rc}, expected 0;"
    " ${noop_count} noop warnings, expected 1).\nExpected launcher arguments:\n${expected}"
    "Got:\n${out}\nstderr:\n${err}")
endif()
