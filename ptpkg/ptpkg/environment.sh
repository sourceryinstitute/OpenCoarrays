#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: rcfile.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#    bashrc and cshrc file management functions
#

# Target environment language (bash, csh, module)
PTPKG_ENV_LANG="bash"

# Environment file processing action.
# exec: The executing shell's environment will be altered as
#       specified in the environment file.
# echo: A script will be echoed that, when executed, will alter
#       the executing shell's environment.
PTPKG_ENV_ACTION="exec"

# Environment file processing mode.
# NOTE: It is the environment file's responsibility to use the 
#       envIsRun and envIsBuild functions to specify runtime and
#       buildtime environment options
# run: Runtime environment options will be processed
# build: Buildtime environment + runtime environment options will be processed
PTPKG_ENV_MODE="run"

#
# Flag function.  Indicates that environment file
# is being processed for runtime environment.
#
function envIsRun {
  test "$PTPKG_ENV_MODE" == "run"
  return $?
}

#
# Flag function.  Indicates that environment file
# is being processed for buildtime environment.
#
function envIsBuild {
  test "$PTPKG_ENV_MODE" == "build"
  return $?
}

#
# Sets an environment variable to a value
# Arguments: $1: Variable to set, $2...: Value
# Returns: none
#
function envSet {
  local cmd
  local var="$1"
  shift
  local val="$@"

  if [ "$PTPKG_ENV_ACTION" == "exec" ] ; then
    echo "export $var=\"$val\""
    export $var="$val"
  elif [ "$PTPKG_ENV_ACTION" == "echo" ] ; then
    case "$PTPKG_ENV_LANG" in
      "bash")
        echo "export $var=\"$val\""
        ;;
      "csh")
        echo "setenv $var $val"
        ;;
      "module")
        echo "setenv $var $val"
        ;;
      *)
        echo "ERROR: Unknown PTPKG_ENV_LANG: $PTPKG_ENV_LANG" 1>&2
        ;;
    esac
  else
    echo "ERROR: Unknown PTPKG_ENV_ACTION: $PTPKG_ENV_ACTION" 1>&2
  fi
}

#
# Unsets an environment variable
# Arguments: $1: Variable to unset
# Returns: none
#
function envUnset {
  local cmd
  local var="$1"

  if [ "$PTPKG_ENV_ACTION" == "exec" ] ; then
    echo "unset $var"
    unset $var
  elif [ "$PTPKG_ENV_ACTION" == "echo" ] ; then
    case "$PTPKG_ENV_LANG" in
      "bash")
        echo "unset $var"
        ;;
      "csh")
        echo "unsetenv $var"
        ;;
      "module")
        echo "unsetenv $var"
        ;;
      *)
        echo "ERROR: Unknown PTPKG_ENV_LANG: $PTPKG_ENV_LANG" 1>&2
        ;;
    esac
  else
    echo "ERROR: Unknown PTPKG_ENV_ACTION: $PTPKG_ENV_ACTION" 1>&2
  fi
}

#
# Adds a value to an environment variable
# Arguments: $1: Variable to update
#            $2: Value to add
#            $3: Optional field deliminator (default is colon ':')
#            $4: Optional append flag
# Returns: none
#
function envMunge {
  local var="$1"
  local val="$2"
  local _D=${3:-":"}
  local _FLAG="${4}"

  if [ "$PTPKG_ENV_ACTION" == "exec" ] ; then
    if [ -z "$_FLAG" ] ; then
      echo "export $var=\"$val$_D${!var}\""
      export $var="$val$_D${!var}"
    else
      echo "export $var=\"${!var}$_D$val\""
      export $var="${!var}$_D$val"
    fi
  else
    if [ "$PTPKG_ENV_LANG" != "module" ] ; then
      if [ -z "$_FLAG" ] ; then
        envSet "$var" "$val$_D\${$var}"
      else
        envSet "$var" "\${$var}$_D$val"
      fi
    else
      if [ -z "$_FLAG" ] ; then
        echo "prepend-path $var $val"
      else
        echo "append-path $var $val"
      fi
    fi
  fi
}

#
# Appends a value to an environment variable.
# Arguments: $1: Variable to append to, $2...: Value
# Returns: none
#
function envAppend {
  local var="$1"
  local val="$2"
  local i
  shift
  shift

  for i in $@ ; do
    val="$val $i"
  done
  envMunge "$var" "$val" " " "append"
}

#
# Prepends a value to an environment variable.
# Arguments: $1: Variable to append to, $2: Value
# Returns: none
#
function envPrepend {
  local var="$1"
  local val="$2"
  local i
  shift
  shift

  for i in $@ ; do
    val="$val $i"
  done
  envMunge "$var" "$val" " "
}

#
# Appends a value to a colon-seperated list.
# Arguments: $1: Variable to append to, $2: Value
# Returns: none
#
function envPathAppend {
  local var="$1"
  local val="$2"
  local i
  shift
  shift

  for i in $@ ; do
    val="$val:$i"
  done
  envMunge "$var" "$val" ":" "append"
}

