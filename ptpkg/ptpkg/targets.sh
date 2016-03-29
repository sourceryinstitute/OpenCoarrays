#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: targets.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    Target management functions
#

# Use shell script loader
include menus.sh

# PTPKG targets directory
export PTPKG_TARGETS_DIR="$PTPKG_HOME/targets"

# Host configuration
PTPKG_HOST_OS="any"
PTPKG_HOST_DIST="any"
PTPKG_HOST_COMP="any"
PTPKG_HOST_ARCH="any"
PTPKG_HOST=""

# Target configuration
PTPKG_TARGET_OS="any"
PTPKG_TARGET_DIST="any"
PTPKG_TARGET_COMP="any"
PTPKG_TARGET_ARCH="any"
PTPKG_TARGET=""
PTPKG_TARGET_DIR=""
PTPKG_TARGET_LIBRARY_PATH="LD_LIBRARY_PATH"

# Target dependency information
declare -a PTPKG_TARGET_PROVIDES
declare -a PTPKG_TARGET_REQUIRES
declare -a PTPKG_TARGET_EXCLUDES

# Supported targets
declare -a PTPKG_ALL_TARGETS

# Installed targets
declare -a PTPKG_INSTALLED_TARGETS


#
# Sets global target variables
# Arguments: $1: target to select
# Returns: 0 if the target was selected successfully, 1 otherwise
#
function selectTarget {
  local _TARGET="$1"

  verifyTarget "${_TARGET}" || return 1

  # Set global variables
  PTPKG_TARGET_OS="`echo "$_TARGET" | cut -d- -f1`"
  PTPKG_TARGET_DIST="`echo "$_TARGET" | cut -d- -f2`"
  PTPKG_TARGET_COMP="`echo "$_TARGET" | cut -d- -f3`"
  PTPKG_TARGET_ARCH="`echo "$_TARGET" | cut -d- -f4`"
  PTPKG_TARGET="${_TARGET}"
  PTPKG_TARGET_DIR="${PTPKG_TARGETS_DIR}/${_TARGET}"

  # Get target dependency information
  source "$PTPKG_TARGET_DIR/dependencies" || return 1
  PTPKG_TARGET_PROVIDES=( ${PROVIDES[@]} )
  PTPKG_TARGET_REQUIRES=( ${REQUIRES[@]} )
  PTPKG_TARGET_EXCLUDES=( ${EXCLUDES[@]} )

  # Set system-specific library path string
  case "$PTPKG_TARGET_OS" in
    darwin)
      export PTPKG_TARGET_LIBRARY_PATH="DYLD_LIBRARY_PATH"
      ;;
    aix)
      export PTPKG_TARGET_LIBRARY_PATH="LIBPATH"
      ;;
    *)
      export PTPKG_TARGET_LIBRARY_PATH="LD_LIBRARY_PATH"
      ;;
  esac

  PTPKG_ENV_ACTION="exec"
  PTPKG_ENV_MODE="run"
  source "$PTPKG_TARGET_DIR/environment" || return 1

  # Set required utility commands
  PTPKG_MAKE_CMD=${MAKE:-make}
  PTPKG_TAR_CMD=${TAR:-tar}
  PTPKG_SED_CMD=${TAR:-sed}
  PTPKG_AWK_CMD=${TAR:-awk}

  # Make sure we're using GNU freeware on AIX
  if [ "$PTPKG_TARGET_OS" == "aix" ] ; then
    if which gmake 2>&1 > /dev/null ; then
      PTPKG_MAKE_CMD=gmake
    else
      echo "AIX detected but gmake not available."
      echo "gmake is required to compile required packages."
      return 1
    fi
    if which gtar 2>&1 > /dev/null ; then
      PTPKG_TAR_CMD=gtar
    else    
      echo "AIX detected but gtar not available."
      echo "gtar is required to handle long filenames in tar archives."
      return 1
    fi
    PTPKG_SED_CMD=/opt/freeware/bin/sed
    if ! [ -x "$PTPKG_SED_CMD" ] ; then
      echo "AIX detected but $PTPKG_SED_CMD not executable."
      return 1
    fi
    PTPKG_AWK_CMD=/opt/freeware/bin/awk
    if ! [ -x "$PTPKG_AWK_CMD" ] ; then
      echo "AIX detected but $PTPKG_AWK_CMD not executable."
      return 1
    fi
  fi
}

