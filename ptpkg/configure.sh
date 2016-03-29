#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: configure.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Selects and performs a PTPKG action
#

# Import common functionality
source `cd ${0%/*} && pwd -P`/ptpkg/ptpkg.sh

function showUsage {
  echo "$PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)"
  echo "$PTPKG_RELEASE_DESC_SHORT"
  echo
  echo "Usage: $0 action [action arguments]"
  echo "    Available actions:"
  for action in ${PTPKG_ACTIONS[@]} ; do
    echo "      $action"
  done
  echo "    Hint: <action> --help to get help for an action"
}


# Check arguments
if [ $# -eq 0 ] ; then
  showUsage
  exit 1
fi

# Get action from user and execute script
_VALID=false
_ACTION="$1"
shift

for ((i=0; i<${#PTPKG_ACTIONS[@]}; i++)) {
  if [ "$_ACTION" == "${PTPKG_ACTIONS[$i]}" ] ; then
    $PTPKG_SRC_DIR/${PTPKG_ACTION_SCRIPTS[$i]} "$@"
    _VALID=true
    break
  fi
}

if [ $_VALID != true ] ; then
  echo "Unknown action: $_ACTION"
  showUsage
fi

# EOF
