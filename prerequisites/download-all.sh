#!/usr/bin/env bash
# BASH3 Boilerplate
#
# download-all-prerequisites.sh
#
#  - Download all packages required for building OpenCoarrays and its prerequisites
#
# Usage: LOG_LEVEL=7 B3B_USE_CASE=/opt/bash3boilerplate/src/use-case ./my-script.sh -f script_input.txt
#
# More info:
#
#  - https://github.com/kvz/bash3boilerplate
#  - http://kvz.io/blog/2013/02/26/introducing-bash3boilerplate/
#
# Version: 2.0.0
#
# Authors:
#
#  - Kevin van Zonneveld (http://kvz.io)
#  - Izaak Beekman (https://izaakbeekman.com/)
#  - Alexander Rathai (Alexander.Rathai@gmail.com)
#  - Dr. Damian Rouson (http://www.sourceryinstitute.org/) (documentation)
#
# Licensed under MIT
# Copyright (c) 2013 Kevin van Zonneveld (http://kvz.io)

# The invocation of bootstrap.sh below performs the following tasks:
# (1) Import several bash3boilerplate helper functions & default settings.
# (2) Set several variables describing the current file and its usage page.
# (3) Parse the usage information (default usage file name: current file's name with -usage appended).
# (4) Parse the command line using the usage information.


export OPENCOARRAYS_SRC_DIR="${OPENCOARRAYS_SRC_DIR:-${PWD%prerequisites*}}"
export __usage=${OPENCOARRAYS_SRC_DIR}/prerequisites/download-all.sh-usage
if [[ ! -f "${OPENCOARRAYS_SRC_DIR}/src/libcaf.h" ]]; then
  echo "Please run this script inside the OpenCoarrays source \"prerequisites\" subdirectory"
  echo "or set OPENCOARRAYS_SRC_DIR to the top-level OpenCoarrays source directory path."
  exit 1
fi
export B3B_USE_CASE="${B3B_USE_CASE:-${OPENCOARRAYS_SRC_DIR}/prerequisites/use-case}"
if [[ ! -f "${B3B_USE_CASE:-}/bootstrap.sh" ]]; then
  echo "Please set B3B_USE_CASE to the bash3boilerplate use-case directory path."
  exit 2
fi
# shellcheck source=./use-case/bootstrap.sh
source "${B3B_USE_CASE}/bootstrap.sh" "$@"

# Set up a function to call when receiving an EXIT signal to do some cleanup. Remove if
# not needed. Other signals can be trapped too, like SIGINT and SIGTERM.
function cleanup_before_exit () {
  info "Cleaning up. Done"
}
trap cleanup_before_exit EXIT # The signal is specified here. Could be SIGINT, SIGTERM etc.

### Validation (decide what's required for running your script and error out)
#####################################################################

export __flag_present=1

[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "

### Print bootstrapped magic variables to STDERR when LOG_LEVEL
### is at the default value (6) or above.
#####################################################################
# shellcheck disable=SC2154
{
info "__file: ${__file}"
info "__dir: ${__dir}"
info "__base: ${__base}"
info "__os: ${__os}"
info "__usage: ${__usage}"
info "LOG_LEVEL: ${LOG_LEVEL}"

info "-b (--install-branch):   ${arg_b} "
info "-d (--debug):            ${arg_d} "
info "-e (--verbose):          ${arg_e} "
info "-h (--help):             ${arg_h} "
info "-l (--list-branches):    ${arg_l} "
}

download_list=( "m4" "bison" "flex" "mpich" "cmake" )

for package_to_download in "${download_list[@]}" ; do
  if [[ "${arg_l}" == "${__flag_present}" ]]; then
    info "${package_to_download}"
  else
    ./install.sh --package ${package_to_download} --only-download
  fi
done
  
if [[ ! -z ${install_branch:-}  ]]; then
  ./install.sh --package gcc  --only-download --install-branch "${arg_b}"
else  # Download default version
  ./install.sh --package gcc  --only-download
fi

pushd prerequisites/downloads/trunk

  source ../../build-functions/set_or_print_downloader.sh
  set_or_print_downloader

  source ../../build-functions/edit_GCC_download_prereqs_file_if_necessary.sh
  edit_GCC_download_prereqs_file_if_necessary

  ./contrib/download_prerequisites

popd