#
# Attempts to automatically select a target configuration.
# Arguments: none
# Returns: 0 if the target was selected successfully, 1 otherwise
#
function autoSelectTarget {
  if [ -d "$PTPKG_TARGETS_DIR/$PTPKG_HOST" ] ; then
    # Always prefer the host target
    selectTarget "$PTPKG_HOST" || \
        exitWithError "Invalid target: $PTPKG_HOST"
  elif [ ${#PTPKG_ALL_TARGETS[@]} -eq 1 ] ; then
    # If there's only one target available, use that target
    selectTarget "${PTPKG_ALL_TARGETS[0]}" || \
        exitWithError "Invalid target: ${PTPKG_ALL_TARGETS[0]}"
  else
    PTPKG_TARGET=""
    for _OS in "$PTPKG_HOST_OS" "any" ; do
      for _DIST in "$PTPKG_HOST_DIST" "any" ; do
        for _COMP in "$PTPKG_HOST_COMP" "any" ; do
          for _ARCH in "$PTPKG_HOST_ARCH" "any" ; do
            _TARG="${_OS}-${_DIST}-${_COMP}-${_ARCH}"
            if [ -d "$PTPKG_TARGETS_DIR/$_TARG" ] ; then
              selectTarget "$_TARG" || \
                  exitWithError "Invalid target: $_TARG"
            fi
          done
        done
      done
    done
    if [ -z "$PTPKG_TARGET" ] ; then
      # Target couldn't be automatically selected
      return 1
    fi
  fi
  return 0
}

#
# Attempts to set PTPKG_HOST automatically
# Arguments: none
# Returns: 0 if the target was resolved successfully, 1 otherwise
#
function resolveHost {
  PTPKG_HOST_OS="any"
  PTPKG_HOST_DIST="any"
  PTPKG_HOST_COMP="any"
  PTPKG_HOST_ARCH="any"

  local _RETVAL=0

  # Detect operating system family
  local _HOST_OS=`uname -s`
  case "`echo $_HOST_OS`" in
    "AIX")
      PTPKG_HOST_OS="aix"
      PTPKG_HOST_DIST=`oslevel`
      ;;
    "Darwin")
      PTPKG_HOST_OS="darwin"
      getDarwinDistro
      _RETVAL=$?
      ;;
    "Linux")
      PTPKG_HOST_OS="linux"
      getLinuxDistro
      _RETVAL=$?
      ;;
    *)
      echo "Warning: Unknown host OS: $_HOST_OS"
      PTPKG_HOST_OS="any"
      _RETVAL=1
      ;;
  esac

  # Detect compiler
  if ! [ -z "$COMPILER" ] ; then
    # Some DSRC systems set this in module files
    case $COMPILER in
      "intel")
        PTPKG_HOST_COMP="intel"
        ;;
      "gcc")
        PTPKG_HOST_COMP="gnu"
        ;;
      "pgi")
        PTPKG_HOST_COMP="pgi"
        ;;
      "ibm")
        PTPKG_HOST_COMP="ibm"
        ;;
    esac
  fi
  if [ "$PTPKG_HOST_COMP" == "any" ] ; then
    # Search the path
    if which icc > /dev/null 2>&1 ; then
      PTPKG_HOST_COMP="intel"
    elif which gcc > /dev/null 2>&1 ; then
      PTPKG_HOST_COMP="gnu"
    elif which pgcc > /dev/null 2>&1 ; then
      PTPKG_HOST_COMP="pgi"
    elif which xlc > /dev/null 2>&1 ; then
      PTPKG_HOST_COMP="ibm"
    fi
  fi

  # Detect architecture
  local _HOST_ARCH
  if [ "$PTPKG_HOST_OS" == "aix" ] ; then
    _HOST_ARCH=`uname -p`
  else
    _HOST_ARCH=`uname -m`
  fi
  case "$_HOST_ARCH" in
    *i386*)
      PTPKG_HOST_ARCH="i386"
      ;;
    *x86_64*)
      PTPKG_HOST_ARCH="x86_64"
      ;;
    *powerpc*)
      case `ls -l /unix` in
      *_64)
        PTPKG_HOST_ARCH="ppc64"
        ;;
      *_up|*_mp)
        PTPKG_HOST_ARCH="ppc32"
        ;;
      *)
        PTPKG_HOST_ARCH="ppc"
        ;;
      esac
      ;;
    *)
      echo "Warning: Unknown host architecture: $PTPKG_HOST_ARCH"
      PTPKG_HOST_ARCH="any"
      _RETVAL=1
      ;;
  esac

  export PTPKG_HOST_OS
  export PTPKG_HOST_DIST
  export PTPKG_HOST_COMP
  export PTPKG_HOST_ARCH
  export PTPKG_HOST="${PTPKG_HOST_OS}-${PTPKG_HOST_DIST}-${PTPKG_HOST_COMP}-${PTPKG_HOST_ARCH}"

  echo "Detected host configuration: $PTPKG_HOST"
  return $_RETVAL
}

