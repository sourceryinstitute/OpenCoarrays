#!/usr/bin/env bash
# This script is part of the OpenCoarrays project. For licensing
# details please see the "LICENSE" file at the root directory of the
# project.
#
# This script is used to bootstrap dependencies on
#
# AUTHORS: Izaak B Beekman (https://izaakbeekman.com)
#
# Copyright (c) 2016 SourceryInstitute
#
# This script was bootstrapped using the bash3boilerplate project,
# Licensed under MIT and carries the following copyright notice:
# Copyright (c) 2013 Kevin van Zonneveld (http://kvz.io)


### Configuration
#####################################################################

# Exit on error. Append ||true if you expect an error.
# `set` is safer than relying on a shebang like `#!/bin/bash -e` because that is neutralized
# when someone runs your script as `bash yourscript.sh`
set -o errexit
set -o nounset

# Bash will remember & return the highest exitcode in a chain of pipes.
# This way you can catch the error in case mysqldump fails in `mysqldump |gzip`
set -o pipefail
# set -o xtrace

# Environment variables and their defaults
LOG_LEVEL="${LOG_LEVEL:-6}" # 7 = debug -> 0 = emergency
NO_COLOR="${NO_COLOR:-}"    # true = disable color. otherwise autodetected

if [[ $LOG_LEVEL = 7 ]] && ${CI:-false}; then
  set -o verbose
fi

# Commandline options. This defines the usage page, and is used to parse cli
# opts & defaults from. The parsing is unforgiving so be precise in your syntax
# - A short option must be preset for every long option; but every short option
#   need not have a long option
# - `--` is respected as the separator between options and arguments
read -r -d '' usage <<-'EOF' || true # exits non-zero when EOF encountered
  -b --build   [arg] Set build type to setup. Required.
  -m --mpi-lib [arg] Set MPI library to use. Default="mpich"
  -v --verbose       Enable verbose mode, print script as it is executed
  -d --debug         Enables debug mode
  -h --help          This page
EOF

# Set magic variables for current file and its directory.
# BASH_SOURCE[0] is used so we can display the current file even if it is sourced by a parent script.
# If you need the script that was executed, consider using $0 instead.
__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
__file="${__dir}/$(basename "${BASH_SOURCE[0]}")"
__base="$(basename ${__file} .sh)"
__os="Linux"
if [[ "${OSTYPE:-}" == "darwin"* ]]; then
  __os="OSX"
fi

### Functions
#####################################################################

function bottle_install () {
    brew install $1 --only-dependencies || true # try to keep going
    brew install $1 --force-bottle || \
      brew upgrade $1 --force-bottle || \
      brew outdated $1 # throw an error if outdated or not installed
}

function brew_install () {
    brew install $1 || brew upgrade $1 || brew outdated $1
}

function _fmt ()      {
  local color_debug="\x1b[35m"
  local color_info="\x1b[32m"
  local color_notice="\x1b[34m"
  local color_warning="\x1b[33m"
  local color_error="\x1b[31m"
  local color_critical="\x1b[1;31m"
  local color_alert="\x1b[1;33;41m"
  local color_emergency="\x1b[1;4;5;33;41m"
  local colorvar=color_$1

  local color="${!colorvar:-$color_error}"
  local color_reset="\x1b[0m"
  if [ "${NO_COLOR}" = "true" ] || [[ "${TERM:-}" != "xterm"* ]] || [ -t 1 ]; then
    # Don't use colors on pipes or non-recognized terminals
    color=""; color_reset=""
  fi
  echo -e "$(date -u +"%Y-%m-%d %H:%M:%S UTC") ${color}$(printf "[%9s]" ${1})${color_reset}";
}
function emergency () {                             echo "$(_fmt emergency) ${@}" 1>&2 || true; exit 1; }
function alert ()     { [ "${LOG_LEVEL}" -ge 1 ] && echo "$(_fmt alert) ${@}" 1>&2 || true; }
function critical ()  { [ "${LOG_LEVEL}" -ge 2 ] && echo "$(_fmt critical) ${@}" 1>&2 || true; }
function error ()     { [ "${LOG_LEVEL}" -ge 3 ] && echo "$(_fmt error) ${@}" 1>&2 || true; }
function warning ()   { [ "${LOG_LEVEL}" -ge 4 ] && echo "$(_fmt warning) ${@}" 1>&2 || true; }
function notice ()    { [ "${LOG_LEVEL}" -ge 5 ] && echo "$(_fmt notice) ${@}" 1>&2 || true; }
function info ()      { [ "${LOG_LEVEL}" -ge 6 ] && echo "$(_fmt info) ${@}" 1>&2 || true; }
function debug ()     { [ "${LOG_LEVEL}" -ge 7 ] && echo "$(_fmt debug) ${@}" 1>&2 || true; }

