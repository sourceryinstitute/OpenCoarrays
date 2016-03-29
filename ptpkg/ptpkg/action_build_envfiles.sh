#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: action_build_envfiles.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Rebuilds environment files $PTPKG_INSTALL_PREFIX/etc
#

################################################################################
#
# Default configuration
#
################################################################################

PTPKG_PREFIX_FLAG="ask"
PTPKG_TARGET_FLAG="auto"

################################################################################
#
# Function definitions
#
################################################################################

#
# Initializes action.
# Arguments: Command line arguments
# Returns: 0 if action was initilized successfully, 1 otherwise
#
function initAction {
  local _OS
  local _DIST
  local _COMP
  local _ARCH
  local _TARG
  local _ARGVAL

  # Process arguments
  while [ $# -gt 0 ] ; do
    _ARGVAL="`echo $1 | cut -d= -f2-`"
    case "$1" in
      *help*)
        echo
        hline
        echo " ** Build Environment Files command parameters **"
        echo "  The first option is the default when the parameter is omitted"
        echo "  The option in ALL-CAPS is the default when the parameter value is omitted"
        echo
        echo "  --prefix=(ask|<path>)         Installation root directory"
        echo "  --target=(auto|ask|<target>)  Specify target configuration"
        echo "  --help                        Show this help message"
        echo
        echo " ** Examples **"
        echo
        echo "  configure.sh build_envfiles:"
        echo "     prefix=ask, target=auto"
        echo
        echo "  configure.sh install --prefix=<path>"
        echo "     prefix=<path>, target=auto"
        echo
        echo "  configure.sh install --target=linux-suse11cray-gnu-x86_64"
        echo "     prefix=ask, target=linux-suse11cray-gnu-x86_64"
        hline
        echo
        return 1
        ;;
      --prefix*)
        case "$_ARGVAL" in
          ask|ASK)
            PTPKG_PREFIX_FLAG="ask"
            ;;
          $1)
            echo "--prefix requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --prefix must be an absolute path."
              return 1
            fi
            PTPKG_PREFIX_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      --target*)
        case "$_ARGVAL" in
          auto|AUTO)
            PTPKG_TARGET_FLAG="auto"
            ;;
          ask|ASK)
            PTPKG_TARGET_FLAG="ask"
            ;;
          $1)
            echo "--target requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            PTPKG_TARGET_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      *)
        echo "Unknown argument: $1.  Try '--help' for help."
        return 1
        ;;
    esac
    shift
  done

  # Discover supported targets
  getTargets

  # Get host configuration
  resolveHost

  # Select a target configuration
  case "$PTPKG_TARGET_FLAG" in
    ask)
      banner "Please select a target configuration"
      select _TARG in ${PTPKG_ALL_TARGETS[@]} ; do
        selectTarget "$_TARG" && break
        exitWithError "Invalid target: $_TARG.  The targets database is corrupt."
      done
      ;;
    auto)
      if ! autoSelectTarget ; then
        echo "The target configuration could not be determined."
        echo "Please specify a target with the --target flag."
        showAllTargets
        exitWithError "Aborting..."
      fi
      ;;
    *)
      if ! selectTarget "$PTPKG_TARGET_FLAG" ; then
        echo "Invalid target specified on command line: $PTPKG_TARGET_FLAG"
        showAllTargets
        echo
        exitWithError "Aborting..."
      fi
      ;;
  esac
  echo "Selected target configuration: $PTPKG_TARGET"

  # Load packages database
  getPackages

  return 0
}

################################################################################
#
# BEGIN installation action script
#
################################################################################

# Import common functionality
source `cd ${0%/*} && pwd -P`/ptpkg.sh

# Initialize action
initAction $@ || exit 1

# Get installation destination from user
if [ "$PTPKG_PREFIX_FLAG" == "ask" ] ; then
  while true ; do
    promptForDir "Please enter the absolute path to the installation" "$PTPKG_HOME"
    setPrefix "$PTPKG_PROMPTFORDIR_RESULT" && break
  done
else
  setPrefix "$PTPKG_PREFIX_FLAG" || \
      exitWithError "Invalid path specified on command line: $PTPKG_PREFIX_FLAG"
fi

# Show work to be done
echo
hline
echo "  Host:   $PTPKG_HOST"
echo "  Target: $PTPKG_TARGET"
echo "  Install directory: $PTPKG_INSTALL_PREFIX"
hline
echo
promptYesNo "Do you wish to continue?" || exit 0

# Check installation destination
[ -d "$PTPKG_PREFIX/$PTPKG_TARGET" ] || \
  exitWithError "Target $PTPKG_TARGET is not installed at $PTPKG_PREFIX"

# Initialize installation destination
initPrefix || exitWithError "Error: Cannot initilize $PTPKG_PREFIX"

# Start proceedure timer
PTPKG_START_TIME=`date +%s`

# Write environment scripts
banner "Writing bashrc environment file"
writeBashrc
echo

banner "Writing cshrc environment file"
writeCshrc
echo

banner "Writing $PTPKG_RELEASE_NAME module file"
writeModule
echo


# Stop proceedure timer
PTPKG_STOP_TIME=`date +%s`
((PTPKG_SEC=$PTPKG_STOP_TIME - $PTPKG_START_TIME))

echo "Completed in ${PTPKG_SEC} seconds"
echo

################################################################################
#
# END action script
#
################################################################################

# EOF