#
# On Darwin hosts, determines the distribution
# Arguments: none
# Returns 0 if the distro was determined, 1 otherwise
#
function getDarwinDistro {
  local _RETVAL=0

  # Ask Bash about the OS
  case "$OSTYPE" in
  darwin10*)
    PTPKG_HOST_DIST="snowleopard"
    ;;
  darwin11*)
    PTPKG_HOST_DIST="lion"
    ;;
  *)
    echo "Warning: Unknown darwin distribution: $OSTYPE"
    PTPKG_HOST_DIST="any"
    _RETVAL=1
    ;;
  esac

  return $_RETVAL
}

#
# On Linux hosts, determines the distribution
# Arguments: none
# Returns 0 if the distro was determined, 1 otherwise
#
function getLinuxDistro {
  local _DIST
  local _NUM
  local _VENDOR
  local _RETVAL=0

  declare -a _RELEASE_FILES=( 
      "/etc/system-release"
      "/etc/redhat-release"
      "/etc/fedora-release" 
      "/etc/SuSE-release"
      "/etc/lsb-release" )

  declare -a _PRODUCT_LIST=( `echo $PE_PRODUCT_LIST | tr : ' '` )

  # Try LSB
  if which lsb_release > /dev/null 2>&1 ; then
    _DIST="`lsb_release -i | cut -d: -f2`"
    _NUM="`lsb_release -r | cut -d: -f2`"
    _NUM="`echo $_NUM`"
    case "$_DIST" in
      *Fedora*)
        _DIST="fedora"
        ;;
      *SUSE*)
        _DIST="suse"
        ;;
      *RedHat*)
        _DIST="redhat"
        ;;
      *)
        echo "Warning: Unknown Linux distribution: $_DIST"
        _DIST="any"
        _NUM=""
        _RETVAL=1
        ;;
    esac
  else
    # Try the release files
    local f
    for f in ${_RELEASE_FILES[@]} ; do
      [ -r "$f" ] || continue
      read _W1 _W2 _W3 _W4 _W5 _W6 _W7 _W8 _W9 _W10 < "$f"
      case "$_W1" in
        Fedora)
          _DIST="fedora"
          _NUM="$_W3"
          ;;
        SUSE)
          _DIST="suse"
          _NUM="$_W5"
          ;;
        Red)
          if [ "$_W2" == "Hat" ] ; then
            _DIST="redhat"
            _NUM="$_W7"
          fi
          ;;
        *)
          echo "Warning: Unknown Linux distribution: $_W1"
          _DIST="any"
          _NUM=""
          _RETVAL=1
          ;;
      esac
      break
    done
  fi

  # Is this Cray CLE?
  if contains "LIBSCI" ${_PRODUCT_LIST[@]} && \
     ( contains "CRAY_MPICH2" ${_PRODUCT_LIST[@]} || contains "XTMPT" ${_PRODUCT_LIST[@]} )
  then
    _VENDOR="cray"
  fi

  PTPKG_HOST_DIST="${_DIST}${_NUM}${_VENDOR}"
  return $_RETVAL
}

