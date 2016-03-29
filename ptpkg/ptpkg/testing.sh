#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: tests.sh
# Created: 12 Feb. 2012
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Package regression and unit testing
#

declare -a PTPKG_TEST_STATUS

#
# Executes the package test suite, if it exists
# Arguments: none
# Returns: 0 if all tests pass, 1 otherwise
#
function execPackageTests {

  # Skip if there are no tests
  if ! [ -d "$PTPKG_PKG_TEST_DIR" ] ; then
    PTPKG_SCRIPT_FLAG="skip"
    PTPKG_TEST_STATUS=( "${PTPKG_TEST_STATUS[@]}" "SKIP:No tests:$PTPKG_PKG" )
    return 1
  fi

  initPackageLogfile "test" "$PTPKG_PKG TEST LOG"

  # Test in a subshell to facilitate logging
  (
    echo
    hline
    echo "  $PTPKG_PKG: Testing"
    echo "  Test Suite: $PTPKG_PKG_TEST_DIR"
    hline
    echo

    cd $PTPKG_PKG_TEST_DIR || exit 2

    # Add package dependencies to the environment
    bootstrapPackageDeps || \
        exitWithError "$PTPKG_PKG: Dependency packages could not be bootstraped." 2

    # Add package to the environment
    execPackageBootstrap || \
        exitWithError "$PTPKG_PKG: Package could not be bootstraped." 2

    # Mark test beginning
    _START_TIME=`date +%s`

    # Look for test.sh.   If it's not here, do the default actions
    if [ -r test.sh ] ; then
      source test.sh
    else
      # Execute .sh tests
      for _TEST in `ls *.sh 2>/dev/null` ; do
        python `writePythonWrapper "$_TEST"` "$_TEST"
      done

      # Execute .py tests
      for _TEST in `ls *.py 2>/dev/null` ; do
        python "$_TEST"
      done
    fi

    # Mark test end
    _STOP_TIME=`date +%s`
    ((_INSTALL_SEC=$_STOP_TIME - $_START_TIME))

    banner "${PTPKG_PKG}: Tests executed in $_INSTALL_SEC seconds"
  ) 2>&1 | tee $PTPKG_LOGFILE

  # Scan logfile and set flag accordingly
  if egrep -q "^FAILED ?" "$PTPKG_LOGFILE" ; then
    PTPKG_SCRIPT_FLAG="fail"
    PTPKG_TEST_STATUS=( "${PTPKG_TEST_STATUS[@]}" "FAIL:At least one test did not pass:$PTPKG_PKG" )
    return 1
  elif egrep -q "^OK ?" "$PTPKG_LOGFILE" ; then
    PTPKG_SCRIPT_FLAG="pass"
    PTPKG_TEST_STATUS=( "${PTPKG_TEST_STATUS[@]}" "OK::$PTPKG_PKG" )
    return 0
  else
    PTPKG_SCRIPT_FLAG="unknown"
    PTPKG_TEST_STATUS=( "${PTPKG_TEST_STATUS[@]}" "UNKNOWN:See logfile ${PTPKG_LOGFILE##*/}:$PTPKG_PKG" )
    return 1
  fi
}

function writePythonWrapper {
  local _WRAPPER=`newTmpfile`

  cat > $_WRAPPER <<EOF
#!/usr/bin/env python

import unittest
import subprocess
import sys

SCRIPT = "echo"

class TestScriptWrapper(unittest.TestCase):

  def setUp(self):
    self.script = SCRIPT

  def test_script_exec(self):
    for line in self.script.splitlines():
      subprocess.check_call(line.strip(), shell=True)

if __name__ == '__main__':
  scriptfile = open(sys.argv[1], 'r')
  SCRIPT = scriptfile.read()
  unittest.main(argv=[sys.argv[0]])
EOF

  echo $_WRAPPER
}
