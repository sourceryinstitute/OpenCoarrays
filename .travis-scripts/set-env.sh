#!/usr/bin/env bash
# This script is part of the OpenCoarrays project. For licensing
# details please see the "LICENSE" file at the root directory of the
# project.
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

# We are sourcing this file on travis... make sure that these are not
# set in travis' environment
#set -o errexit
#set -o nounset
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
  -v --verbose     Enable verbose mode, print script as it is executed
  -d --debug       Enables debug mode
  -h --help        This page
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
#  exit 1
}

function cleanup_before_exit () {
  info "Done setting up environment."
}
#trap cleanup_before_exit EXIT


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

[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "


### Runtime
#####################################################################
if ${CI:-false} ; then
  info "Setting up environment on Travis-CI $__os image for ${BUILD:-testing} build type."
else
  info "Setting up environment for local testing on $__os for ${BUILD:-testing} build type."
fi

debug "__file: ${__file}"
debug "__dir: ${__dir}"
debug "__base: ${__base}"
debug "__os: ${__os}"

debug "arg_d: ${arg_d}"
debug "arg_v: ${arg_v}"
debug "arg_h: ${arg_h}"

### Start mapping Travis-CI vars to saner counterparts,
### using some local value if not on Travis-CI
#####################################################################

# Attempt to unshallow git, if shallow clone detected
if [[ -a .git/shallow ]]; then git fetch --unshallow; fi
debug "$(git status)"

if ! ${CI:-false}; then
  # This code would fail on travis because travis checks out detached HEAD
  _branch_name="$(git symbolic-ref -q HEAD)"
  _branch_name="${_branch_name##refs/heads/}"
  _branch_name="${_branch_name:-HEAD}"
fi

export GIT_BR=${TRAVIS_BRANCH:-"${_branch_name:-}"}
info "GIT_BR = $GIT_BR"
export BUILD_DIR=${TRAVIS_BUILD_DIR:-"${__dir}/.."}
info "BUILD_DIR = $BUILD_DIR"
export BUILD_ID=${TRAVIS_BUILD_ID:-foo-build-local-testing}
debug "BUILD_ID = $BUILD_ID"
export BUILD_NUM=${TRAVIS_BUILD_NUMBER:-0}
info "BUILD_NUM = $BUILD_NUM"
export GIT_COMMIT=${TRAVIS_COMMIT:-"$(git rev-parse HEAD)"}
info "GIT_COMMIT = $GIT_COMMIT"
# if not on travis, just use current commit as commit range
export COMMIT_RANGE=${TRAVIS_COMMIT_RANGE:-"$(git rev-parse HEAD)^..$(git rev-parse HEAD)"}
info "COMMIT_RANGE = $COMMIT_RANGE"
export JOB_ID=${TRAVIS_JOB_ID:-foo-job-local-testing}
debug "JOB_ID = $JOB_ID"
export JOB_NUMBER=${TRAVIS_JOB_NUMBER:-"0.0"}
info "JOB_NUMBER = $JOB_NUMBER"
export MY_OS=${TRAVIS_OS_NAME:-$(tr '[:upper:]' '[:lower:]' <<< "$__os")}
info "MY_OS = $MY_OS"
PR=${TRAVIS_PULL_REQUEST:-false}
[ "X$PR" = "X1" ] && PR="true"
export PR
info "PR = $PR"
_origin_url="$(git config --get remote.origin.url)"
_GH_repo_slug="$(sed 's/.*github\.com[:/]\(.*\)\(\.git\)\{0,1\}/\1/' <<< $_origin_url)"
export REPO_SLUG=${TRAVIS_REPO_SLUG:-${_GH_repo_slug}}
info "REPO_SLUG = $REPO_SLUG"
export SECURE_ENV=${TRAVIS_SECURE_ENV_VARS:-false}
info "SECURE_ENV = $SECURE_ENV"
#TRAVIS_TEST_RESULT: is set to 0 if the build is successful and 1 if the build is broken.
debug "TRAVIS_TEST_RESULT = ${TRAVIS_TEST_RESULT:-0}"
_maybe_tag="$(git describe --tags)"
# If there have been commits since the last tag (matching [vV]?[0-9]+\.[0-9]+\(\.[0-9]+\)*)
# Pull out what the last tag was
_base_tag="$(sed -n 's/^\([vV]\{0,1\}[0-9]\{1,\}\.[0-9]\{1,\}\(\.[0-9]\{1,\}\)*\).*/\1/p' <<< $_maybe_tag)"
if ${CI:-false}; then
  export GIT_TAG=${TRAVIS_TAG:-''}
elif [[ "X$_maybe_tag" = "X$_base_tag" ]]; then # No commits since last tag
  export GIT_TAG="$_base_tag"
else # Commits since last tag, not a tag
  declare GIT_TAG
  export GIT_TAG
fi
info "GIT_TAG = $GIT_TAG"

### prepare for linux brew
#####################################################################
if [[ "X$MY_OS" = "Xlinux" ]]; then
  export PATH="${HOME}/.linuxbrew/bin:$PATH"
fi

### Determine the files changed in the commit range being tested
#####################################################################

# Be carefull here; pull requests with forced pushes can potentially
# cause issues
if [ "$PR" != "false" ]; then # Use github API to get changed files
  [ "X$MY_OS" = "Xosx" ] && (brew update > /dev/null || true ; brew install jq || true)
  _files_changed=($(curl "https://api.github.com/repos/$REPO_SLUG/pulls/$PR/files" 2> /dev/null | \
			 jq '.[] | .filename' | tr '"' ' '))
  if [[ ${#_files_changed[@]} -eq 0 || -z ${_files_changed[@]} ]]; then
    info "Using git to determine changed files"
    # no files detected, try using git instead
    # This approach may only pick up files from the most recent commit, but that's
    # better than nothing
    _files_changed=($(git diff --name-only $COMMIT_RANGE | sort -u || \
                      git diff --name-only "${GIT_COMMIT}^..${GIT_COMMIT}" | sort -u || echo '<none>'))
  else
    info "Using Github API to determine changed files"
  fi
else
  info "Using git to determine changed files"
  # We should be ok using git, see https://github.com/travis-ci/travis-ci/issues/2668
  _files_changed=($(git diff --name-only $COMMIT_RANGE | sort -u || \
                    git diff --name-only "${GIT_COMMIT}^..${GIT_COMMIT}" | sort -u || echo '<none>'))
fi

FILES_CHANGED=()
for file in "${_files_changed[@]}"; do
  if [[ ! -f "$file" ]]; then
    info "File $file no longer exists, removing from list of changed files"
  else
    FILES_CHANGED+=("$file")
  fi
done

info "Files changed in $COMMIT_RANGE:"
for f in "${FILES_CHANGED[@]}"; do
  echo "    $f"
done

tmp=${FILES_CHANGED[@]}
unset FILES_CHANGED
export FILES_CHANGED=$(sort -u <<< ${tmp}) # Can't export array variables
info "Files changed: $FILES_CHANGED"

COMMITS_TESTED=($(git rev-list ${COMMIT_RANGE} || git rev-list "${GIT_COMMIT}^..${GIT_COMMIT}"))
info "The following commits are being tested in $COMMIT_RANGE:"
info "(if \`git rev-list \$COMMIT_RANGE\` fails, just use current commit)"
git rev-list ${COMMIT_RANGE} >/dev/null 2>/dev/null && info "Using \$COMMIT_RANGE" || \
    info "Unable to use \$COMMIT_RANGE"
if [ "$LOG_LEVEL" -ge 6 ]; then
  for commit in ${COMMITS_TESTED[@]}; do
    echo "    $commit"
  done
fi

tmp=${COMMITS_TESTED[@]}
unset COMMITS_TESTED
export COMMITS_TESTED=${tmp} # Can't export array variables

# Undo settings we may have set
# debug mode
set +o xtrace
# verbose mode
set +o verbose

set +o errexit
set +o nounset
set +o pipefail
