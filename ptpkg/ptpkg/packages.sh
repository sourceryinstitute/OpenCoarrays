#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: packages.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Package management functions
#

# Use shell script loader
include menus.sh
include targets.sh
include logging.sh

# PTPKG packages directory
PTPKG_PACKAGES_DIR="$PTPKG_HOME/packages"

# Number of parallel make jobs (automatically determined)
PTPKG_MAKE_JOBS=1

# Flag to control script function execution
PTPKG_SCRIPT_FLAG=""

# Package configuration
PTPKG_PKG=""
PTPKG_PKG_NAME=""
PTPKG_PKG_VERSION=""
PTPKG_PKG_DIR=""
PTPKG_PKG_BUILD_DIR=""
PTPKG_PKG_INSTALL_DIR=""
PTPKG_PKG_TEST_DIR=""

# Package source 
PTPKG_PKG_ARCHIVE=""

# Package dependencies
declare -a PTPKG_PKG_PROVIDES
declare -a PTPKG_PKG_REQUIRES
declare -a PTPKG_PKG_EXCLUDES

# Selected packages
declare -a PTPKG_PACKAGE_SELECTION

# Available packages
# Also, master index for forward lookup
declare -a PTPKG_ALL_PACKAGES

# Available features
# Also master index for reverse lookup
declare -a PTPKG_ALL_FEATURES

# Package --> data forward lookup
declare -a PTPKG_PACKAGE_DATA

# Package --> dependency info forward lookup
declare -a PTPKG_PROVIDED_BY_PACKAGE
declare -a PTPKG_REQUIRED_BY_PACKAGE
declare -a PTPKG_EXCLUDED_BY_PACKAGE

# Packages <-- dependency info reverse lookup
declare -a PTPKG_PACKAGES_PROVIDING

# Default packages
declare -a PTPKG_DEFAULT_PACKAGES

# Package installation status lists
declare -a PTPKG_INSTALLED_PACKAGES
declare -a PTPKG_SKIPPED_PACKAGES
declare -a PTPKG_ABORTED_PACKAGES

# Packages that were uninstalled
declare -a PTPKG_UNINSTALLED_PACKAGES

# Packages installed for dependies
declare -a PTPKG_DEPENDENCIES

# All packages to be installed, in order of installation
declare -a PTPKG_INSTALLATION

# These variables are not local, despite underscore prefix.
# They are used as local variables and cleared after use, so
# they behave something like local variables, but they are also
# visible across recursive function calls
declare -a _DEPENDENCIES
declare -a _VISITED
# TODO: Show which package triggered the dependency
declare -a _REQUIRED_BY 

