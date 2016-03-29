#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: action_extract_target.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Extracts a target configuration from the all-in-one distro
#

################################################################################
#
# Default configuration
#
################################################################################

PTPKG_PREFIX_FLAG="ask"
PTPKG_TARGET_FLAG="ask"
PTPKG_CUSTOMIZE_FLAG="no"

################################################################################
#
# Function definitions
#
################################################################################

#
# Initializes extraction action.
# Sets PTPKG_CONFIG_* flags according to the arguments.
# Arguments:
# Returns: 0 if PTPKG was initilized successfully, 1 otherwise
#
function initExtraction {
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
        echo " ** Extract target command parameters **"
        echo "  The first option is the default when the parameter is omitted"
        echo "  The option in ALL-CAPS is the default when the parameter value is omitted"
        echo
        echo "  --prefix=(ask|<path>)         Installation root directory"
        echo "  --target=(ask|<target>)       Specify target configuration"
        echo "  --customize[=(no|YES|ask)]    Answer to \"Customize installation?\" prompt"
        echo "  --help                        Show this help message"
        echo
        echo " ** Examples **"
        echo
        echo "  configure.sh extract_target:"
        echo "     prefix=ask, target=ask, customize=no"
        echo
        echo "  configure.sh extract_target --prefix=<path> --target=aix-5.3.0.0-ibm-ppc64"
        echo "     prefix=<path>, target=aix-5.3.0.0-ibm-ppc64, customize=no"
        echo
        echo "  configure.sh extract_target --customize --target=linux-suse11cray-gnu-x86_64"
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

  # Get target selection from user
  if [ $PTPKG_TARGET_FLAG == ask ] ; then
    banner "Select the target configuration you wish to extract"
    select _TARG in `ls "$PTPKG_TARGETS_DIR"` ; do
      selectTarget "$_TARG" || \
	  exitWithError "Invalid target: $_TARG"
      break
    done
  else
    selectTarget "$PTPKG_TARGET_FLAG" || \
	exitWithError "Invalid target: $PTPKG_TARGET_FLAG"
  fi

  echo "Selected target configuration: $PTPKG_TARGET"

  # Discover available packages
  echo "Loading packages database..."
  getPackages

  return 0
}

#
# Extracts a specific target configuration from the selected package
# Arguments: none
# Returns: 0 if no errors occured, 1 otherwise
#
function extractPackage {
  local _DEST="$PTPKG_PKG_INSTALL_DIR/$PTPKG_TARGET"

  PTPKG_SCRIPT_FLAG="extract"
  _START_TIME=`date +%s`
  banner "${PTPKG_PKG}: Extracting for target $PTPKG_TARGET"

  copyPackage "$PTPKG_PKG_INSTALL_DIR" "include_archive" || return 1

  _STOP_TIME=`date +%s`
  ((_SEC=$_STOP_TIME - $_START_TIME))

  echo "Extraction complete in $_SEC seconds"

  # Read subshell exit code to get return code
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
initExtraction $@ || exit 1

# Allow the user to customize the extraction
while true ; do
  echoAll "The following packages and their dependencies will be extracted" ${PTPKG_DEFAULT_PACKAGES[@]}
  case "$PTPKG_CUSTOMIZE_FLAG" in
    yes)
      getPackageSelections
      ;;
    no)
      PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      ;;
    *)
      if promptYesNo "Would you like to customize the extraction?" ; then
        getPackageSelections
      else
        PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      fi
      ;;
  esac
  buildInstallation && break
  echo "The set of packages you have selected cannot be extracted."
  promptYesNo "Would you like to select a different set of packages?" || \
      exitWithError "Installation cannot continue.  Exiting..."
done

# Show target configuration
if [ ${#PTPKG_TARGET_PROVIDES[@]} -ne 0 ] ; then
  echoAll "Packages provided by $PTPKG_TARGET" ${PTPKG_TARGET_PROVIDES[@]}
fi
if [ ${#PTPKG_TARGET_REQUIRES[@]} -ne 0 ] ; then
  echoAll "Packages required by $PTPKG_TARGET" ${PTPKG_TARGET_REQUIRES[@]}
fi
if [ ${#PTPKG_TARGET_EXCLUDES[@]} -ne 0 ] ; then
  echoAll "Packages excluded by $PTPKG_TARGET" ${PTPKG_TARGET_EXCLUDES[@]}
fi

# Show required dependencies
if [ ${#PTPKG_DEPENDENCIES[@]} -gt 0 ] ; then
  echoAll "Required dependencies" ${PTPKG_DEPENDENCIES[@]}
fi

# Show extraction list
echoAll "Packages to be extracted, in order of extraction" ${PTPKG_INSTALLATION[@]}
echo

# Get installation destination from user
if [ "$PTPKG_PREFIX_FLAG" == "ask" ] ; then
  while true ; do
    promptForDir "Please enter the extraction destination directory" \
                "${PTPKG_HOME}/${PTPKG_TARGET}" "allow_nonexistant"
    setPrefix "$PTPKG_PROMPTFORDIR_RESULT" && break
  done
else
  setPrefix "$PTPKG_PREFIX_FLAG" || \
      exitWithError "Invalid path specified on command line: $PTPKG_PREFIX_FLAG"
fi

# Adjust install prefix so packages are "installed" to packages folder
PTPKG_INSTALL_PREFIX="$PTPKG_PREFIX/packages"

# Show work to be done
echo
hline
echo "  Host:   $PTPKG_HOST"
echo "  Target: $PTPKG_TARGET"
echo "  Extraction directory: $PTPKG_INSTALL_PREFIX"
hline
echo
promptYesNo "Do you wish to continue?" || exit 0

# Initialize installation destination
initPrefix || exitWithError "Error: Cannot initilize $PTPKG_PREFIX"

# Copy PTPKG
mkdir -p "$PTPKG_PREFIX"
cp -R "$PTPKG_HOME/ptpkg" "$PTPKG_PREFIX"

# Copy release files
if [ -d "$PTPKG_HOME/release" ] ; then
  cp -R "$PTPKG_HOME/release" "$PTPKG_PREFIX"
fi

# Copy the target configuration
mkdir -p "$PTPKG_PREFIX/targets"
cp -R "$PTPKG_TARGETS_DIR/$PTPKG_TARGET" "$PTPKG_PREFIX/targets"

# Copy the default packages list
mkdir -p "$PTPKG_PREFIX/packages"
cp "$PTPKG_PACKAGES_DIR/.defaults" "$PTPKG_PREFIX/packages"

# Copy configure.sh
cp "$PTPKG_HOME/configure.sh" "$PTPKG_PREFIX"

# Start proceedure timer
PTPKG_START_TIME=`date +%s`

# Extract each package in the database
for pkg in ${PTPKG_INSTALLATION[@]} ; do
  selectPackage "${pkg}" || exitWithError "${pkg}: Invalid package"
  if ! extractPackage ; then
    promptYesNo "Package extraction failed. Continue?" || \
            exitWithError "Aborted due to package extraction failure"
  fi
done

# Stop proceedure timer
PTPKG_STOP_TIME=`date +%s`
((PTPKG_SEC=$PTPKG_STOP_TIME - $PTPKG_START_TIME))

echo "Target extraction completed in ${PTPKG_SEC} seconds"
echo

# EOF