#
# Prepends a value to a colon-seperated list.
# Arguments: $1: Variable to append to, $2: Value
# Returns: none
#
function envPathPrepend {
  local var="$1"
  local val="$2"
  local i
  shift
  shift

  for i in $@ ; do
    val="$val:$i"
  done
  envMunge "$var" "$val" ":"
}

#
#
# Echos a comment string
# Arguments: $1...: Comment
# Returns: none
#
function echoComment {
  if [ "$PTPKG_ENV_ACTION" == "echo" ] ; then
    echo "# $@"
  else
    echo "$@"
  fi
}

#
# Adds the installation target directory to the shell environment.
#
function loadInstallationEnvironment {
  # Load the release environment
  source "$PTPKG_RELEASE_ENV_FILE"
  
  # Prepend to PATH to override system executables
  envPathPrepend PATH "$PTPKG_INSTALL_PREFIX/bin"
  # Append to LIBRARY_PATH so system DSOs are searched first
  envPathAppend $PTPKG_TARGET_LIBRARY_PATH "$PTPKG_INSTALL_PREFIX/lib64" "$PTPKG_INSTALL_PREFIX/lib"
  # Append to PKG_CONFIG_PATH so system packages are searched first
  envPathAppend PKG_CONFIG_PATH "$PTPKG_INSTALL_PREFIX/lib64/pkgconfig" "$PTPKG_INSTALL_PREFIX/lib/pkgconfig"
  # Prepend to MANPATH to override system manpages
  envPathPrepend MANPATH "$PTPKG_INSTALL_PREFIX/man"
  envPathPrepend MANPATH "$PTPKG_INSTALL_PREFIX/share/man"
  # If we're building stuff, then add to build flags
  if envIsBuild ; then
    envPrepend CPPFLAGS "-I$PTPKG_INSTALL_PREFIX/include"
    envPrepend LDFLAGS "-L$PTPKG_INSTALL_PREFIX/lib64" "-L$PTPKG_INSTALL_PREFIX/lib"
  fi
}