#
# Returns the database index of the specified package
# Arguments: $1: Package to get index of
# Returns: Package index, or 1+npkgs if the package is not in the database
#
function getPackageIndex {
  local i
  for ((i=0; i<${#PTPKG_ALL_PACKAGES[@]}; i++)) {
    [ "${PTPKG_ALL_PACKAGES[$i]}" == "$1" ] && return $i
  }
  return $i
}

#
# Sets global package variables
# Arguments: $1: package to select
# Returns: 1 if the package cannot be selected, 0 otherwise
#
function selectPackage {
  [ -z "$1" ] && return 0

  local _IDX
  local _PKG
  if [[ $1 == ${1//[^0-9]/} ]] ; then
    _IDX=$1
    _PKG="${PTPKG_ALL_PACKAGES[$_IDX]}"
  else
    getPackageIndex "$1"
    _IDX=$?
    _PKG="$1"
  fi

  if ! checkLimits 0 $_IDX ${#PTPKG_ALL_PACKAGES[@]} ; then
    echo "$_PKG: Invalid database index: '$_IDX'"
    return 1
  fi

  # Get package data
  declare -a _DATA=( ${PTPKG_PACKAGE_DATA[$_IDX]} )

  # Set global variables from package name.
  # Different behavior if this is the target pseudo-package
  if [ "$_PKG" == "$PTPKG_TARGET" ] ; then
    PTPKG_PKG="$PTPKG_TARGET"
    PTPKG_PKG_NAME="$PTPKG_TARGET"
    PTPKG_PKG_VERSION="$PTPKG_TARGET"
    PTPKG_PKG_BUILD_DIR="$PTPKG_BUILD_PREFIX"
    PTPKG_PKG_INSTALL_DIR="$PTPKG_INSTALL_PREFIX"
    PTPKG_PKG_TEST_DIR="$PTPKG_TARGET_DIR/test"
    PTPKG_PKG_DIR="$PTPKG_TARGET_DIR"
    PTPKG_PKG_RECEIPT=""
  else
    PTPKG_PKG="$_PKG"
    PTPKG_PKG_NAME="${_PKG%-*}"  # Everything before the last '-'
    PTPKG_PKG_VERSION="${_PKG##*-}" # Everything after the last '-'
    PTPKG_PKG_BUILD_DIR="$PTPKG_BUILD_PREFIX/$_PKG"
    PTPKG_PKG_INSTALL_DIR="$PTPKG_INSTALL_PREFIX"
    PTPKG_PKG_TEST_DIR="$PTPKG_PACKAGES_DIR/$_PKG/test"
    PTPKG_PKG_DIR="$PTPKG_PACKAGES_DIR/$_PKG"
    PTPKG_PKG_RECEIPT="$PTPKG_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET/receipt"
  fi

  # Set global variables from package data
  PTPKG_PKG_SOURCE_FILE="$PTPKG_PKG_DIR/${_DATA[0]}"
  PTPKG_PKG_DEPENDS_FILE="$PTPKG_PKG_DIR/${_DATA[1]}"
  PTPKG_PKG_INSTALL_FILE="$PTPKG_PKG_DIR/${_DATA[2]}"
  PTPKG_PKG_UNINSTALL_FILE="$PTPKG_PKG_DIR/${_DATA[3]}"
  PTPKG_PKG_ENV_FILE="$PTPKG_PKG_DIR/${_DATA[4]}"
  PTPKG_PKG_TEST_DIR="$PTPKG_PKG_DIR/${_DATA[5]}"
  PTPKG_PKG_NO_FREEZEDRY="$PTPKG_PKG_DIR/${_DATA[6]}"

  # Get package dependency information
  PTPKG_PKG_PROVIDES=( ${PTPKG_PROVIDED_BY_PACKAGE[$_IDX]} )
  PTPKG_PKG_REQUIRES=( ${PTPKG_REQUIRED_BY_PACKAGE[$_IDX]} )
  PTPKG_PKG_EXCLUDES=( ${PTPKG_EXCLUDED_BY_PACKAGE[$_IDX]} )

  # Get package source information
  if [ "$_PKG" != "$PTPKG_TARGET" ] ; then
    source "$PTPKG_PKG_SOURCE_FILE"
    PTPKG_PKG_ARCHIVE="$FILE"
  else
    PTPKG_PKG_ARCHIVE=""
  fi

  return 0
}

#
# Extracts the selected package archive file into PTPKG_BUILD_PREIFX
# Arguments: none
# Returns: 0 if no problems were encountered, 1 otherwise
#
function pkgUnpack {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  # Remove old files if needed
  if [ -d "$PTPKG_PKG_BUILD_DIR" ] ; then
      echo "Removing $PTPKG_PKG_BUILD_DIR"
      rm -rf "$PTPKG_PKG_BUILD_DIR"
  fi
  mkdir -p "$PTPKG_BUILD_PREFIX"

  # Unpack the archive file.  This creates $PTPKG_PKG_BUILD_DIR
  unpack "$PTPKG_PKG_DIR/$PTPKG_PKG_ARCHIVE" "$PTPKG_BUILD_PREFIX" || pkgAbort

  # Change directory
  cd "$PTPKG_PKG_BUILD_DIR" || pkgAbort

  return 0
}

#
# Unpacks the package archive directly to the installation directory
# Arguments: none
# Returns: 0 if no problems were encountered, 1 otherwise
#
function pkgUnpackToInstall {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  unpack "$PTPKG_PKG_DIR/$PTPKG_PKG_ARCHIVE" "$PTPKG_PKG_INSTALL_DIR" || pkgAbort
  return 0
}

#
# Change to a directory and execute a command
# Arguments: $1: Directory to execute in,
#            $2: Command to execute,
#            $3...: Command arguments
# Returns: Command's exit status code
#
function pkgExecInDir {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  local _DIR="$1"
  local _CMD="$2"
  local _RETVAL=127
  shift
  shift

  echo "${PTPKG_PKG}: Executing '$_CMD $@' in '$_DIR'"

  if ! cd "${_DIR}" ; then
    pkgAbort "Cannot enter directory: $_DIR"
    return 1
  fi

  $_CMD "$@"
  _RETVAL=$?
  if [ $_RETVAL -ne 0 ] ; then
    pkgAbort "Nonzero exit code from '$_CMD $@' in $_DIR"
  fi
  return $_RETVAL
}

#
# Executes a command as part of a package script
# Arguments: $1: Command to execute, $2...: Command arguments
# Returns: Command's exit status code
#
function pkgExec {
  pkgExecInDir "${PTPKG_PKG_BUILD_DIR}" "$@"
  return $?
}

#
# Executes package configure script.
# Arguments: $1...: Arguments passed directly to configure script
# Returns: Command's exit status code
#
function pkgConfigure {
  pkgExec ./configure "$@"
  return $?
}


#
# Executes a parallel make
# Arguments: $1...: Arguments passed directly to make
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgMakeParallel {
  pkgExec $PTPKG_MAKE_CMD -j$PTPKG_MAKE_JOBS "$@"
  return $?
}

#
# Executes a serial make
# Arguments: $1...: Arguments passed directly to make
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgMake {
  pkgExec $PTPKG_MAKE_CMD "$@"
  return $?
}

#
# Executes package python setup
# Arguments: $1...: Arguments passed directly to python setup script
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgSetupPy {
  pkgExec python setup.py "$@"
  return $?
}

#
# Uses pip to install a package
# Arguments: $1...: Arguments passed to pip
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgPipInstall {
  pip="pip -v install -M -I -b $PTPKG_PKG_BUILD_DIR $@"
  # First try to download from pypy.  Subshell to protect against failure
  ( pkgExecInDir "$PTPKG_BUILD_PREFIX" $pip "$PTPKG_PKG_NAME>=$PTPKG_PKG_VERSION" )
  retval=$?
  if [ $retval -ne 0 ] ; then
    pkgExecInDir "$PTPKG_BUILD_PREFIX" $pip --no-deps "$PTPKG_PKG_DIR/$PTPKG_PKG_ARCHIVE" 
    retval=$?
  fi
  return $retval
}

#
# Replaces "PTPKG_INSTALL_PREFIX" with the value of "$PTPKG_INSTALL_PREFIX"
# in all arguments
# Arguments: $1...: Files to process, relative to $PTPKG_PKG_INSTALL_DIR
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgFixAbsolutePaths {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  local file

  cd "$PTPKG_PKG_INSTALL_DIR"
  for file in $@ ; do
    echo "Fixing paths in $file"
    $PTPKG_SED_CMD -i -e "s#PTPKG_INSTALL_PREFIX#$PTPKG_INSTALL_PREFIX#g" "$file" || pkgAbort
  done
  return 0
}

#
# Applies a patch to the unpacked package source
# Arguments: $1: patch file, $2...: Arguments to patch command
# Returns: 0 if no problems encountered, 1 otherwise
#
function pkgPatch {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  local _PATCHFILE="${PTPKG_PKG_DIR}/$1"
  if ! [ -r "${_PATCHFILE}" ] ; then
    pkgAbort "Cannot read patch file ${_PATCHFILE}"
    return 1
  fi
  shift

  echo "${PTPKG_PKG}: Executing 'patch $@ < ${_PATCHFILE}' in '${PTPKG_PKG_BUILD_DIR}'"

  if ! cd "${PTPKG_PKG_BUILD_DIR}" ; then
    pkgAbort "Cannot enter directory: ${PTPKG_PKG_BUILD_DIR}"
    return 1
  fi

  patch "$@" < "${_PATCHFILE}"
  local _RETVAL=$?
  if [ $_RETVAL -ne 0 ] ; then
    pkgAbort "Nonzero exit code from 'patch $@ < ${_PATCHFILE}' in '${PTPKG_PKG_BUILD_DIR}'"
  fi
  return $_RETVAL
}

#
# Sets the script flag to "abort"
# Arguments: $1: Optional message saying why aborted
# Returns: none
#
function pkgAbort {
  PTPKG_SCRIPT_FLAG="abort"
  if ! [ -z "$1" ] ; then
    echo "ABORTING $PTPKG_PKG: $1"
  fi
}

#
# Echos the full path to the directory that a package is installed in.
# If the package is not installed, the empty string is echoed.
# Arguments: $1: Feature or package name
# Returns: none
#
function pkgGetInstallDir {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  if resolveFeature "$1" ; then
    local pkg="$PTPKG_RESOLVEFEATURE_RESULT"
    if pkgIsInstalled "$pkg" ; then
      ( # Execute in subshell to preserve package selection in parent shell
        selectPackage "$pkg"
        echo $PTPKG_PKG_INSTALL_DIR
      )
      return
    fi
  fi
  echo ""
}

#
# Determines if the selected package has already been installed in the target directory
# Arguments: $1: name of package to check
# Returns: 0 if the package is already installed, 1 otherwise
#
function pkgIsInstalled {
  [ "$PTPKG_SCRIPT_FLAG" == "skip" ] && return 0
  [ "$PTPKG_SCRIPT_FLAG" == "abort" ] && return 1

  local _PKG="$1"
  ( # Select package in subshell to preserve calling shell's environment
    selectPackage $_PKG
    [ -f "$PTPKG_PKG_RECEIPT" ] && exit 0
    exit 1
  ) && return 0
  return 1
}

#
# Executes the appropriate installation script for the selected package
# Arguments: none
# Returns: 0 if the installation executed successfully or was skipped
#          1 if the installation was aborted or status is unknown
#
function execPackageInstall {
  initPackageLogfile "install" "$PTPKG_PKG INSTALLATION LOG"
  PTPKG_SCRIPT_FLAG="install"

  # Installation executes in a subshell to protect against
  # envrionment pollution in the parent script and to facilitate logging
  (
    local logfile="$PTPKG_LOGFILE"

    # Check if package is already installed
    if pkgIsInstalled "$PTPKG_PKG" ; then
      case "$PTPKG_REINSTALL_FLAG" in
        y*|Y*)
          execPackageUninstall
          PTPKG_LOGFILE="$logfile"
          # Continue with installation
          ;;
        n*|N*)
          # Skip installation. Just exits subshell, not parent
          echo "$PTPKG_PKG is installed."
          exit 1
          ;;
        *)
          if promptYesNo "$PTPKG_PKG is installed.  Reinstall?" ; then
            execPackageUninstall
            PTPKG_LOGFILE="$logfile"
          else
            # Skip installation. Just exits subshell, not parent
            echo "$PTPKG_PKG is installed."
            exit 1
          fi
          ;;
      esac
    fi

    # Mark installation beginning
    _START_TIME=`date +%s`

    echo
    hline
    echo "  Installing    : $PTPKG_PKG"
    echo "  Install script: $PTPKG_PKG_INSTALL_FILE"
    echo "  Archive file  : $PTPKG_PKG_ARCHIVE"
    echo "  Provides      : ${PTPKG_PKG_PROVIDES[@]}"
    echo "  Requires      : ${PTPKG_PKG_REQUIRES[@]}"
    echo "  Excludes      : ${PTPKG_PKG_EXCLUDES[@]}"
    echo "  Env. file     : $PTPKG_PKG_ENV_FILE"
    hline
    echo

    # Add package dependencies to the environment
    bootstrapPackageDeps || \
        exitWithError "$PTPKG_PKG: Dependency packages could not be bootstraped." 2

    # Execute the installation script
    source "$PTPKG_PKG_INSTALL_FILE" || \
        exitWithError "$PTPKG_PKG: Installation script could not be executed." 2

    # If still installing at this point, copy package metafiles
    if [ "$PTPKG_SCRIPT_FLAG" == "install" ] ; then
      # Also copy package archive files if package can't be freezedried
      if [ -e "$PTPKG_PKG_NO_FREEZEDRY" ] ; then
        copyPackage "$PTPKG_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET" "include_source"
      else
        copyPackage "$PTPKG_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET"
      fi
      echo "Writing package receipt..."
      find "$PTPKG_INSTALL_PREFIX" | sort > "$PTPKG_HOME/.receipt1" || exit 2
      comm -3 "$PTPKG_HOME/.receipt0" "$PTPKG_HOME/.receipt1" > "$PTPKG_PKG_RECEIPT" || exit 2
      mv "$PTPKG_HOME/.receipt1" "$PTPKG_HOME/.receipt0" || exit 2
      nfiles=`cat "$PTPKG_PKG_RECEIPT" | wc -l`
      echo "$PTPKG_PKG: $nfiles files installed"
    fi

    # Mark installation end
    _STOP_TIME=`date +%s`
    ((_INSTALL_SEC=$_STOP_TIME - $_START_TIME))

    # Interpret results
    case "$PTPKG_SCRIPT_FLAG" in
      "install")
        rm -rf $PTPKG_PKG_BUILD_DIR
        banner "$PTPKG_PKG: Installation complete in $_INSTALL_SEC seconds"
        exit 0  # Just exits subshell, not parent
        ;;
      "skip")
        banner "$PTPKG_PKG: Installation skipped"
        exit 1  # Just exits subshell, not parent
        ;;
      "abort")
        banner "$PTPKG_PKG: Installation aborted after $_INSTALL_SEC seconds"
        exit 2  # Just exits subshell, not parent
        ;;
      *)
        banner "$PTPKG_PKG: Installation status unknown"
        exit 100
        ;;
    esac
  ) 2>&1 | tee $PTPKG_LOGFILE

  # Read subshell exit code to set flags and return code
  case ${PIPESTATUS[0]} in
    0)
      PTPKG_SCRIPT_FLAG="install"
      PTPKG_INSTALLED_PACKAGES=( ${PTPKG_INSTALLED_PACKAGES[@]} "$PTPKG_PKG" )
      return 0
      ;;
    1)
      PTPKG_SCRIPT_FLAG="skip"
      PTPKG_SKIPPED_PACKAGES=( ${PTPKG_SKIPPED_PACKAGES[@]} "$PTPKG_PKG" )
      rm -f ${PTPKG_LOGFILE}
      return 0
      ;;
    2)
      PTPKG_SCRIPT_FLAG="abort"
      PTPKG_ABORTED_PACKAGES=( ${PTPKG_ABORTED_PACKAGES[@]} "$PTPKG_PKG" )
      return 1
      ;;
    *)
      PTPKG_SCRIPT_FLAG="unknown"
      return 1
      ;;
    esac
}


