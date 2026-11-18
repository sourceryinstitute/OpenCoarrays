# Check that caf prints its usage when stdout is not a terminal, with a `less`
# and a PAGER that would lose the text (#810). Usage:
#   cmake -DCAF=<caf> -DBASH=<bash> -DOUT_DIR=<dir> -P CafHelpTest.cmake
foreach(var IN ITEMS CAF BASH OUT_DIR)
  if(NOT ${var})
    message(FATAL_ERROR "Missing -D${var}=<value>. Usage:\n"
      "  cmake -DCAF=<caf> -DBASH=<bash> -DOUT_DIR=<dir> -P ${CMAKE_CURRENT_LIST_FILE}")
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
# Run caf with ARGN; record a failure unless it exits with `expected_rc` and
# prints its usage, from the `Usage:` line to the last variable it documents
macro(check_usage case expected_rc)
  execute_process(COMMAND "${BASH}" "${CAF}" ${ARGN}
    OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
  string(FIND "${out}" " Usage: " first)
  string(FIND "${out}" "CAF_CODESIGN_IDENTIFIER" last)
  if(NOT rc EQUAL ${expected_rc} OR first EQUAL -1 OR last EQUAL -1)
    string(APPEND failed "${case} (exit ${rc}, expected ${expected_rc}):\n${out}\n${err}\n")
  endif()
endmacro()

check_usage("caf --help with PAGER unset" 0 --help)
check_usage("caf without arguments, PAGER unset" 1)
# A pager must not run when stdout is not a terminal
set(ENV{PAGER} false)
check_usage("caf --help with PAGER=false" 0 --help)

if(failed)
  message(FATAL_ERROR "caf did not print its usage with the expected exit status:\n${failed}")
endif()