function help () {
  echo "" 1>&2
  echo " ${@}" 1>&2
  echo "" 1>&2
  echo "  ${usage}" 1>&2
  echo "" 1>&2
  exit 1
}

function cleanup_before_exit () {
  info "Cleaning up. Done"
}
trap cleanup_before_exit EXIT


### Parse commandline options
#####################################################################

# Translate usage string -> getopts arguments, and set $arg_<flag> defaults
while read line; do
  # fetch single character version of option string
  opt="$(echo "${line}" |awk '{print $1}' |sed -e 's#^-##')"

  # fetch long version if present
  long_opt="$(echo "${line}" |awk '/\-\-/ {print $2}' |sed -e 's#^--##')"
  long_opt_mangled="$(sed 's#-#_#g' <<< $long_opt)"

  # map long name back to short name
  varname="short_opt_${long_opt_mangled}"
  eval "${varname}=\"${opt}\""

  # check if option takes an argument
  varname="has_arg_${opt}"
  if ! echo "${line}" |egrep '\[.*\]' >/dev/null 2>&1; then
    init="0" # it's a flag. init with 0
    eval "${varname}=0"
  else
    opt="${opt}:" # add : if opt has arg
    init=""  # it has an arg. init with ""
    eval "${varname}=1"
  fi
  opts="${opts:-}${opt}"

  varname="arg_${opt:0:1}"
  if ! echo "${line}" |egrep '\. Default=' >/dev/null 2>&1; then
    eval "${varname}=\"${init}\""
  else
    match="$(echo "${line}" |sed 's#^.*Default=\(\)#\1#g')"
    eval "${varname}=\"${match}\""
  fi
done <<< "${usage}"

# Allow long options like --this
opts="${opts}-:"

# Reset in case getopts has been used previously in the shell.
OPTIND=1

# start parsing command line
set +o nounset # unexpected arguments will cause unbound variables
               # to be dereferenced