#
# Executes the appropriate uninstallation script for the selected package
# Arguments: none
# Returns: 0 if the uninstallation executed successfully, 1 otherwise
#
function execPackageUninstall {
  initPackageLogfile "uninstall" "$PTPKG_PKG UNINSTALLATION LOG"
  PTPKG_SCRIPT_FLAG="uninstall"

  # Uninstallation executes in a subshell to protect against
  # envrionment pollution in the parent script and to facilitate logging
  (
    _START_TIME=`date +%s`
    banner "$PTPKG_PKG: Uninstalling"

    # Delete all files in the receipt
    cat "$PTPKG_PKG_RECEIPT" | $PTPKG_AWK_CMD '{print length, $0}' | \
        sort -rn | $PTPKG_AWK_CMD '{$1=""; print $0 }' | ( \
            while read file ; do
              if [ -d "$file" ] ; then
                rmdir "$file" 2>&1 && echo "$file"
              else
                rm "$file" && echo "$file"
              fi
            done
          )

    # Run the uninstall script, if it exists.
    if [ -r "$PTPKG_PKG_UNINSTALL_FILE" ] ; then
      source "$PTPKG_PKG_UNINSTALL_FILE" || exit 2
    fi

    # Verify that all files have been removed
    nfiles=`cat "$PTPKG_PKG_RECEIPT" | xargs ls 2>/dev/null | wc -l`
    if [ "$nfiles" -gt 0 ] ; then
      echo "WARNING: One or more files remain after uninstall operation."
      cat "$PTPKG_PKG_RECEIPT" | xargs ls -d 2>/dev/null
    fi

    # Remove the old receipt
    mv "$PTPKG_PKG_RECEIPT" "$PTPKG_PKG_RECEIPT.`date +%Y%m%d%H%M%S`"

    # Rescan installation directory
    echo "Updating file lists..."
    find "$PTPKG_INSTALL_PREFIX" | sort > "$PTPKG_HOME/.receipt0"

    _STOP_TIME=`date +%s`
    ((_INSTALL_SEC=$_STOP_TIME - $_START_TIME))

    case "$PTPKG_SCRIPT_FLAG" in
      "uninstall")
        banner "$PTPKG_PKG: Uninstallation complete in $_INSTALL_SEC seconds"
        exit 0  # Just exits subshell, not parent
        ;;
      "skip")
        banner "$PTPKG_PKG: Uninstallation skipped"
        exit 1  # Just exits subshell, not parent
        ;;
      "abort")
        banner "$PTPKG_PKG: Uninstallation aborted after $_INSTALL_SEC seconds"
        exit 2  # Just exits subshell, not parent
        ;;
      *)
        banner "$PTPKG_PKG: Uninstallation status unknown"
        exit 100
        ;;
    esac
  ) 2>&1 | tee "$PTPKG_LOGFILE"

  # Read subshell exit code to set flags and return code
  case ${PIPESTATUS[0]} in
    0)
      PTPKG_SCRIPT_FLAG="uninstall"
      PTPKG_UNINSTALLED_PACKAGES=( ${PTPKG_UNINSTALLED_PACKAGES[@]} "$PTPKG_PKG" )
      return 0
      ;;
    1)
      PTPKG_SCRIPT_FLAG="skip"
      rm -f "$PTPKG_LOGFILE"
      PTPKG_SKIPPED_PACKAGES=( ${PTPKG_SKIPPED_PACKAGES[@]} "$PTPKG_PKG" )
      return 0
      ;;
    2)
      PTPKG_SCRIPT_FLAG="abort"
      PTPKG_ABORTED_PACKAGES=( ${PTPKG_ABORTED_PACKAGES[@]} "$PTPKG_PKG" )
      return 1
      ;;
    *)
      PTPKG_SCRIPT_FLAG="unknown"
      return 1
      ;;
    esac
}

