#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: action_test.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Tests an installation
#

################################################################################
#
# Default configuration
#
################################################################################

PTPKG_PREFIX_FLAG="ask"
PTPKG_TARGET_FLAG="auto"
PTPKG_CUSTOMIZE_FLAG="no"
PTPKG_LIST_FLAG="" # Default: don't list

declare -a PTPKG_TEST_SELECTION

################################################################################
#
# Function definitions
#
################################################################################

#
# Initializes test action.
# Sets PTPKG_CONFIG_* flags according to the arguments.
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
  local _PREFIX

  # Process arguments
  while [ $# -gt 0 ] ; do
    _ARGVAL="`echo $1 | cut -d= -f2-`"
    case "$1" in
      *help*)
        echo
        hline
        echo " ** Test command parameters **"
        echo "  The first option is the default when the parameter is omitted"
        echo "  The option in ALL-CAPS is the default when the parameter value is omitted"
        echo
        echo "  --prefix=(ask|<path>)         Installation root directory"
        echo "  --target=(auto|ask|<target>)  Specify target configuration"
        echo "  --customize[=(no|YES|ask)]    Disable, enable, or ask for test suite customization"
        echo "  --list[=(ALL|with|without)]   No action if omitted.  List all package tests, only"
        echo "                                packages with tests, or only packages without tests."
        echo "  --help                        Show this help message"
        echo
        echo " ** Examples **"
        echo
        echo "  configure.sh test"
        echo "     prefix=ask, target=auto, customize=no"
        echo
        echo "  configure.sh test --customize --target=linux-suse11cray-gnu-x86_64"
        echo "     prefix=ask, target=linux-suse11cray-gnu-x86_64, customize=yes"
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
      --customize*)
        case "$_ARGVAL" in
          no|NO)
            PTPKG_CUSTOMIZE_FLAG="no"
            ;;
          ask|ASK)
            PTPKG_CUSTOMIZE_FLAG="ask"
            ;;
          $1|yes|YES)
            PTPKG_CUSTOMIZE_FLAG="yes"
            ;;
          *)
            echo "Invalid value for --customize: $_ARGVAL.  Try '--help' for help."
            return 1
            ;;
        esac
        ;;
      --list*)
        case "$_ARGVAL" in
          $1|all|ALL)
            PTPKG_LIST_FLAG="all"
            ;;
          with|WITH)
            PTPKG_LIST_FLAG="with"
            ;;
          without|WITHOUT)
            PTPKG_LIST_FLAG="without"
            ;;
          *)
            echo "Invalid value for --list: $_ARGVAL.  Try '--help' for help."
            return 1
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
  echo "Loading targets database..."
  getTargets

  # Get host configuration
  resolveHost
  echo "Detected host configuration: $PTPKG_HOST"

  # Get installation destination from user
  if [ "$PTPKG_PREFIX_FLAG" == "ask" ] ; then
    while true ; do
      promptForDir "Please enter the absolute path to the installation" "$PTPKG_HOME"
      if isAbsolutePath "$PTPKG_PROMPTFORDIR_RESULT" ; then
        break
      else
        echo "The path must be absolute."
      fi
    done
    _PREFIX="$PTPKG_PROMPTFORDIR_RESULT"
  else
    isAbsolutePath "$PTPKG_PREFIX_FLAG" || \
        exitWithError "Invalid or relative path specified on command line: $PTPKG_PREFIX_FLAG"
    _PREFIX="$PTPKG_PREFIX_FLAG"
  fi

  # Discover installed targets
  PTPKG_INSTALLED_TARGETS=( )
  for _TARG in ${PTPKG_ALL_TARGETS[@]} ; do
    if [ -d "$_PREFIX/$_TARG" ] ; then
      PTPKG_INSTALLED_TARGETS=( ${PTPKG_INSTALLED_TARGETS[@]} "$_TARG" )
    fi
  done
  if [ ${#PTPKG_INSTALLED_TARGETS[@]} -eq 0 ] ; then
    exitWithError "No targets are installed at $_PREFIX"
  fi

  # Select a target configuration
  case "$PTPKG_TARGET_FLAG" in
    ask)
      banner "Please select a target configuration"
      select _TARG in ${PTPKG_INSTALLED_TARGETS[@]} ; do
        selectTarget "$_TARG" && break
        exitWithError "Invalid target: $_TARG.  The targets database is corrupt."
      done
      ;;
    auto)
      if [ -d "$_PREFIX/$PTPKG_HOST" ] ; then
        selectTarget "$PTPKG_HOST" || \
            exitWithError "Invalid target: $PTPKG_HOST"
      else
        PTPKG_TARGET=""
        for _OS in "$PTPKG_HOST_OS" "any" ; do
          for _DIST in "$PTPKG_HOST_DIST" "any" ; do
            for _COMP in "$PTPKG_HOST_COMP" "any" ; do
              for _ARCH in "$PTPKG_HOST_ARCH" "any" ; do
                _TARG="${_OS}-${_DIST}-${_COMP}-${_ARCH}"
                if [ -d "$_PREFIX/$_TARG" ] ; then
                  selectTarget "$_TARG" || \
                      exitWithError "Invalid target: $_TARG"
                fi
              done
            done
          done
        done
        if [ -z "$PTPKG_TARGET" ] ; then
          echo "The target configuration could not be determined."
          echo "Please specify a target with the --target flag."
          showAllTargets
          exitWithError "Aborting..."
        fi
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

  # Discover available packages
  echo "Loading packages database..."
  getPackages

  # Set the installation prefix
  setPrefix "$_PREFIX" || \
      exitWithError "Failed to set PTPKG_PREFIX to $_PREFIX"

  # Discover installed packages
  echo "Scanning for installed packages..."
  PTPKG_INSTALLED_PACKAGES=( )
  for _PKG in ${PTPKG_ALL_PACKAGES[@]} ; do
    if [ -d "$PTPKG_INSTALL_PREFIX/$_PKG" ] ; then
      PTPKG_INSTALLED_PACKAGES=( ${PTPKG_INSTALLED_PACKAGES[@]} "$_PKG" )
    fi
  done
  if [ ${#PTPKG_INSTALLED_PACKAGES[@]} -eq 0 ] ; then
    exitWithError "No packages are installed at $PTPKG_INSTALL_PREFIX"
  fi

  return 0
}

function showTestList()
{
  declare -a _LIST
  declare -a _TESTS
  local _PKG
  local _NAMEWIDTH=0

  for _PKG in ${PTPKG_INSTALLED_PACKAGES[@]} ; do
    if [ ${#_PKG} -gt $_NAMEWIDTH ] ; then
      _NAMEWIDTH=${#_PKG}
    fi
  done

  for _PKG in ${PTPKG_INSTALLED_PACKAGES[@]} ; do
    selectPackage "$_PKG"
    if [ -d "$PTPKG_PKG_TEST_DIR" ] ; then
      _TESTS=( `cd "$PTPKG_PKG_TEST_DIR" && ls *.py *.sh 2>/dev/null` )
    else
      _TESTS=( )
    fi
    case "$PTPKG_LIST_FLAG" in
    all)
      printf "  %${_NAMEWIDTH}s | "  "$_PKG"
      echo ${_TESTS[@]}
      ;;
    with)
      if [ ${#_TESTS[@]} -gt 0 ] ; then
        printf "  %${_NAMEWIDTH}s | "  "$_PKG"
        echo ${_TESTS[@]}
      fi
      ;;
    without)
      if [ ${#_TESTS[@]} -eq 0 ] ; then
        printf "  %${_NAMEWIDTH}s |\n"  "$_PKG"
      fi
      ;;
    *)
      exitWithError "Unexpected value for PTPKG_LIST_FLAG: $PTPKG_LIST_FLAG"
      ;;
    esac
  done
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

if [ -n "$PTPKG_LIST_FLAG" ] ; then
  showTestList
  exit 0
fi

# Allow the user to customize the test suite
case "$PTPKG_CUSTOMIZE_FLAG" in
  yes)
    getPackageSelections PTPKG_INSTALLED_PACKAGES PTPKG_INSTALLED_PACKAGES
    ;;
  no)
    PTPKG_PACKAGE_SELECTION=( ${PTPKG_INSTALLED_PACKAGES[@]} )
    ;;
  *)
    if promptYesNo "Would you like select the packages to be tested?" ; then
      getPackageSelections PTPKG_INSTALLED_PACKAGES PTPKG_INSTALLED_PACKAGES
    else
      PTPKG_PACKAGE_SELECTION=( ${PTPKG_INSTALLED_PACKAGES[@]} )
    fi
    ;;
esac

# All packages must be selected so dependencies are available
PTPKG_TEST_SELECTION=( ${PTPKG_PACKAGE_SELECTION[@]} )
PTPKG_PACKAGE_SELECTION=( ${PTPKG_INSTALLED_PACKAGES[@]} )
echoAll "The following packages will be tested" ${PTPKG_TEST_SELECTION[@]}

# Show work to be done
echo
hline
echo "  Host:   $PTPKG_HOST"
echo "  Target: $PTPKG_TARGET"
echo
echo "  Install directory: $PTPKG_INSTALL_PREFIX"
hline
echo
promptYesNo "Do you wish to continue?" || exit 0

# Create logfile directory
mkdir -p "$PTPKG_LOGFILE_PREFIX" || return 1

# Start proceedure timer
PTPKG_START_TIME=`date +%s`

# Install each package in the installation
for pkg in ${PTPKG_TEST_SELECTION[@]} ; do
  selectPackage "$pkg" || exitWithError "$pkg: Invalid package"
  execPackageTests
done

# Stop proceedure timer
PTPKG_STOP_TIME=`date +%s`
((PTPKG_SEC=$PTPKG_STOP_TIME - $PTPKG_START_TIME))

# Show status in a nice table

_FLAGWIDTH=0
_DESCWIDTH=0
_NAMEWIDTH=0
for _STATUS in "${PTPKG_TEST_STATUS[@]}"; do
  _FLAG="`echo $_STATUS | cut -d: -f1`"
  _DESC="`echo $_STATUS | cut -d: -f2`"
  _NAME="`echo $_STATUS | cut -d: -f3-`"

  if [ ${#_FLAG} -gt $_FLAGWIDTH ] ; then
    _FLAGWIDTH=${#_FLAG}
  fi
  if [ ${#_DESC} -gt $_DESCWIDTH ] ; then
    _DESCWIDTH=${#_DESC}
  fi
  if [ ${#_NAME} -gt $_NAMEWIDTH ] ; then
    _NAMEWIDTH=${#_NAME}
  fi
done

_TMPFILE=`newTmpfile`
(
  for _STATUS in "${PTPKG_TEST_STATUS[@]}"; do
    _FLAG="`echo $_STATUS | cut -d: -f1`"
    _DESC="`echo $_STATUS | cut -d: -f2`"
    _NAME="`echo $_STATUS | cut -d: -f3-`"

    printf "  %${_FLAGWIDTH}s | %${_NAMEWIDTH}s | %${_DESCWIDTH}s\n" "$_FLAG" "$_NAME" "$_DESC"
  done
) > $_TMPFILE

initLogfile "$PTPKG_INSTALL_PREFIX/test_report" "Test Suite Summary Report"
(
  echo
  hline
  echo
  sort $_TMPFILE
  echo "${#PTPKG_TEST_STATUS[@]} tests executed in $PTPKG_SEC seconds"
  echo
  hline
  echo
) | tee -a "$PTPKG_LOGFILE"

################################################################################
#
# END test action script
#
################################################################################

# EOF
