# Check that caf and cafrun write their usage straight to stdout when stdout
# is not a terminal (#810). A fake `less` and `PAGER=false` discard the text,
# so the test fails if either script sends its usage to a pager. Usage:
#   cmake -DCAF=<caf> -DCAFRUN=<cafrun> -DBASH=<bash> -DOUT_DIR=<dir>
#         -P CafHelpTest.cmake
foreach(var IN ITEMS CAF CAFRUN BASH OUT_DIR)
  if(NOT ${var})
    message(FATAL_ERROR "Missing -D${var}=<value>. Usage:\n"
      "  cmake -DCAF=<caf> -DCAFRUN=<cafrun> -DBASH=<bash> -DOUT_DIR=<dir>"
      " -P ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()

# Put first on PATH a `less` that reads nothing, so caf loses its usage if it
# pages. Keep the rest of PATH: MSYS2 tools need the DLLs found through it
file(WRITE "${OUT_DIR}/bin/less" "#!/bin/sh\nexit 0\n")
file(CHMOD "${OUT_DIR}/bin/less" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
if(CMAKE_HOST_WIN32)
  set(ENV{PATH} "${OUT_DIR}/bin;$ENV{PATH}")
else()
  set(ENV{PATH} "${OUT_DIR}/bin:$ENV{PATH}")
endif()
unset(ENV{PAGER})

set(failed "")
# Run `script` with ARGN; record a failure unless it exits with `expected_rc`
# and prints its usage, from the `Usage:` line to the text `last` near its end
macro(check_usage case expected_rc script last)
  execute_process(COMMAND "${BASH}" "${script}" ${ARGN}
    OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
  string(FIND "${out}" " Usage: " first_pos)
  string(FIND "${out}" "${last}" last_pos)
  if(NOT rc EQUAL ${expected_rc} OR first_pos EQUAL -1 OR last_pos EQUAL -1)
    string(APPEND failed "${case} (exit ${rc}, expected ${expected_rc}):\n${out}\n${err}\n")
  endif()
endmacro()

check_usage("caf --help with PAGER unset" 0 "${CAF}" CAF_CODESIGN_IDENTIFIER --help)
check_usage("caf without arguments, PAGER unset" 1 "${CAF}" CAF_CODESIGN_IDENTIFIER)
check_usage("cafrun without arguments" 1 "${CAFRUN}" "caf --wraps")
# A pager must not run when stdout is not a terminal
set(ENV{PAGER} false)
check_usage("caf --help with PAGER=false" 0 "${CAF}" CAF_CODESIGN_IDENTIFIER --help)

if(failed)
  message(FATAL_ERROR "caf or cafrun did not print its usage with the expected exit status:\n"
    "${failed}")
endif()