# Overwrite $arg_<flag> defaults with the actual CLI options
while getopts "${opts}" opt; do
  [ "${opt}" = "?" ] && help "Invalid use of script: ${@} "

  if [ "${opt}" = "-" ]; then
    # OPTARG is long-option-name or long-option=value
    if [[ "${OPTARG}" =~ .*=.* ]]; then
      # --key=value format
      long=${OPTARG/=*/}
      long_mangled="$(sed 's#-#_#g' <<< $long)"
      # Set opt to the short option corresponding to the long option
      eval "opt=\"\${short_opt_${long_mangled}}\""
      OPTARG=${OPTARG#*=}
    else
      # --key value format
      # Map long name to short version of option
      long_mangled="$(sed 's#-#_#g' <<< $OPTARG)"
      eval "opt=\"\${short_opt_${long_mangled}}\""
      # Only assign OPTARG if option takes an argument
      eval "OPTARG=\"\${@:OPTIND:\${has_arg_${opt}}}\""
      # shift over the argument if argument is expected
      ((OPTIND+=has_arg_${opt}))
    fi
    # we have set opt/OPTARG to the short value and the argument as OPTARG if it exists
  fi
  varname="arg_${opt:0:1}"
  default="${!varname}"

  value="${OPTARG}"
  if [ -z "${OPTARG}" ] && [ "${default}" = "0" ]; then
    value="1"
  fi

  eval "${varname}=\"${value}\""
  debug "cli arg ${varname} = ($default) -> ${!varname}"
done
set -o nounset # no more unbound variable references expected

shift $((OPTIND-1))

[ "${1:-}" = "--" ] && shift


### Switches (like -d for debugmode, -h for showing helppage)
#####################################################################

# debug mode
if [ "${arg_d}" = "1" ]; then
  set -o xtrace
  LOG_LEVEL="7"
fi

# verbose mode
if [ "${arg_v}" = "1" ]; then
  set -o verbose
fi

# help mode
if [ "${arg_h}" = "1" ]; then
  # Help exists with code 1
  help "Help using ${0}"
fi


### Validation (decide what's required for running your script and error out)
#####################################################################

[ -z "${arg_b:-}" ]     && help      "Setting a build type with -b or --build is required"
[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "


### Runtime
#####################################################################
if ${CI:-false} ; then
  info "Installing prerequisite software on Travis-CI $__os image for ${arg_b:-testing} build type."
else
  info "Installing prerequisite software for local testing on $__os for ${arg_b:-testing} build type."
fi

debug "__file: ${__file}"
debug "__dir: ${__dir}"
debug "__base: ${__base}"
debug "__os: ${__os}"

debug "arg_b: ${arg_b}"
debug "arg_m: ${arg_m}"
debug "arg_d: ${arg_d}"
debug "arg_v: ${arg_v}"
debug "arg_h: ${arg_h}"

### Install prerequisite software via Homebrew
#####################################################################

info "All dependencies are managed with Homebrew or Linuxbrew"

if [[ "X$MY_OS" = "Xlinux" ]]; then
  if [ ! -x "${HOME}/.linuxbrew/bin/brew" ]; then # Linux brew *NOT* installed in cache
    info "Linux brew not installed/cached. Installing linux brew now..."
    yes | ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/linuxbrew/go/install)" || true
    info "Done installing Linux brew!"
    info "Updating formula library..."
    brew update > /dev/null
    info "Done updating formula library."
    if [ "${LOG_LEVEL}" -ge 6 ]; then # do some debugging
      info "Checking health of Linux brew using \`brew doctor\`..."
      brew doctor || true
    fi
  else
    info "Linux brew appears to already be installed"
  fi
else
  info "Updating formula library..."
  brew update > /dev/null
  info "Done updating formula library."
fi

info "Installing latest GCC, this could take some time..."
[ "X$MY_OS" = "Xosx" ] && brew unlink gcc cmake
bottle_install gcc
cd $(brew --cellar)/gcc/5.3.0/bin
[ -f "gcc" ] || ln -s gcc-5 gcc
[ -f "gfortran" ] || ln -s gfortran-5 gfortran
[ -f "g++" ] || ln -s g++-5 g++
cd -
brew unlink gcc || true
brew link --force gcc || true
info "Done installing gcc, version: $(gcc --version)"

[ "X$MY_OS" = "Xlinux" ] && brew_install homebrew/dupes/openldap # needed for curl, needed for CMake

# Use FILES_CHANGED defined in set-env.sh to determine if we need to test installation of
# prerequisites with build script
info "Files changed: $FILES_CHANGED"
echo "${BUILD_DIR}/install.sh" > script_files.txt
for f in install_prerequisites/*; do
  echo "$f" >> script_files.txt
done
info "Script files:"
cat script_files.txt
if grep -F -f script_files.txt <<< "${FILES_CHANGED}" > /dev/null ; then
  info "Detected changes to install scripts"
  _install_script_touched=true
else
  info "No changes to install scripts detected, installing CMake & MPI via package manager"
fi

# if debugging show what we thing changed
debug "Here are the FILES_CHANGED:"
if [ "$LOG_LEVEL" -ge 7 ]; then
  for f in ${FILES_CHANGED}; do
      echo "    $f"
  done
fi

if ([[ "X$arg_b" = "XScript" ]] && ${_install_script_touched:-false}); then
  # Don't install CMake or MPI implementation
  info "Install script files \`install.sh\` and/or stuff in \`install_prerequisites\` has changed."
  info "Not installing CMake or MPI implementation so that the install script may be fully tested."
elif [[ "X$arg_b" = "XLint" ]]; then
  info "Installing packages for linting recent changes"
else
  info "Normal build without changes to install script. Installing CMake and $arg_m."
  bottle_install cmake
  if [[ "X$MY_OS" = "Xosx" ]]; then
    # Set custom OSX bottle download locations, used only if needed
    mpich="https://github.com/sourceryinstitute/opencoarrays/files/64308/mpich-3.2.yosemite.bottle.1.tar.gz"
    openmpi="https://github.com/sourceryinstitute/opencoarrays/files/64404/open-mpi-1.10.1_1.yosemite.bottle.1.tar.gz"
    bottle_install $arg_m || (curl -O "${!arg_m}" && brew install "${!arg_m##*/}")
    ls -l $(which mpif90)
    mpif90 --version
    ls -l $(which mpicc)
    mpicc --version
    ls -l $(which mpicxx)
    mpicxx --version
  else
    # We can upload a custom Linux MPICH bottle, but we'll need to patch mpich.rb too...
      bottle_install $arg_m
      ls -l $(which mpif90)
      mpif90 --version
      ls -l $(which mpicc)
      mpicc --version
      ls -l $(which mpicxx)
      mpicxx --version
  fi
fi

if ! ${CI:-false}; then
  info "Attempting to remove outdated packages"
  [ "X$MY_OS" = "Xlinux" ] && brew cleanup --force || info "No outdated packages found"
fi

info "The following packages are installed via Homebrew/Linuxbrew:"
brew list
brew doctor || true