#
# lists ${PTPKG_TARGETS_DIR} to discover supported targets
# Arguments: none
# Returns: none
#
function getTargets {
  validHome || return

  echo "Loading targets database from $PTPKG_TARGETS_DIR"
  PTPKG_ALL_TARGETS=( `ls "$PTPKG_TARGETS_DIR"` )
  if [ ${#PTPKG_ALL_TARGETS[@]} -eq 0 ] ; then
    exitWithError "No targets found in '${PTPKG_TARGETS_DIR}'"
  fi
}


#
# Verifies target format
# Arguments: $1: target to verify
# Returns: 0 if target format is OK, 1 otherwise
#
function verifyTarget {
  local _TARGET="$1"
  local _TARGET_DIR="${PTPKG_TARGETS_DIR}/${_TARGET}"
  [ -d "${_TARGET_DIR}" ] || return 1
  [ -r "${_TARGET_DIR}/dependencies" ] || return 1
  [ -r "${_TARGET_DIR}/environment" ] || return 1
  return 0
}

#
# Finds the directory with the name that most closely
# matches the selected target.  The match is echoed.
# Arguments: $1: top-level directory to search
# Returns: 0 if a match was found, 1 otherwise
#
function matchTargetDir {
  [ -d "$1" ] || return 1

  if [ -d "$1/${PTPKG_TARGET}" ] ; then
    echo "${PTPKG_TARGET}"
    return 0
  fi

  local _OS
  local _DIST
  local _COMP
  local _ARCH
  local _TARG

  for _OS in "$PTPKG_TARGET_OS" "any" ; do
    ls -d $1/${_OS}-*/ > /dev/null 2>&1 || continue
    for _DIST in "$PTPKG_TARGET_DIST" "any" ; do
      ls -d $1/${_OS}-${_DIST}-*/ > /dev/null 2>&1 || continue
      for _COMP in "$PTPKG_TARGET_COMP" "any" ; do
        ls -d $1/${_OS}-${_DIST}-${_COMP}-*/ > /dev/null 2>&1 || continue
        for _ARCH in "$PTPKG_TARGET_ARCH" "any" ; do
          _TARG="${_OS}-${_DIST}-${_COMP}-${_ARCH}"
          if [ -d "$1/$_TARG" ] ; then
            echo "$_TARG"
            return 0
          fi
        done
      done
    done
  done
  return 1
}

#
# Uses host target information to get the processor count, if possible
# Arguments: none
# Returns: The number of CPUs in the system, or 0 if it cannot be calculated
#
function getProcessorCount {
  [ -z "$PTPKG_HOST_OS" ] && return 0

  local _NPROCS=0
  case "$PTPKG_HOST_OS" in
    "linux")
      _NPROCS=`cat /proc/cpuinfo \
                | grep '^processor[[:space:]]*:[[:space:]]*[0-9]\+' \
                | cut -d: -f2 | sort -r | head -1`
      ((_NPROCS++))
      ;;
    "darwin")
      _NPROCS=`/usr/sbin/sysctl -n hw.ncpu`
      ;;
    "aix")
      _NPROCS=`lscfg | grep proc | wc -l`
      ;;
    *)
      _NPROCS=0
      ;;
  esac

  return $_NPROCS
}

#
# Prints all available targets, one per line
# Arguments: none
# Returns: none
#
function showAllTargets() {
  local _TARG
  banner "Available targets"
  for _TARG in ${PTPKG_ALL_TARGETS[@]} ; do
    echo "    $_TARG"
  done
}

# EOF
