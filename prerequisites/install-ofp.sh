#!/usr/bin/env bash
# BASH3 Boilerplate
#
# install-binary.sh
#
#  - Build OpenCoarrays prerequisite packages and their prerequisites
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

export __usage="${OPENCOARRAYS_SRC_DIR}/prerequisites/install-ofp.sh-usage"

### Start of boilerplate -- do not edit this block #######################
if [[ ! -f "${B3B_USE_CASE:-}/bootstrap.sh" ]]; then
  echo "Please set B3B_USE_CASE to the bash3boilerplate use-case directory path." 
  exit 1
elif [[ ! -d "${OPENCOARRAYS_SRC_DIR:-}" ]]; then
  echo "Please set OPENCOARRAYS_SRC_DIR to the OpenCoarrays source directory path." 
  exit 2
else
  source "${B3B_USE_CASE}/bootstrap.sh" "$@"
fi
### End of boilerplate -- start user edits below #########################

# Set up a function to call when receiving an EXIT signal to do some cleanup. Remove if
# not needed. Other signals can be trapped too, like SIGINT and SIGTERM.
function cleanup_before_exit () {
  info "Cleaning up. Done"
}
trap cleanup_before_exit EXIT # The signal is specified here. Could be SIGINT, SIGTERM etc.

export __flag_present=1

# Verify requirements 

[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "

if [[ "${__os}" != "OSX" ]]; then
   info      "${__base} currently installs binaries that work only on OS X"
   emergency "To request other platforms, please submit an issue at http://github.com/sourceryinstitute/opencoarrays/issues"
fi

default_ofp_downloader=wget
# If -D is passed, print the download programs used for OFP and its prerequisites.  
# Then exit with normal status.
if [[ "${arg_D}" == "${__flag_present}" ]]; then
  echo "strategoxt-superbundle downloader: `${OPENCOARRAYS_SRC_DIR}/prerequisites/install-binary.sh -D strategoxt-superbundle`"
  echo "ofp-sdf default downloader: ${default_ofp_downloader}"
  exit 0
fi

# If -P is passed, print the default installation paths for OFP and its prerequisites.  
# Then exit with normal status.
default_ofp_install_path="${OPENCOARRAYS_SRC_DIR}/prerequisites/installations"
install_path="${arg_i:-"${default_ofp_install_path}"}"
strategoxt_superbundle_install_path="/opt"
if [[ "${arg_P}" == "${__flag_present}" ]]; then
  echo "strategoxt-superbundle default installation path: ${strategoxt_superbundle_install_path}"
  echo "ofp default installation path: ${default_ofp_install_path}"
  exit 0
fi

# If -V is passed, print the default versions of OFP and its prerequisites.  
# Then exit with normal status.
default_ofp_version=sdf
if [[ "${arg_V}" == "${__flag_present}" ]]; then
  echo "strategoxt-superbundle default version: `${OPENCOARRAYS_SRC_DIR}/prerequisites/install-binary.sh -V strategoxt-superbundle`"
  echo "ofp default version: ${default_ofp_version}"
  exit 0
fi

# If -U is passed, print the URLs for OFP and its prerequisites.  
# Then exit with normal status.
ofp_URL="https://github.com/sourceryinstitute/opencoarrays/files/213108/ofp-sdf.tar.gz"
if [[ "${arg_U}" == "${__flag_present}" ]]; then
  echo "strategoxt-superbundle URL: `${OPENCOARRAYS_SRC_DIR}/prerequisites/install-binary.sh -U strategoxt-superbundle`"
  echo "ofp URL: ${ofp_URL}"
  exit 0
fi

### Print bootstrapped magic variables to STDERR when LOG_LEVEL 
### is at the default value (6) or above.
#####################################################################

info "__file: ${__file}"
info "__dir: ${__dir}"
info "__base: ${__base}"
info "__os: ${__os}"
info "__usage: ${__usage}"
info "LOG_LEVEL: ${LOG_LEVEL}"

info "-d (--debug):             ${arg_d}"
info "-D (--print-downloaders): ${arg_D}"
info "-e (--verbose):           ${arg_e}"
info "-h (--help):              ${arg_h}"
info "-n (--no-color):          ${arg_n}"
info "-P (--print-paths):       ${arg_P}" 
info "-U (--print-URLs):        ${arg_U}"
info "-V (--print-versions):    ${arg_V}"

# Set OFP installation path to the value of the -i argument if present.
# Otherwise, install OFP in the OpenCoarrays prerequisites/installations directory.
opencoarrays_prerequisites_dir="${OPENCOARRAYS_SRC_DIR}"/prerequisites/
if [[ "${arg_i}" == "${__flag_present}" ]]; then
  install_path="${arg_i}"
else
  install_path="${opencoarrays_prerequisites_dir}"/installations
fi

ofp_prereqs_install_dir="/opt"
# Change present working directory to installation directory
if [[ ! -d "${install_path}" ]]; then
  source "${opencoarrays_prerequisites_dir}/build-functions/set_SUDO_if_needed_to_write_to_directory.sh"
  set_SUDO_if_needed_to_write_to_directory "${install_path}"
  ${SUDO:-} mkdir -p "${install_path}"
fi
# Install OFP prerequisites to /opt (currently the only option)
"${opencoarrays_prerequisites_dir}"/install-binary.sh -p strategoxt-superbundle -i "${strategoxt_superbundle_install_path}"
# Downlaod OFP
pushd "${install_path}"
${default_ofp_downloader} "${ofp_URL}"
# Uncompress OFP
tar xf ofp-sdf.tar.gz
# Return to the original working directory
popd

export SDF2_PATH="${ofp_prereqs_install_dir}"/sdf2-bundle/v2.4/bin
export ST_PATH="${ofp_prereqs_install_dir}"/strategoxt/v0.17/bin
export DYLD_LIBRARY_PATH="${ofp_prereqs_install_dir}"/strategoxt/v0.17/lib:/opt/aterm/v2.5/lib

OFP_HOME="${install_path}"/ofp-sdf
source "${opencoarrays_prerequisites_dir}"/install-binary-functions/build_parse_table.sh
build_parse_table
