#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: common.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description: ptpkg common functionality
#

#
# Script exit on error
# Arguments: $1: an error message, [$2: an error code]
# Returns: none
#
function exitWithError {
  local _CODE
  if [ $# -gt 1 ] ; then
    _CODE=$2
  else
    _CODE=100
  fi
  echo "$1"
  exit ${_CODE}
}

#
# Displays a horizontal line
# Arguments: none
# Returns: none
#
function hline {
  echo "--------------------------------------------------------------------------------"
}

#
# Displays a section banner
# Arguments: Banner message
# Returns: none
#
function banner {
  echo
  hline
  echo "  $1"
  hline
  echo
}

#
# Echos all arguments under a banner
# Based on columnize.sh by Loïc Cattani "Arko" <loic cattani at gmail com>
# Arguments: $1: Banner message, $2...: Strings to echo
# Returns: None
#
function echoAll {
  banner "$1"
  shift

  declare -a values=( $* )
  local longest_value=0
  local value

  # Find the longest value
  for value in ${values[@]}
  do
    if [[ ${#value} -gt $longest_value ]]; then
      longest_value=${#value}
    fi
  done

  # Compute terminal width
  local term_width=`tput cols 2>/dev/null`
  if [ -z "$term_width" ] ; then
    term_width=80
  elif [ $term_width -gt 80 ] ; then
    term_width=80
  fi

  # Compute column span
  local columns
  (( columns = $term_width / ($longest_value + 2) ))

  # Print values with pretty column width
  local curr_col=0
  for value in ${values[@]}
  do
    value_len=${#value}
    echo -n "  $value"
    local spaces_missing
    (( spaces_missing = $longest_value - $value_len + 2 ))
    printf "%*s" $spaces_missing
    (( curr_col++ ))
    if [[ $curr_col == $columns ]]; then
      echo
      curr_col=0
    fi
  done

  # Make sure there is a newline at the end
  if [[ $curr_col != 0 ]]; then
    echo
  fi
}

#
# Checks that PTPKG_HOME is a valid directory and that it
# contains the expected subfolders
# Arguments: none
# Returns: none
#
function validHome {
  if [ -z "${PTPKG_HOME}" ] ; then
    exitWithError "ERROR: PTPKG_HOME environment variable is not set."
  elif ! [ -d "${PTPKG_HOME}" ] ; then
    exitWithError "ERROR: PTPKG_HOME is not set to a valid directory."
  elif ! [ -d "${PTPKG_SRC_DIR}" ] ; then
    exitWithError "ERROR: PTPKG_HOME does not contain a ptpkg directory."
  elif ! [ -d "${PTPKG_TARGETS_DIR}" ] ; then
    exitWithError "ERROR: PTPKG_HOME does not contain a targets directory."
  elif ! [ -d "${PTPKG_PACKAGES_DIR}" ] ; then
    exitWithError "ERROR: PTPKG_HOME does not contain a packages directory."
  fi
}

#
# Determines if the first argument appears more than once in the arguments
# Arguments: $1: search value, $2..: values to search
# Returns: 0 if $1 appears at least twice in the arguments, 1 otherwise
#
function contains {
  local val="$1"
  shift
  [[ " $@ " =~ " $val " ]]
  return $?
}

#
# Checks if the arguments are numbers
# Arguments: $1...: strings to check
# Returns: 0 if all arguments are numbers,
#          1 if at least one argument is non-numeric
#
function isNumeric {
  [ $# -gt 0 ] || return 1
  while [ $# -gt 0 ] ; do
    [ "$1" -eq "$1" 2> /dev/null ] || return 1
    shift
  done
  return 0
}

#
# Checks if the arguments are absolute paths
# Arguments: $1...: strings to check
# Returns: 0 if all arguments are absolute paths, 1 otherwise
#
function isAbsolutePath {
  [ $# -gt 0 ] || return 1
  while [ $# -gt 0 ] ; do
    [ "${1:0:1}" == "/" ] || return 1
    shift
  done
  return 0
}

#
# Removes duplicates from the arguments.  The result is
# in the PTPKG_REMOVEDUPLICATES_RESULT global array on exit.
# Arguments: $1...: Values to search for duplicates
# Returns: none
#
function removeDuplicates {
  PTPKG_REMOVEDUPLICATES_RESULT=( )
  local x
  for x in $@ ; do
    if ! contains "$x" ${PTPKG_REMOVEDUPLICATES_RESULT[@]} ; then
      PTPKG_REMOVEDUPLICATES_RESULT=( ${PTPKG_REMOVEDUPLICATES_RESULT[@]} "$x" )
    fi
  done
}

#
# Determines if an integer value is within acceptable limits.
# Arguments: $1: lower, $2: value, $3: upper
# Returns: 0 if lower <= value <= upper, 1 otherwise
#
function checkLimits {
  if isNumeric $@ && [ $# -eq 3 ] && [ $1 -le $2 ] && [ $2 -le $3 ] ; then
    return 0
  fi
  return 1
}

#
# Indicates if a directory exists and contains files
# Arguments: $1: Path to directory
# Returns: 0 if the directory contains files, 1 otherwise
#
function containsFiles {
  local _DIR="$1"
  [ -d "$_DIR" ] && [ "$(ls -A $_DIR 2>/dev/null)" ] && return 0
  return 1
}

#
# Extracts an archive file to a destination
# Arguments: $1: file to extract, $2: destination directory
# Returns: 0 if no problems were encountered, 1 otherwise
#
function unpack {
  local _ARCHIVE="$1"
  local _DEST="$2"

  echo "Unpacking ${_ARCHIVE} to ${_DEST}"

  if ! [ -d "${_DEST}" ] || ! [ -w "${_DEST}" ] ; then
    echo "Error: cannot write to directory ${_DEST}"
    return 1
  fi

  # Determine archive format from file extension and extract
  # Use old form to humor AIX
  if [ -r ${_ARCHIVE} ] ; then
    if [ "${_ARCHIVE:(-4)}" == ".tgz" ] || [ "${_ARCHIVE:(-7)}" == ".tar.gz" ] ; then
      ( cd "$_DEST" && $PTPKG_TAR_CMD xzpf "$_ARCHIVE" ) || return 1
    elif [ "${_ARCHIVE:(-8)}" == ".tar.bz2" ] ; then
      ( cd "$_DEST" && $PTPKG_TAR_CMD xjpf "$_ARCHIVE" ) || return 1
    elif [ "${_ARCHIVE:(-4)}" == ".zip" ] ; then
      unzip "$_ARCHIVE" -d "$_DEST" || return 1
    else
      echo "Unknown archive file format: $_ARCHIVE"
      return 1
    fi
  else
    echo "Invalid package archive file: $_ARCHIVE"
    return 1
  fi

  return 0
}

# EOF
