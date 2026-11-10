# Configure caf with signing settings that contain shell syntax, and check that
# `caf --help` reports them unchanged. Usage:
#   cmake -DCAF_TEMPLATE=<caf.in> -DOUT_DIR=<dir> -DBASH=<bash> -P CafCodesignQuotingTest.cmake
set(CAF_CODESIGN_IDENTITY [[MPI $Dev 'a' "b" `c` \d]])
set(CAF_CODESIGN_IDENTIFIER [[org.$x.'y']])
configure_file("${CAF_TEMPLATE}" "${OUT_DIR}/caf" @ONLY)
# caf prefers these over the configured values
unset(ENV{CAF_CODESIGN_IDENTITY})
unset(ENV{CAF_CODESIGN_IDENTIFIER})
execute_process(COMMAND "${BASH}" "${OUT_DIR}/caf" --help
  OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
foreach(value IN ITEMS "${CAF_CODESIGN_IDENTITY}" "${CAF_CODESIGN_IDENTIFIER}")
  string(FIND "${out}" "Current: '${value}'" pos)
  if(NOT rc EQUAL 0 OR pos EQUAL -1)
    message(FATAL_ERROR "caf --help (exit ${rc}) does not report '${value}':\n${out}\n${err}")
  endif()
endforeach()