#
# Writes etc/${PTPKG_TARGET}.bashrc
# Arguments: none
# Returns: 0 if the file was created successfully, 1 otherwise
#
function writeBashrc {
  local _RCFILE="$PTPKG_ETCFILE_PREFIX/${PTPKG_TARGET}.bashrc"

  # Get installed packages
  declare -a _INSTALLED=( `cd "$PTPKG_PREFIX/packages" && ls` )
  if [ ${#_INSTALLED[@]} -eq 0 ] ; then
    echo "Error: no packages found at $PTPKG_INSTALL_PREFIX"
    return 1
  fi

  # Set environment script language
  PTPKG_ENV_LANG="bash"
  # Set environment file processing action
  PTPKG_ENV_ACTION="echo"
  # Set environment mode
  PTPKG_ENV_MODE="run"

  # Initilize file by writing header
  cat > "$_RCFILE" <<EOF
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
# `echo "$PTPKG_RELEASE_DESC_SHORT" | tr '\n' ' '`
#
# File: ${PTPKG_TARGET}.bashrc
# Created: `date`
#

EOF

  (
    # Add release environment to runtime environment
    if [ -r "$PTPKG_RELEASE_ENV_FILE" ] ; then
      echo
      echoComment "$PTPKG_RELEASE_NAME configuration"
      ( source "$PTPKG_RELEASE_ENV_FILE" )
    fi

    # Add the installation to the environment
    echo
    echoComment "$PTPKG_RELEASE_NAME configuration"
    loadInstallationEnvironment

    # Write environment for each package
    for pkg in ${_INSTALLED[@]} ; do
      selectPackage $pkg
      if [ -r "$PTPKG_PKG_ENV_FILE" ] ; then
        echo
        echoComment "$pkg"
        ( source "$PTPKG_PKG_ENV_FILE" )
      fi
    done
  ) >> "$_RCFILE"

  # Write footer
  cat >> "$_RCFILE" <<EOF

#
# EOF
# File: ${PTPKG_TARGET}.bashrc
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
EOF

  echo "Wrote $_RCFILE"

  # Symlink
  rm -f "$PTPKG_ETCFILE_PREFIX/${PTPKG_RELEASE_NAME}.bashrc"
  ln -s "$_RCFILE" "$PTPKG_ETCFILE_PREFIX/${PTPKG_RELEASE_NAME}.bashrc"
}

#
# Writes etc/${PTPKG_TARGET}.cshrc
# Arguments: none
# Returns: 0 if the file was created successfully, 1 otherwise
#
function writeCshrc {
  local _RCFILE="$PTPKG_ETCFILE_PREFIX/${PTPKG_TARGET}.cshrc"

  # Get installed packages
  declare -a _INSTALLED=( `cd "$PTPKG_PREFIX/packages" && ls` )
  if [ ${#_INSTALLED[@]} -eq 0 ] ; then
    echo "Error: no packages found at $PTPKG_INSTALL_PREFIX"
    return 1
  fi

  # Set environment script language
  PTPKG_ENV_LANG="csh"
  # Set environment file processing action
  PTPKG_ENV_ACTION="echo"
  # Set environment mode
  PTPKG_ENV_MODE="run"

  # Initilize file by writing header
  cat > "$_RCFILE" <<EOF
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
# `echo "$PTPKG_RELEASE_DESC_SHORT" | tr '\n' ' '`
#
# File: ${PTPKG_TARGET}.bashrc
# Created: `date`
#

if (\$?PATH == 0) then
  setenv PATH
endif
if (\$?$PTPKG_TARGET_LIBRARY_PATH == 0) then
  setenv $PTPKG_TARGET_LIBRARY_PATH
endif
if (\$?PKG_CONFIG_PATH == 0) then
  setenv PKG_CONFIG_PATH
endif
if (\$?PYTHONPATH == 0) then
  setenv PYTHONPATH
endif
if (\$?MANPATH == 0) then
  setenv MANPATH
endif

EOF

  (
    # Add release environment to runtime environment
    if [ -r "$PTPKG_RELEASE_ENV_FILE" ] ; then
      echo
      echoComment "$PTPKG_RELEASE_NAME configuration"
      ( source "$PTPKG_RELEASE_ENV_FILE" )
    fi

    # Add the installation to the environment
    echo
    echoComment "$PTPKG_RELEASE_NAME configuration"
    loadInstallationEnvironment

    # Write environment for each package
    for pkg in ${_INSTALLED[@]} ; do
      selectPackage $pkg
      if [ -r "$PTPKG_PKG_ENV_FILE" ] ; then
        echo
        echoComment "$pkg"
        ( source "$PTPKG_PKG_ENV_FILE" )
      fi
    done
  ) >> "$_RCFILE"

  # Write footer
  cat >> "$_RCFILE" <<EOF

#
# EOF
# File: ${PTPKG_TARGET}.cshrc
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
EOF

  echo "Wrote $_RCFILE"

  # Symlink
  rm -f "$PTPKG_ETCFILE_PREFIX/${PTPKG_RELEASE_NAME}.cshrc"
  ln -s "$_RCFILE" "$PTPKG_ETCFILE_PREFIX/${PTPKG_RELEASE_NAME}.cshrc"
}



#
# Writes module file etc/$PTPKG_RELEASE_NAME/$PTPKG_TARGET
# Arguments: none
# Returns: 0 if the file was created successfully, 1 otherwise
#
function writeModule {
  local _RCFILE="$PTPKG_ETCFILE_PREFIX/$PTPKG_RELEASE_NAME/$PTPKG_TARGET"

  # Get installed packages
  declare -a _INSTALLED=( `cd "$PTPKG_PREFIX/packages" && ls` )
  if [ ${#_INSTALLED[@]} -eq 0 ] ; then
    echo "Error: no packages found at $PTPKG_INSTALL_PREFIX"
    return 1
  fi  

  # Create module folder
  mkdir -p "$PTPKG_ETCFILE_PREFIX/$PTPKG_RELEASE_NAME" || return 1

  # Set environment script language
  PTPKG_ENV_LANG="module"
  # Set environment file processing action
  PTPKG_ENV_ACTION="echo"
  # Set environment mode
  PTPKG_ENV_MODE="run"

  # Initilize file by writing header
  cat > "$_RCFILE" <<EOF
#%Module1.0#####################################################################
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
# File: ${PTPKG_TARGET}
# Created: `date`
#

proc ModulesHelp { } {
  puts stderr "\n $PTPKG_RELEASE_DESC_LONG \n"
}

module-whatis `echo "$PTPKG_RELEASE_DESC_SHORT" | tr '\n' ' '`

EOF

  (
    # Add release environment to runtime environment
    if [ -r "$PTPKG_RELEASE_ENV_FILE" ] ; then
      echo
      echoComment "$PTPKG_RELEASE_NAME configuration"
      ( source "$PTPKG_RELEASE_ENV_FILE" )
    fi

    # Add the installation to the environment
    echo
    echoComment "$PTPKG_RELEASE_NAME configuration"
    loadInstallationEnvironment

    # Write environment for each package
    for pkg in ${_INSTALLED[@]} ; do
      selectPackage $pkg
      if [ -r "$PTPKG_PKG_ENV_FILE" ] ; then
        echo
        echoComment "$pkg"
        ( source "$PTPKG_PKG_ENV_FILE" )
      fi
    done
  ) >> "$_RCFILE"

  cat >> "$_RCFILE" <<EOF
#
# EOF
# File: $PTPKG_TARGET
#
# $PTPKG_RELEASE_NAME ($PTPKG_RELEASE_VERSION)
#
EOF

  echo "Wrote $_RCFILE"
}

# EOF