#
# Creates a "freezedried" package from the binary and text files
# in a $PTPKG_PKG_INSTALL_DIR.  Absolute paths in ASCII text files
# are replaced with the string "PTPKG_INSTALL_PREFIX" so the paths
# can be fixed when the package is installed later.
#
#
function execPackageFreezedry {
  PTPKG_SCRIPT_FLAG="freezedry"

  # Execute in a subshell to protect envrionment pollution in the parent script
  (
    _START_TIME=`date +%s`
    banner "$PTPKG_PKG: Freezedrying"

    SRC_CONFDIR="$PTPKG_SOURCE_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET"
    SRC_ANYDIR="$PTPKG_SOURCE_PREFIX/packages/$PTPKG_PKG/any-any-any-any"
    DEST_CONFDIR="$PTPKG_DEST_PREFIX/packages/$PTPKG_PKG/$PTPKG_TARGET"
    DEST_ANYDIR="$PTPKG_DEST_PREFIX/packages/$PTPKG_PKG/any-any-any-any"

    if [ -d "$DEST_CONFDIR" ] ; then
      case "$PTPKG_OVERWRITE_FLAG" in
        y*|Y*)
          echo "Removing old target configuration: $DEST_CONFDIR"
          rm -rf "$DEST_CONFDIR"
          ;;
        n*|N*)
          # Skip this package. Just exits subshell, not parent
          echo "$PTPKG_PKG is already freezedried. Skipping."
          exit 1
          ;;
        *)
          if promptYesNo "$PTPKG_TARGET for $PTPKG_PKG exists.  Overwrite?" ; then
            echo "Removing old target configuration: $DEST_CONFDIR"
            rm -rf "$DEST_CONFDIR"
          else
            # Skip installation. Just exits subshell, not parent
            exit 1
          fi
          ;;
      esac
    fi

    # If the package cannot be freezedried, copy the source package
    if [ -e "$PTPKG_PKG_NO_FREEZEDRY" ] ; then
      echo "$PTPKG_PKG: Cannot be freezedried.  Copying source instead."
      if ! copyPackage "$DEST_CONFDIR" "include_source" ; then
        echo "Failed to copy source"
        exit 2 # Exit subshell
      fi
      # If the any-any-any-any config is missing, use the target config
      if [ ! -d "$DEST_ANYDIR" ] ; then 
        echo "Using $PTPKG_TARGET for any-any-any-any configuration"
        mkdir -p "$DEST_ANYDIR"
        echo "This freezedried package can only be installed on $PTPKG_TARGET" > "$DEST_ANYDIR/README"
      fi
      echo "Source package copied successfully"
      echo
      exit 0 # Exit subshell
    fi

    # Create a working directory
    PTPKG_PKG_FREEZEDRY_DIR="$PTPKG_BUILD_PREFIX/$PTPKG_PKG"
    [ -d "PTPKG_PKG_FREEZEDRY_DIR" ] && rm -rf "$PTPKG_PKG_FREEZEDRY_DIR"
    mkdir -p "$PTPKG_PKG_FREEZEDRY_DIR" || exit 2

    # Use tar to collect the files to the working directory
    cd $PTPKG_INSTALL_PREFIX
    echo "Collecting files from $PTPKG_INSTALL_PREFIX"
    cat "$PTPKG_PKG_RECEIPT" | $PTPKG_SED_CMD -e "s#$PTPKG_INSTALL_PREFIX/##" | \
        xargs file | grep -v directory | cut -d: -f1 \
        > "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.files"
    $PTPKG_TAR_CMD cpf "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.tar" \
        -T "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.files" || exit 2

    # Extract and remove the temp tar file (can I do this in one step?)
    cd "$PTPKG_PKG_FREEZEDRY_DIR"
    echo "Copying files to $PTPKG_PKG_FREEZEDRY_DIR"
    $PTPKG_TAR_CMD xpf "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.tar" || exit 2
    rm "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.tar"

    # Find files with absolute paths to the installation directory
    echo "Finding files with absolute paths..."
    find . | egrep -v '\.(a|so|so\.*|py[oc])$' | \
        xargs file | egrep "(text|libtool)" | cut -d: -f1 | \
        xargs grep -l "$PTPKG_INSTALL_PREFIX" > "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.fix"
    nfix=`cat "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.fix" | wc -l | cut -d' ' -f1`
    echo "$nfix files contain absolute paths"

    # Fix absolute paths
    if [ "$nfix" -gt 0 ] ; then 
      echo "Fixing absolute paths:"
      cat "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.fix" | \
        (
          while read file ; do
            echo "    $file"
            $PTPKG_SED_CMD -i -e "s#$PTPKG_INSTALL_PREFIX#PTPKG_INSTALL_PREFIX#g" "$file"
          done
        )
    fi

    # Create the configuration directory
    mkdir -p "$DEST_CONFDIR"

    # Create the package source archive
    echo "Creating source archive file: ${PTPKG_PKG}_${PTPKG_TARGET}.tar.bz2"
    $PTPKG_TAR_CMD cjpf "$DEST_CONFDIR/../${PTPKG_PKG}_${PTPKG_TARGET}.tar.bz2" .

    # Write the package source file
    cat > "$DEST_CONFDIR/source" <<EOF
