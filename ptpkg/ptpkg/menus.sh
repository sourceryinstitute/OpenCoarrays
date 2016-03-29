#
# ParaTools Packages
#
# Copyright (c) ParaTools, Inc.
#
# File: menus.sh
# Created: 15 Dec. 2011
# Author: John C. Linford (jlinford@paratools.com)
#
# Description:
#     Interactive command line menus written in
#     100% pure Bash shell script.
#

# Use shell script loader
include common.sh

declare -a PTPKG_FILTERMENU_SELECTIONS

#
# Prompts the user for a key press
# Arguments: none
# Returns: none
#
function promptKeyPress {
  read -p "Press any key to continue:" -n 1 _dummy
  echo
}

#
# Prompts the user for a yes/no response
# Arguments: $1: prompt string
# Returns: 0 if the user indicated "yes", 1 if the user indicated "no"
#
function promptYesNo {
  local _INPUT
  local _PROMPT="${1-(y/n)}"
  while true ; do
    read -p "$_PROMPT: " _INPUT
    case "$_INPUT" in
    y|Y|yes|YES)
      return 0
      ;;
    n|N|no|NO)
      return 1
      ;;
    *)
      echo "Please answer y[es] or n[o]"
      ;;
    esac
  done
}

#
# Prompts the user for a directory path.  The user's input is checked
# for validity and, if valid, returned as PTPKG_PROMPTFORDIR_RESULT.
# Arguments: $1: optional prompt string,
#            $2: optional default value,
#            $3: optional flag to allow non-existant dir to be accepted input
# Returns: none
#
function promptForDir {
  PTPKG_PROMPTFORDIR_RESULT=""
  local _PROMPT="${1-Please enter an directory path}"
  local _DEFAULT="$2"
  local _FLAG="$3"
  local _INPUT

  if [ -n "$_DEFAULT" ] ; then
    _PROMPT="$_PROMPT [$_DEFAULT]"
  fi

  while [ -z "$PTPKG_PROMPTFORDIR_RESULT" ] ; do
    read -p "$_PROMPT: " _INPUT
    if [ -z "$_INPUT" ] && [ -n "$_DEFAULT" ] ; then
      _INPUT="$_DEFAULT"
    fi

    if [ -z "$_FLAG" ] ; then
      if [ -d "$_INPUT" ] ; then
        PTPKG_PROMPTFORDIR_RESULT="$_INPUT"
      else
        echo "'$_INPUT' is not a valid directory."
      fi
    else
      if ! [ -z "$_INPUT" ] ; then
        PTPKG_PROMPTFORDIR_RESULT="$_INPUT"
      fi
    fi
  done
}

#
# Displays a multi-choice menu and returns all the user's selections.
# Users selections are in PTPKG_FILTERMENU_SELECTIONS on exit.
# Arguments: $1: Menu title
#            $2: Name of an array variable containing selections.
#            $3: Name of an array variable containing defaults.
# Returns: None
#
function filterMenu {
  declare -a _ITEMS=( `eval echo \$\{$2\[@\]\}` )
  declare -a _SELECTED=( `eval echo \$\{$3\[@\]\}` )
  declare -a _NEW_SELECTED
  declare -a _BOXED_ITEMS

  local _TITLE="$1"
  local _MAXITEM=${#_ITEMS[@]}
  local _ITEM=""
  local _DEFAULTS=""
  local _INPUT=""
  local _PTR
  local i
  local j

  # Clear invalid defaults
  for i in ${_SELECTED[@]} ; do
    if contains "$i" ${_ITEMS[@]} ; then
      _DEFAULTS="${_DEFAULTS} $i"
    fi
  done
  _SELECTED=( `echo "$_DEFAULTS"` )

  # Display menu and get selections
  while true ; do

    # Show menu title
    banner "$_TITLE"

    # Show the menu
    local _NUMS=""
    _BOXED_ITEMS=( )
    for ((i=0; i < ${_MAXITEM}; i++)) {
      if contains "${_ITEMS[$i]}" ${_SELECTED[@]} ; then
        _BOXED_ITEMS=( "${_BOXED_ITEMS[@]}" "[X]  ${_ITEMS[$i]}" )
        _NUMS="${_NUMS} $i"
      else
        _BOXED_ITEMS=( "${_BOXED_ITEMS[@]}" "[ ]  ${_ITEMS[$i]}" )
      fi
    }

    # Get input from user
    local _PS3=${PS3:-"#? "}
    PS3="Enter numbers or 'OK' to accept ('+N' and '-N' modify selection): "
    select _DUMMY in "${_BOXED_ITEMS[@]}" ; do
      _INPUT="$REPLY"
      break;
    done
    PS3="$_PS3"

    # Break loop if selections have been finalized
    if [ "$_INPUT" == "OK" ] || [ "$_INPUT" == "ok" ]; then
      if [ ${#_SELECTED[@]} -gt 0 ] ; then
        break
      else
        echo "ERROR: No packages selected."
        continue
      fi
    fi

    # Check user's input for validity and update selections
    _PTR=0
    for i in ${_INPUT} ; do
      case $i in
      # Item has + prefix
      +*)
        i=${i:1} # Remove prefix
        if checkLimits 1 $i $_MAXITEM ; then
          ((_IDX=$i-1))
          _ITEM="${_ITEMS[$_IDX]}"
          if ! contains "${_ITEM}" ${_SELECTED[@]} ; then
            _SELECTED=( ${_SELECTED[@]} ${_ITEM} )
          fi
          continue
        fi
        ;;
      # Item has - prefix
      -*)
        i=${i:1} # Remove prefix
        if checkLimits 1 $i $_MAXITEM ; then
          ((_IDX=$i-1))
          _ITEM="${_ITEMS[$_IDX]}"
          for ((j=0; j < ${#_SELECTED[@]}; j++)) {
            if [ "${_ITEM}" == "${_SELECTED[$j]}" ] ; then
              unset _SELECTED[$j]
              break
            fi
          }
          continue
        fi
        ;;
      # Item has no prefix
      *)
        if checkLimits 1 $i $_MAXITEM ; then
          ((_IDX=$i-1))
          _NEW_SELECTED[${_PTR}]="${_ITEMS[$_IDX]}"
          ((_PTR++))
          continue
        fi
        ;;
      esac
      echo "Invalid selection: $i"
    done
    if [ ${#_NEW_SELECTED[@]} -gt 0 ] ; then
      _SELECTED=( ${_NEW_SELECTED[@]} )
      _NEW_SELECTED=( )
      echo "Replaced _SELECTED with _NEW_SELECTED"
    fi

  done

  # Return selections in global variable
  PTPKG_FILTERMENU_SELECTIONS=( ${_SELECTED[@]} )
  return 0
}


