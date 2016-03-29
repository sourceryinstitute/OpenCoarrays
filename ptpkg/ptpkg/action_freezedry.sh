#!/bin/bash
#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: action_freezedry.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Creates a tarball from a compiled package with the absolute paths fixed.
#

################################################################################
#
# Default configuration
#
################################################################################

PTPKG_SOURCE_FLAG="ask"
PTPKG_DEST_FLAG="ask"
PTPKG_BUILD_PREFIX_FLAG="auto"
PTPKG_TARGET_FLAG="auto"
PTPKG_CUSTOMIZE_FLAG="yes"
PTPKG_OVERWRITE_FLAG="no"

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
        echo " ** Freezedry command parameters **"
        echo "  The first option is the default when the parameter is omitted"
        echo "  The option in ALL-CAPS is the default when the parameter value is omitted"
        echo
        echo "  --source=(ask|<path>)         Installation directory"
        echo "  --dest=(ask|<path>)           Repository directory"
        echo "  --build-prefix=(auto|<path>)  Build directory"
        echo "  --target=(auto|ask|<target>)  Specify target configuration"
        echo "  --customize[=(no|YES|ask)]    Answer to \"Customize installation?\" prompt"
        echo "  --help                        Show this help message"
        hline
        echo
        return 1
        ;;
      --source*)
        case "$_ARGVAL" in
          ask|ASK)
            PTPKG_SOURCE_FLAG="ask"
            ;;
          $1)
            echo "--source requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --source must be an absolute path."
              return 1
            fi
            PTPKG_SOURCE_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      --dest*)
        case "$_ARGVAL" in
          ask|ASK)
            PTPKG_DEST_FLAG="ask"
            ;;
          $1)
            echo "--dest requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --dest must be an absolute path."
              return 1
            fi
            PTPKG_DEST_FLAG="$_ARGVAL"
            ;;
        esac
        ;;
      --build-prefix*)
        case "$_ARGVAL" in
          auto|AUTO)
            # Value is already set by ptptkg.sh
            ;;
          $1)
            echo "--build-prefix requires a value.  Try '--help' for help."
            return 1
            ;;
          *)
            if ! isAbsolutePath "$_ARGVAL" ; then
              echo "The value for --build-prefix must be an absolute path."
              return 1
            fi
            export PTPKG_BUILD_PREFIX="$_ARGVAL"
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
      --overwrite*)
        case "$_ARGVAL" in
          no|NO)
            PTPKG_OVERWRITE_FLAG="no"
            ;;
          ask|ASK)
            PTPKG_OVERWRITE_FLAG="ask"
            ;;
          $1|yes|YES)
            PTPKG_OVERWRITE_FLAG="yes"
            ;;
          *)
            echo "Invalid value for --overwrite: $_ARGVAL.  Try '--help' for help."
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

  # Get source path
  if [ "$PTPKG_SOURCE_FLAG" == "ask" ] ; then
    while true ; do
      promptForDir "Please enter path to a $PTPKG_RELEASE_NAME installation" "$PTPKG_HOME"
      PTPKG_SOURCE_PREFIX="$PTPKG_PROMPTFORDIR_RESULT"
      isAbsolutePath "$PTPKG_SOURCE_PREFIX" && break
      echo "ERROR: $PTPKG_SOURCE_PREFIX is not an absolute path"
    done
  else
    PTPKG_SOURCE_PREFIX="$PTPKG_SOURCE_FLAG"
    if ! isAbsolutePath "$PTPKG_SOURCE_PREFIX" ; then
      echo "ERROR: Source '$PTPKG_SOURCE_PREFIX' is not an absolute path"
      exit 1
    fi
    if [ ! -d "$PTPKG_SOURCE_PREFIX" ] ; then
      echo "ERROR: Source '$PTPKG_SOURCE_PREFIX' doesn't exist or is not a directory"
      exit 1
    fi
  fi

  # Get destination path
  if [ "$PTPKG_DEST_FLAG" == "ask" ] ; then
    while true ; do
      promptForDir "Please enter the path where freezedried packages will be saved" "$HOME/${PTPKG_RELEASE_NAME}-${PTPKG_RELEASE_VERSION}" "allow_nonexistant"
      PTPKG_DEST_PREFIX="$PTPKG_PROMPTFORDIR_RESULT"
      isAbsolutePath "$PTPKG_DEST_PREFIX" && break
      echo "ERROR: $PTPKG_DEST_PREFIX is not an absolute path"
    done
  else
    PTPKG_DEST_PREFIX="$PTPKG_DEST_FLAG"
    if ! isAbsolutePath "$PTPKG_DEST_PREFIX" ; then
      echo "ERROR: Destination '$PTPKG_DEST_PREFIX' is not an absolute path"
      exit 1
    fi
  fi

  if [ "$PTPKG_SOURCE_PREFIX" == "$PTPKG_DEST_PREFIX" ] ; then
    exitWithError "Source and destination paths cannot be the same."
  fi

  # Discover installed target configurations
  export PTPKG_TARGETS_DIR="$PTPKG_SOURCE_PREFIX/targets"
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

  # Set the installation prefix
  setPrefix "$PTPKG_SOURCE_PREFIX" || \
      exitWithError "Invalid path specified on command line: $PTPKG_SOURCE_FLAG"

  # Discover available packages
  export PTPKG_PACKAGES_DIR="$PTPKG_SOURCE_PREFIX/packages"
  getPackages

  # Remove the target pseudo-package
  PTPKG_ALL_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]/$PTPKG_TARGET/} )

  # Remove the "no_freezedry" packages from PTPKG_ALL_PACKAGES
  #for pkg in ${PTPKG_ALL_PACKAGES[@]} ; do
  #  selectPackage "$pkg"
  #  if [ -d "$PTPKG_PKG_DIR/$PTPKG_TARGET" ] ; then
  #    if [ -e "$PTPKG_PKG_DIR/$PTPKG_TARGET/no_freezedry" ] ; then
  #      echo "---------- Excluding $pkg because of no_freezedry"
  #      PTPKG_ALL_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]/$pkg/} )
  #    elif ! [ -e "$PTPKG_PKG_RECEIPT" ] ; then
  #      echo "!!WARNING!! Excluding $pkg because of MISSING RECEIPT"
  #      PTPKG_ALL_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]/$pkg/} )
  #    fi
  #  else
  #    echo "$pkg: no target $PTPKG_TARGET"
  #  fi
  #done

  # Select all packages by default, excluding the target pseudo-package
  PTPKG_DEFAULT_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]} )

  return 0
}

