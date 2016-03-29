#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: ptpkg.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    ptpkg loader.  "source" this file to use ptpkg.
#

# PTPKG Version
export PTPKG_VERSION="0.1"

# Valid actions
declare -a PTPKG_ACTIONS
declare -a PTPKG_ACTION_SCRIPTS

# Scratch files
declare -a PTPKG_ALL_TMPFILE

# Default build location
export PTPKG_BUILD_PREFIX="/tmp/$USER/ptpkg/build"

################################################################################
#
# Function definitions
#
################################################################################

#
# Loads release information from $PTPKG_HOME/release, if it exists.
# Arguments: none
# Returns: none
#
function loadReleaseInfo {
  if [ -d "$PTPKG_HOME/release" ] ; then
    export PTPKG_RELEASE_DIR="$PTPKG_HOME/release"
  else
    export PTPKG_RELEASE_DIR=""
    return
  fi

  if [ -r "$PTPKG_RELEASE_DIR/information" ] ; then
    source "$PTPKG_RELEASE_DIR/information"
    export PTPKG_RELEASE_NAME="$NAME"
    export PTPKG_RELEASE_VERSION="$VERSION"
    export PTPKG_RELEASE_DESC_SHORT="$DESCRIPT_SHORT"
    export PTPKG_RELEASE_DESC_LONG="$DESCRIPT_LONG"
  else
    export PTPKG_RELEASE_NAME="(no name)"
    export PTPKG_RELEASE_VERSION="0.0"
    export PTPKG_RELEASE_DESC_SHORT="No description."
    export PTPKG_RELEASE_DESC_LONG="No description."
  fi

  #
  # Release environment file is processed later.
  #
}

#
# Sets global paths for building and installing files
# Arguments: $1: installation destination directory
# Returns: 0 if the prefix was set successfully, 1 otherwise
#
function setPrefix {
  local _PREFIX=`echo "$1"`
  if [ ! -d "$_PREFIX" ] ; then
    # Goofy way of cleaning up path without leaving empty directories
    mkdir -p "$_PREFIX" || return 1
    pushd "$_PREFIX" > /dev/null
    export PTPKG_PREFIX=`pwd`
    popd > /dev/null
    if ! rmdir "$_PREFIX" ; then
      echo "WARNING: Created $_PREFIX directory but couldn't remove it"
      return 1
    fi
  else 
    export PTPKG_PREFIX=`cd "$_PREFIX" && pwd`
  fi

  # Prefix must be an absolute pathname
  if ! isAbsolutePath "$PTPKG_PREFIX" ; then
    echo "ERROR: $PTPKG_PREFIX is not an absolute path"
    return 1
  fi

  # A target must be selected
  if [ -z "$PTPKG_TARGET" ] ; then
    echo "ERROR: You must call selectTarget before calling setPrefix"
    return 1
  fi

  # Set file prefixes
  export PTPKG_ETCFILE_PREFIX="$PTPKG_PREFIX/etc"
  export PTPKG_EXTERNAL_PREFIX="$PTPKG_PREFIX/external"
  export PTPKG_INSTALL_PREFIX="$PTPKG_PREFIX/$PTPKG_TARGET"

  # Load release environment
  if [ -r "$PTPKG_RELEASE_DIR/environment" ] ; then
    export PTPKG_RELEASE_ENV_FILE="$PTPKG_RELEASE_DIR/environment"
  else
    export PTPKG_RELEASE_ENV_FILE=""
  fi

  return 0
}

#
# Initializes the selected target installation directory
# Arguments: none
# Returns: 0 if the target was initilized successfully, 1 otherwise
#
function initPrefix {
  # Create build and install prefix directories
  mkdir -p "$PTPKG_BUILD_PREFIX" || return 1
  mkdir -p "$PTPKG_INSTALL_PREFIX" || return 1
  mkdir -p "$PTPKG_ETCFILE_PREFIX" || return 1
  mkdir -p "$PTPKG_EXTERNAL_PREFIX" || return 1
  mkdir -p "$PTPKG_EXTERNAL_PREFIX/bin" || return 1
  mkdir -p "$PTPKG_EXTERNAL_PREFIX/include" || return 1
  mkdir -p "$PTPKG_EXTERNAL_PREFIX/lib" || return 1

  return 0
}

#
# Removes temporary files on exit
# Arguments: none
# Returns: none
#
function cleanup {
  rm -f ${PTPKG_ALL_TMPFILE[@]}
}

#
# Create a temporary file and echo its filename
# Arguments: none
# Returns: 0 if file was created, 1 otherwise
#
function newTmpfile {
  local _TMP="/tmp/$$.${#PTPKG_ALL_TMPFILE[@]}"
  touch "$_TMP" || exitWithError "Cannot create tmpfile: $_TMP"
  PTPKG_ALL_TMPFILE=( ${PTPKG_ALL_TMPFILE[@]} "$_TMP" )
  echo "$_TMP"
}

################################################################################
#
# BEGIN PTPKG initialization
#
################################################################################

# Call cleanup on exit
trap cleanup EXIT SIGTERM SIGKILL SIGQUIT

# Attempt to set PTPKG_HOME to the directory containing this script
if [ -z "$PTPKG_HOME" ] ; then
  export PTPKG_HOME=`cd ${0%/*} && pwd -P`
fi

# Check PTPKG_HOME
if ! [ -d "$PTPKG_HOME" ] ; then
  echo "ERROR: PTPKG_HOME is not set to a valid directory."
  exit 1
fi

# Set and check PTPKG_SRC_DIR
export PTPKG_SRC_DIR="$PTPKG_HOME/ptpkg"
if ! [ -d "$PTPKG_SRC_DIR" ] ; then
  echo "ERROR: PTPKG_HOME does not contain a valid ptpkg directory."
  exit 1
fi

# Load the loader
source $PTPKG_SRC_DIR/loader.bash

# Add ptpkg to loader paths
loader_addpath "$PTPKG_SRC_DIR"

# Include ptpkg files
include common.sh
include targets.sh
include packages.sh
include logging.sh
include environment.sh
include testing.sh

# Remove loader from shellspace
loader_finish

# Load release information
loadReleaseInfo

# Initialize actions
PTPKG_ACTION_SCRIPTS=( `cd "$PTPKG_SRC_DIR" && ls action_*.sh` )
PTPKG_ACTIONS=( `cd "$PTPKG_SRC_DIR" && ls action_*.sh | cut -d_ -f2- | cut -d. -f1` )

################################################################################
#
# END PTPKG initialization
#
################################################################################
# EOF
