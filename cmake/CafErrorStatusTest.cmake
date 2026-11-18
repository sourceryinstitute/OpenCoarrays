# Check that caf and cafrun exit non-zero, not 0, when a command fails under
# their ERR trap. Bash 3.2 reports some statuses, such as 127, as 1. Usage:
#   cmake -DCAF_TEMPLATE=<caf.in> -DCAFRUN_TEMPLATE=<cafrun.in> -DBASH=<bash>
#         -DOUT_DIR=<dir> -P CafErrorStatusTest.cmake
foreach(var IN ITEMS CAF_TEMPLATE CAFRUN_TEMPLATE BASH OUT_DIR)
  if(NOT ${var})
    message(FATAL_ERROR "Missing -D${var}=<value>. Usage:\n"
      "  cmake -DCAF_TEMPLATE=<caf.in> -DCAFRUN_TEMPLATE=<cafrun.in> -DBASH=<bash>"
      " -DOUT_DIR=<dir> -P ${CMAKE_CURRENT_LIST_FILE}")
  endif()
endforeach()

# caf fails to find this library; cafrun fails to run this launcher
set(CAF_LIBS "lib/libmissing.a")
set(MPIEXEC_EXECUTABLE "${OUT_DIR}/missing-mpiexec")
configure_file("${CAF_TEMPLATE}" "${OUT_DIR}/caf" @ONLY)
configure_file("${CAFRUN_TEMPLATE}" "${OUT_DIR}/cafrun" @ONLY)

set(failed "")
# Run `script` with ARGN; record a failure unless the ERR trap reports the error
# and the script exits non-zero
macro(check_failure case script)
  execute_process(COMMAND "${BASH}" "${script}" ${ARGN}
    OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
  string(FIND "${err}" "Please report this error" report)
  if(rc EQUAL 0 OR report EQUAL -1)
    string(APPEND failed "${case} (exit ${rc}, expected non-zero):\n${out}\n${err}\n")
  endif()
endmacro()

check_failure("caf --show with a missing library" "${OUT_DIR}/caf" --show)
check_failure("cafrun --wraps with a missing launcher" "${OUT_DIR}/cafrun" --wraps)

if(failed)
  message(FATAL_ERROR "caf or cafrun hid a failure:\n${failed}")
endif()