################################################################################
#
# BEGIN freezedry action script
#
################################################################################

# Import common functionality
source `cd ${0%/*} && pwd -P`/ptpkg.sh

# Initialize action
initAction $@ || exit 1

# Allow the user to customize the installation
while true ; do
  echoAll "The following packages and their dependencies will be freezedried" ${PTPKG_DEFAULT_PACKAGES[@]}
  case "$PTPKG_CUSTOMIZE_FLAG" in
    yes)
      getPackageSelections
      ;;
    no)
      PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      ;;
    *)
      if promptYesNo "Would you like to select packages to freezedry?" ; then
        getPackageSelections
      else
        PTPKG_PACKAGE_SELECTION=( ${PTPKG_DEFAULT_PACKAGES[@]} )
      fi
      ;;
  esac
  buildInstallation && break
  echo "The set of packages you have selected cannot be freezedried."
  promptYesNo "Would you like to select a different set of packages?" || \
      exitWithError "Freezedry cannot continue.  Exiting..."
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

# Show installation list
echoAll "Packages to be freezedried" ${PTPKG_INSTALLATION[@]}
echo

# Show work to be done
echo
hline
echo "  Host:   $PTPKG_HOST"
echo "  Target: $PTPKG_TARGET"
echo
echo "  Source directory:   $PTPKG_SOURCE_PREFIX"
echo "  Destionation directory: $PTPKG_DEST_PREFIX"
hline
echo
promptYesNo "Do you wish to continue?" || exit 0

# Start proceedure timer
PTPKG_START_TIME=`date +%s`

# Create destination
mkdir -p "$PTPKG_DEST_PREFIX"

# Freezedry each package
for pkg in ${PTPKG_INSTALLATION[@]} ; do
  selectPackage "$pkg" || exitWithError "$pkg: Invalid package"
  if ! execPackageFreezedry ; then
    promptYesNo "Freezedry failed. Continue?" || exitWithError "Aborted due to failure"
  fi
done

# Copy release files
if [ ! -d "$PTPKG_DEST_PREFIX/release" ] ; then
  echo "Copying release files..."
  cp -R "$PTPKG_HOME/release" "$PTPKG_DEST_PREFIX"
fi

# Install target configuration files
if [ ! -d "$PTPKG_DEST_PREFIX/targets" ] ; then
  echo "Copying target description..."
  mkdir -p "$PTPKG_DEST_PREFIX/targets"
  cp -R "$PTPKG_TARGET_DIR" "$PTPKG_DEST_PREFIX/targets/$PTPKG_TARGET"
fi

# Install PTPKG and configure.sh
if [ ! -d "$PTPKG_DEST_PREFIX/ptpkg" ] ; then
  echo "Copying ptpkg...."
  cp -R "$PTPKG_HOME/ptpkg" "$PTPKG_DEST_PREFIX"
fi
if [ ! -f "$PTPKG_DEST_PREFIX/configure.sh" ] ; then
  echo "Copying configure.sh"
  cp "$PTPKG_HOME/configure.sh" "$PTPKG_DEST_PREFIX"
fi

# Activate disabled scripts
if [ -d "$PTPKG_DEST_PREFIX/ptpkg/.disabled" ] ; then
  pushd "$PTPKG_DEST_PREFIX/ptpkg"
  mv .disabled/* .
  rmdir .disabled
fi

# Create .defaults file
for pkg in ${PTPKG_PACKAGE_SELECTION[@]} ; do
  echo "$pkg" >> "$PTPKG_DEST_PREFIX/packages/.defaults"
done

# Stop proceedure timer
PTPKG_STOP_TIME=`date +%s`
((PTPKG_SEC=$PTPKG_STOP_TIME - $PTPKG_START_TIME))

echo "Freezedry completed in ${PTPKG_SEC} seconds"
echo

################################################################################
#
# END action script
#
################################################################################

# EOF