#
# Freezedried source file
# Created: `date`
#
FILE=${PTPKG_PKG}_${PTPKG_TARGET}.tar.bz2

EOF
    echo "Wrote $PTPKG_TARGET/source"

    # Create the install file
    cat > "$DEST_CONFDIR/install" <<EOF
#
# Installation script for freezedried package
# $nfix files contain absolute paths.
# Created: `date`
#
 
# Unpack the archive directly to the installation destination
pkgUnpackToInstall

EOF

    if [ "$nfix" -gt 0 ] ; then
      cat >> "$DEST_CONFDIR/install" <<EOF
# Replace absolute path placeholders with actual path
pkgFixAbsolutePaths `cat $PTPKG_BUILD_PREFIX/$PTPKG_PKG.fix | tr '\n' ' '`

EOF
    fi
    echo "Wrote $PTPKG_TARGET/install"

    # Copy dependencies, environment, test, and uninstall files
    for f in dependencies environment test uninstall ; do
      tgt="$SRC_CONFDIR/$f"
      any="$SRC_ANYDIR/$f"
      if [ -e "$tgt" ] ; then
        cp -R "$tgt" "$DEST_CONFDIR"
        echo "$PTPKG_TARGET/$f --> $PTPKG_TARGET/$f"
      elif [ -e "$any" ] ; then 
        cp -R "$any" "$DEST_CONFDIR"
        echo "any-any-any-any/$f --> $PTPKG_TARGET/$f"
      fi
    done

    # If the any-any-any-any config is missing, use the target config
    if [ ! -d "$DEST_ANYDIR" ] ; then 
      echo "Using $PTPKG_TARGET for any-any-any-any configuration"
      mkdir -p "$DEST_ANYDIR"
      echo "This freezedried package can only be installed on $PTPKG_TARGET" > "$DEST_ANYDIR/README"
    fi

    # Cleanup
    rm -rf "$PTPKG_PKG_FREEZEDRY_DIR" 
    rm -f "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.fix" "$PTPKG_BUILD_PREFIX/$PTPKG_PKG.files"

    # Report
    ntar=`$PTPKG_TAR_CMD tjf $DEST_CONFDIR/../${PTPKG_PKG}_${PTPKG_TARGET}.tar.bz2 | wc -l`
    nrct=`cat "$PTPKG_PKG_RECEIPT" | wc -l`
    echo "$ntar files freezedried ($nrct files and directories in receipt)"

    _STOP_TIME=`date +%s`
    ((_INSTALL_SEC=$_STOP_TIME - $_START_TIME))

    case "$PTPKG_SCRIPT_FLAG" in
      "freezedry")
        banner "$PTPKG_PKG: Freezedry complete in $_INSTALL_SEC seconds"
        exit 0  # Just exits subshell, not parent
        ;;
      "skip")
        banner "$PTPKG_PKG: Freezedry skipped"
        exit 1  # Just exits subshell, not parent
        ;;
      "abort")
        banner "$PTPKG_PKG: Freezedry aborted after $_INSTALL_SEC seconds"
        exit 2  # Just exits subshell, not parent
        ;;
      *)
        banner "$PTPKG_PKG: Freezedry status unknown"
        exit 100
        ;;
    esac
  )

  # Read subshell exit code to set flags and return code
  case ${PIPESTATUS[0]} in
    0)
      PTPKG_SCRIPT_FLAG="freezedry"
      PTPKG_FREEZEDRIED_PACKAGES=( ${PTPKG_FREEZEDRIED_PACKAGES[@]} "$PTPKG_PKG" )
      return 0
      ;;
    1)
      PTPKG_SCRIPT_FLAG="skip"
      rm -f "$PTPKG_LOGFILE"
      PTPKG_SKIPPED_PACKAGES=( ${PTPKG_SKIPPED_PACKAGES[@]} "$PTPKG_PKG" )
      return 0
      ;;
    2)
      PTPKG_SCRIPT_FLAG="abort"
      PTPKG_ABORTED_PACKAGES=( ${PTPKG_ABORTED_PACKAGES[@]} "$PTPKG_PKG" )
      return 1
      ;;
    *)
      PTPKG_SCRIPT_FLAG="unknown"
      return 1
      ;;
    esac
}


#
# Copies files for the selected package to the specified destination.
# By default, the package archive file is not copied (metafiles only).
# Arguments: $1: folder to copy to, 
#            $2: optional flag to enable source and patch copy
# Returns: 0 if no errors encountered, 1 otherwise
#
function copyPackage {
  local _DEST="$1"
  local _COPYFLAG="${2-false}"

  # Make package directory
  mkdir -p "$_DEST" || return 1

  # Copy required files
  cp "$PTPKG_PKG_SOURCE_FILE" "$_DEST" || return 1
  cp "$PTPKG_PKG_DEPENDS_FILE" "$_DEST" || return 1
  cp "$PTPKG_PKG_INSTALL_FILE" "$_DEST" || return 1

  # Copy optional files
  if [ -f "$PTPKG_PKG_UNINSTALL_FILE" ] ; then
    cp "$PTPKG_PKG_UNINSTALL_FILE" "$_DEST" || return 1
  fi
  if [ -d "$PTPKG_PKG_TEST_DIR" ] ; then
    rm -rf "$_DEST/$PTPKG_PKG_TEST_DIR"
    cp -R "$PTPKG_PKG_TEST_DIR" "$_DEST" || return 1
  fi
  if [ -f "$PTPKG_PKG_ENV_FILE" ] ; then
    cp "$PTPKG_PKG_ENV_FILE" "$_DEST" || return 1
  fi
  if [ -f "$PTPKG_PKG_NO_FREEZEDRY" ] ; then
    cp "$PTPKG_PKG_NO_FREEZEDRY" "$_DEST" || return 1
  fi

  # Copy archive and patch files
  if [ $_COPYFLAG != false ] ; then
    if [ -f "$PTPKG_PKG_DIR/$PTPKG_PKG_ARCHIVE" ] ; then
      cp "$PTPKG_PKG_DIR/$PTPKG_PKG_ARCHIVE" "$1/.." || return 1
    fi
    for patch in `cat $PTPKG_PKG_INSTALL_FILE | grep pkgPatch | cut -d' ' -f2` ; do
      cp "$PTPKG_PKG_DIR/$patch" "$1/.." || return 1
    done
  fi
}

#
# Lists $PTPKG_PACKAGES_DIR to discover available packages
# and the features they provide
# Arguments: none
# Returns: none
#
function getPackages {
  validHome || return

  # Reset globals
  PTPKG_PACKAGE_SELECTION=( )
  PTPKG_ALL_PACKAGES=( )
  PTPKG_ALL_FEATURES=( )
  PTPKG_PROVIDED_BY_PACKAGE=( )
  PTPKG_REQUIRED_BY_PACKAGE=( )
  PTPKG_EXCLUDED_BY_PACKAGE=( )
  PTPKG_PACKAGES_PROVIDING=( )
  PTPKG_PACKAGE_DATA=( )

  local _PKG
  local _PKG_NAME
  local _PKG_DIR
  local _MATCH
  local _IDX=0

  # Get all packages
  echo "Loading packages database from $PTPKG_PACKAGES_DIR"
  declare -a _PACKAGES=( `ls "$PTPKG_PACKAGES_DIR"` )
  if [ ${#_PACKAGES[@]} -eq 0 ] ; then
    exitWithError "No packages found in $PTPKG_PACKAGES_DIR"
  fi

  # Construct package database
  for _PKG in ${_PACKAGES[@]} ; do
    _PKG_NAME="${_PKG%-*}"
    _PKG_DIR="$PTPKG_PACKAGES_DIR/$_PKG"

    # Package must exist
    [ -d "$_PKG_DIR" ] || \
        exitWithError "$_PKG: Invalid package: $_PKG_DIR is not a directory"

    # Find target-specific config files
    _MATCH="`matchTargetDir "$_PKG_DIR"`"
    [ $? -eq 0 ] || \
        exitWithError "$_PKG: Invalid package: No target configurations in $_PKG_DIR"

    # Get handles on package config files
    _PKG_SOURCE_FILE="$_MATCH/source"
    _PKG_ENV_FILE="$_MATCH/environment"
    _PKG_DEPENDS_FILE="$_MATCH/dependencies"
    _PKG_INSTALL_FILE="$_MATCH/install"
    _PKG_UNINSTALL_FILE="$_MATCH/uninstall"
    _PKG_TEST_DIR="$_MATCH/test"
    _PKG_NO_FREEZEDRY="$_MATCH/no_freezedry"

    # Fall back to defaults
    [ -r "$_PKG_DIR/$_PKG_SOURCE_FILE" ] || \
        _PKG_SOURCE_FILE="any-any-any-any/source"
    [ -r "$_PKG_DIR/$_PKG_DEPENDS_FILE" ] || \
        _PKG_DEPENDS_FILE="any-any-any-any/dependencies"
    [ -r "$_PKG_DIR/$_PKG_INSTALL_FILE" ] || \
        _PKG_INSTALL_FILE="any-any-any-any/install"
    [ -r "$_PKG_DIR/$_PKG_UNINSTALL_FILE" ] || \
        _PKG_UNINSTALL_FILE="any-any-any-any/uninstall"
    [ -r "$_PKG_DIR/$_PKG_ENV_FILE" ] || \
        _PKG_ENV_FILE="any-any-any-any/environment"
    [ -d "$_PKG_DIR/$_PKG_TEST_DIR" ] || \
        _PKG_TEST_DIR="any-any-any-any/test"
    [ -e "$_PKG_DIR/$_PKG_NO_FREEZEDRY" ] || \
        _PKG_NO_FREEZEDRY="any-any-any-any/no_freezedry"

    # Check package
    [ -r "$_PKG_DIR/$_PKG_SOURCE_FILE" ] || \
        exitWithError "$_PKG: Invalid package: $_PKG_DIR/$_PKG_SOURCE_FILE is not readable"
    [ -r "$_PKG_DIR/$_PKG_DEPENDS_FILE" ] || \
        exitWithError "$_PKG: Invalid package: $_PKG_DIR/$_PKG_DEPENDS_FILE is not readable"
    [ -r "$_PKG_DIR/$_PKG_INSTALL_FILE" ] || \
        exitWithError "$_PKG: Invalid package: $_PKG_DIR/$_PKG_INSTALL_FILE is not readable"

    # Get package dependencies
    source "$_PKG_DIR/$_PKG_DEPENDS_FILE"
    PTPKG_PKG_PROVIDES=( ${PROVIDES[@]} )
    PTPKG_PKG_REQUIRES=( ${REQUIRES[@]} )
    PTPKG_PKG_EXCLUDES=( ${EXCLUDES[@]} )

    # Filter packages that are excluded by the selected target
    local feat
    local exclude=false
    for feat in "$_PKG" "$_PKG_NAME" ${PTPKG_PKG_PROVIDES[@]} ; do
      if contains "$feat" ${PTPKG_TARGET_EXCLUDES[@]} ; then
        exclude=true
        break
      fi
    done

    if [ $exclude == false ] ; then
      # Add and package full name, name, and features to the database
      PTPKG_ALL_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]} "$_PKG" )
      PTPKG_ALL_FEATURES=( ${PTPKG_ALL_FEATURES[@]} "$_PKG" "$_PKG_NAME" ${PTPKG_PKG_PROVIDES[@]} )

      # Record package data
      PTPKG_PACKAGE_DATA[$_IDX]="$_PKG_SOURCE_FILE $_PKG_DEPENDS_FILE \
          $_PKG_INSTALL_FILE $_PKG_UNINSTALL_FILE $_PKG_ENV_FILE $_PKG_TEST_DIR $_PKG_NO_FREEZEDRY"

      # Record dependency information
      PTPKG_PROVIDED_BY_PACKAGE[$_IDX]="$_PKG $_PKG_NAME ${PTPKG_PKG_PROVIDES[@]}"
      PTPKG_REQUIRED_BY_PACKAGE[$_IDX]="${PTPKG_PKG_REQUIRES[@]}"
      PTPKG_EXCLUDED_BY_PACKAGE[$_IDX]="${PTPKG_PKG_EXCLUDES[@]}"
      ((_IDX++))
    fi
  done # for _PKG in ${_PACKAGES[@]}

  # Add pseudo-package for target configuration
  source $PTPKG_TARGET_DIR/dependencies || return 1
  PTPKG_TARGET_PROVIDES=( ${PROVIDES[@]} )
  PTPKG_TARGET_REQUIRES=( ${REQUIRES[@]} )
  PTPKG_TARGET_EXCLUDES=( ${EXCLUDES[@]} )
  PTPKG_ALL_PACKAGES=( ${PTPKG_ALL_PACKAGES[@]} "$PTPKG_TARGET" )
  PTPKG_ALL_FEATURES=( ${PTPKG_ALL_FEATURES[@]} "$PTPKG_TARGET" ${PTPKG_TARGET_PROVIDES[@]} )
  PTPKG_PACKAGE_DATA[$_IDX]="NULL dependencies NULL NULL environment NULL"
  PTPKG_PROVIDED_BY_PACKAGE[$_IDX]="$PTPKG_TARGET ${PTPKG_TARGET_PROVIDES[@]}"
  PTPKG_REQUIRED_BY_PACKAGE[$_IDX]="${PTPKG_TARGET_REQUIRES[@]}"
  PTPKG_EXCLUDED_BY_PACKAGE[$_IDX]="${PTPKG_TARGET_EXCLUDES[@]}"

  # Remove duplicates from the feature list
  removeDuplicates ${PTPKG_ALL_FEATURES[@]}
  PTPKG_ALL_FEATURES=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} )

  # Construct package <-- feature reverse lookup
  local pkg
  local feat
  local pkg_feat
  local i
  for ((i=0; i<${#PTPKG_ALL_FEATURES[@]}; i++)) {
    feat="${PTPKG_ALL_FEATURES[$i]}"
    local j
    for ((j=0; j<${#PTPKG_ALL_PACKAGES[@]}; j++)) {
      pkg="${PTPKG_ALL_PACKAGES[$j]}"
      pkg_feat="${PTPKG_PROVIDED_BY_PACKAGE[$j]}"
      if contains "${feat}" ${pkg_feat} ; then
        PTPKG_PACKAGES_PROVIDING[$i]="${PTPKG_PACKAGES_PROVIDING[$i]} ${pkg}"
      fi
    }
  }

  # Load default package selections
  local _DEFAULTS_FILE="${PTPKG_PACKAGES_DIR}/.defaults"
  local _DEFAULTS=""
  if [ -r ${_DEFAULTS_FILE} ] ; then
    _DEFAULTS=`cat ${_DEFAULTS_FILE}`
  fi
  PTPKG_DEFAULT_PACKAGES=( `echo $_DEFAULTS` )
}

#
# Presents the user with a menu of available packages and recieves their selections.
# Arguments: $1: Optional name of an array variable containing selections.
#                Defaults to PTPKG_ALL_PACKAGES.
#            $2: Optional name of an array variable containing defaults.
#                Defaults to PTPKG_DEFAULT_PACKAGES.
# Returns: none
#
function getPackageSelections {
  filterMenu "Select packages" ${1-PTPKG_ALL_PACKAGES} ${2-PTPKG_DEFAULT_PACKAGES}
  PTPKG_PACKAGE_SELECTION=( ${PTPKG_FILTERMENU_SELECTIONS[@]} )
  if [ ${#PTPKG_PACKAGE_SELECTION[@]} -eq 0 ] ; then
    exitWithError "No packages selected."
  fi
}

#
# Resolves an ambiguous feature name to a specific package.
# The disambiguated result (a package name) is returned in the
# PTPKG_RESOLVEAMBIGUOUS_RESULT global.
# Arguments: $1: Feature, $2...: Packages providing that feature
# Returns: none
#
function resolveAmbiguousFeature {
  local _FEAT="$1"
  shift
  declare -a _PROV=( "$@" )
  local _PKG="$PTPKG_PKG"
  local pkg

  # If the target provides the feature, use the target pseudo-package
  if contains $PTPKG_TARGET ${_PROV[@]} ; then
    PTPKG_RESOLVEAMBIGUOUS_RESULT="$PTPKG_TARGET"
    return
  fi
  
  # If the target requires a package that provides this feature then 
  # use the required package
  for pkg in ${PTPKG_TARGET_REQUIRES[@]} ; do
    selectPackage "$pkg"
    if contains "$_FEAT" ${PTPKG_PKG_PROVIDES[@]} ; then
      PTPKG_RESOLVEAMBIGUOUS_RESULT="$pkg"
      selectPackage "$_PKG"
      return
    fi
  done

  # Search user-specified packages
  for pkg in ${_PROV[@]} ; do
    if contains "$pkg" ${PTPKG_PACKAGE_SELECTION[@]} ; then
      PTPKG_RESOLVEAMBIGUOUS_RESULT="$pkg"
      return
    fi
  done

  # Ask the user
  banner "Please select a $_FEAT package"
  select _FEAT in ${_PROV[@]} ; do
    PTPKG_RESOLVEAMBIGUOUS_RESULT="`echo $_FEAT`"
    break
  done

  # Record user's selection so we don't have to ask again
  PTPKG_PACKAGE_SELECTION=( ${PTPKG_PACKAGE_SELECTION[@]} "$PTPKG_RESOLVEAMBIGUOUS_RESULT" )
}

#
# Resolves a feature name to a package that provides it.
# Arguments: $1: Feature to resolve
# Returns: 0 if the feature is successfully resolved, 1 otherwise
#
function resolveFeature() {
  PTPKG_RESOLVEFEATURE_RESULT=""
  local req="$1"
  local feat
  declare -a prov

  local i
  for ((i=0; i<${#PTPKG_ALL_FEATURES[@]}; i++)) {
    feat="${PTPKG_ALL_FEATURES[$i]}"
    if [ "$req" == "$feat" ] ; then
      prov=( ${PTPKG_PACKAGES_PROVIDING[$i]} )
      if [ ${#prov[@]} -eq 1 ] ; then
        #echo "Using ${prov[0]} to provide $feat"
        PTPKG_RESOLVEFEATURE_RESULT="${prov[0]}"
      elif [ ${#prov[@]} -gt 1 ] ; then
        #echo "(${prov[@]}) provides $feat"
        resolveAmbiguousFeature "$feat" ${prov[@]}
        PTPKG_RESOLVEFEATURE_RESULT="$PTPKG_RESOLVEAMBIGUOUS_RESULT"
      else
        exitWithError "Error: package/feature database is corrupt."
      fi
      return 0
    fi
  }
  return 1
}

#
# Recursively processes dependency information for the selected package
# Arguments: none
# Returns: 0 if the dependencies are processed successfully, 1 otherwise
#
function processDependencies {
  # Skip empty dependency lists
  [ ${#PTPKG_PKG_REQUIRES[@]} -eq 0 ] && return 0

  # Skip packages already visited
  contains "$PTPKG_PKG" ${_VISITED[@]} && return 0
  _VISITED=( $PTPKG_PKG ${_VISITED[@]} )

  declare -a _REQUIRES
  local pkg
  local req
  local feat
  local _PKG="$PTPKG_PKG"

  #echo "$_PKG: PTPKG_PKG_REQUIRES=${PTPKG_PKG_REQUIRES[@]}"

  for pkg in ${PTPKG_PKG_REQUIRES[@]} ; do
    if resolveFeature "$pkg" ; then
      _REQUIRES=( ${_REQUIRES[@]} "$PTPKG_RESOLVEFEATURE_RESULT" )
    else
      banner "Error: No package provides $pkg (required by $_PKG)"
      return 1
    fi
  done

  # Remove duplicates from the dependency list
  removeDuplicates ${_REQUIRES[@]}
  _REQUIRES=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} )
  #echoAll "$_PKG requirements" ${_REQUIRES[@]}

  for pkg in ${_REQUIRES[@]} ; do
    # Select package and add dependencies to the list
    if selectPackage "$pkg" ; then
      processDependencies || return 1
      _DEPENDENCIES=( ${_DEPENDENCIES[@]} "$pkg" )
    else
      banner "Error: $pkg is invalid (required by $_PKG)"
      return 1
    fi
  done

  # Re-select top-level package
  selectPackage "$_PKG"
  return 0
}

#
# Processes PTPKG_PACKAGE_SELECTION to construct a list of packages to install.
# Packages are listed in order so that dependency packages are installed first.
# The list is returned in the PTPKG_INSTALLATION global array.
# Arguments: none
# Returns: 0 if the installation can proceed, 1 otherwise
#
function buildInstallation {

  declare -a _SELECTION=( ${PTPKG_PACKAGE_SELECTION[@]} )
  local pkg

  PTPKG_INSTALLATION=( )
  _DEPENDENCIES=( )
  _VISITED=( $PTPKG_TARGET )

  # Process package selection
  declare -a _TMP=( )
  for pkg in ${PTPKG_TARGET_REQUIRES[@]} ${PTPKG_PACKAGE_SELECTION[@]} ; do
    # Resolve feature names to packages
    if resolveFeature "$pkg" ; then
      if ! contains "$PTPKG_RESOLVEFEATURE_RESULT" ${_TMP[@]} ; then
        _TMP=( ${_TMP[@]} "$PTPKG_RESOLVEFEATURE_RESULT" )
      fi
    else
      banner "Error: No package provides $pkg"
      return 1
    fi
  done
  PTPKG_PACKAGE_SELECTION=( ${_TMP[@]} )

  # Look for cached selection
  #local key=`echo "${PTPKG_PACKAGE_SELECTION[@]}" | md5sum | awk '{print $1}'`
  #if [ -f "$PTPKG_HOME/.depends_key" ] ; then
  #  local key2=`cat "$PTPKG_HOME/.depends_key"`
  #  if [ "$key" == "$key2" ] && [ -f "$PTPKG_HOME/.depends_cache" ] ; then
  #    PTPKG_INSTALLATION=( `cat "$PTPKG_HOME/.depends_cache"` )
  #    return 0
  #  fi
  #fi

  echo
  echo "Calculating package dependencies..."
  #echoAll "Calculating dependencies for selected packages" ${PTPKG_PACKAGE_SELECTION[@]}

  # Build the package dependency list
  for pkg in ${PTPKG_PACKAGE_SELECTION[@]} ; do
    if ! selectPackage "$pkg" ; then
      banner "Error: Invalid package $pkg"
      return 1
    fi
    #echo "Calculating dependencies for $PTPKG_PKG"
    processDependencies || return 1
    PTPKG_INSTALLATION=( ${PTPKG_INSTALLATION[@]} ${_DEPENDENCIES[@]} "$pkg" )
    PTPKG_DEPENDENCIES=( ${PTPKG_DEPENDENCIES[@]} ${_DEPENDENCIES[@]} )
    _DEPENDENCIES=( )
  done
  _VISITED=( $PTPKG_TARGET )

  # Remove the target pseudo-package from the installation and dependency lists
  PTPKG_INSTALLATION=( ${PTPKG_INSTALLATION[@]/$PTPKG_TARGET/} )
  PTPKG_DEPENDENCIES=( ${PTPKG_DEPENDENCIES[@]/$PTPKG_TARGET/} )

  # Remove duplicates from the installation list
  removeDuplicates ${PTPKG_INSTALLATION[@]}
  PTPKG_INSTALLATION=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} )

  # Remove duplicates from the dependency list
  removeDuplicates ${PTPKG_DEPENDENCIES[@]}
  PTPKG_DEPENDENCIES=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} )

  # Remove user-selected packages from the dependency list
  _TMP=( )
  for pkg in ${PTPKG_DEPENDENCIES[@]} ; do
    if ! contains "$pkg" ${_SELECTION[@]} ; then
      _TMP=( ${_TMP[@]} "$pkg" )
    fi
  done
  PTPKG_DEPENDENCIES=( ${_TMP[@]} )

  echo
  echo "Checking for conflicts..."

  # Check for mutually exclusive packages
  declare -a _PROVIDES
  declare -a _EXCLUDES
  local pkgA
  local pkgB
  local i
  for ((i=0; i<${#PTPKG_INSTALLATION[@]}-1; i++)) {
    pkgA=${PTPKG_INSTALLATION[$i]}
    selectPackage "$pkgA"
    _PROVIDES=( ${PTPKG_PKG_PROVIDES[@]} )
    _EXCLUDES=( ${PTPKG_PKG_EXCLUDES[@]} )
    local j
    for ((j=$i+1; j<${#PTPKG_INSTALLATION[@]}; j++)) {
      pkgB=${PTPKG_INSTALLATION[$j]}
      selectPackage "$pkgB"
      for feat in ${_EXCLUDES[@]} ; do
        if contains "$feat" ${PTPKG_PKG_PROVIDES[@]} ; then
          banner "$pkgA and $pkgB are mutually exclusive."
          return 1
        fi
      done
      for feat in ${_PROVIDES[@]} ; do
        if contains "$feat" ${PTPKG_PKG_EXCLUDES[@]} ; then
          banner "$pkgA and $pkgB are mutually exclusive."
          return 1
        fi
      done
    }
  }

  ## Cache results for quick restart
  #echo "$key" > "$PTPKG_HOME/.depends_key"
  #echo "${PTPKG_INSTALLATION[@]}" > "$PTPKG_HOME/.depends_cache"

  # Installation can proceed
  return 0
}

#
# Recursively bootstraps dependencies required by the selected package
# The target pseudo-package is always bootstraped
# Arguments: none
# Returns: 0 if all packages bootstraped successfully, 1 otherwise
#
function bootstrapPackageDeps {
  local _PKG="$PTPKG_PKG"
  declare -a _BOOTSTRAP
  local dep
  local pkg

  _DEPENDENCIES=( )
  _VISITED=( $PTPKG_TARGET )

  # Set environment file processing flags
  PTPKG_ENV_LANG="bash"
  PTPKG_ENV_ACTION="exec"
  PTPKG_ENV_MODE="build"

  # Build the package dependency list
  for dep in $PTPKG_TARGET ${PTPKG_PKG_REQUIRES[@]} ; do
    resolveFeature "$dep"
    pkg="$PTPKG_RESOLVEFEATURE_RESULT"
    if ! selectPackage "$pkg" ; then
      banner "Error: Cannot bootstrap invalid package $pkg"
      return 1
    fi
    processDependencies || return 1
    _BOOTSTRAP=( ${_BOOTSTRAP[@]} ${_DEPENDENCIES[@]} "$pkg" )
    _DEPENDENCIES=( )
  done
  _VISITED=( $PTPKG_TARGET )

  # Remove duplicates from the bootstrap list
  removeDuplicates ${_BOOTSTRAP[@]}
  _BOOTSTRAP=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} )

  # Add the installation site to the build environment
  loadInstallationEnvironment

  # Bootstrap each package
  for pkg in ${_BOOTSTRAP[@]} ; do
    selectPackage $pkg || return 1
    if [ -r "$PTPKG_PKG_ENV_FILE" ] ; then
      source "$PTPKG_PKG_ENV_FILE" || return 1
    fi
  done

  # Re-select original package
  selectPackage "$_PKG"
  return $?
}

# EOF
