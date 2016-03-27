#!/usr/bin/env bash
# BASH3 Boilerplate
#
# build.sh
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

### Start of boilerplate -- do not edit this block #######################
if [[ -f "${B3B_USE_CASE:-}/bootstrap.sh" ]]; then
  source "${B3B_USE_CASE}/bootstrap.sh" "$@"
else
  echo "Please set B3B_USE_CASE to the bash3boilerplate use-case directory path." 
  exit 1
fi
### End of boilerplate -- start user edits below #########################


# Set up a function to call when receiving an EXIT signal to do some cleanup. Remove if
# not needed. Other signals can be trapped too, like SIGINT and SIGTERM.
function cleanup_before_exit () {
  info "Cleaning up. Done"
}
trap cleanup_before_exit EXIT # The signal is specified here. Could be SIGINT, SIGTERM etc.

### Validation (decide what's required for running your script and error out)
#####################################################################

export __flag_present=1

if [[ "${arg_l}" != "${__flag_present}" && "${arg_L}" != "${__flag_present}" &&
      "${arg_v}" != "${__flag_present}" && "${arg_h}" != "${__flag_present}" &&
      -z "${arg_p:-${arg_P:-${arg_U:-${arg_V}}}}" ]]; then
  help "${__base}: Insufficient arguments. Please pass either -l, -L, -v, -h, -p, -P, -U, -V, or a longer equivalent."
fi

# Suppress info and debug messages if -l, -P, -U, -V, or their longer equivalent is present: 
[[ "${arg_l}" == "${__flag_present}" || ! -z "${arg_P:-${arg_U:-${arg_V}}}" ]] && suppress_info_debug_messages

[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "

### Enforce mutual exclusivity of arguments that print single-line output
[ ! -z "${arg_P:-}" ] && [ ! -z "${arg_V:-}" ] && emergency "Only specify one of -P, -U, -V, or their long-form equivalents."
[ ! -z "${arg_P:-}" ] && [ ! -z "${arg_U:-}" ] && emergency "Only specify one of -P, -U, -V, or their long-form equivalents."
[ ! -z "${arg_U:-}" ] && [ ! -z "${arg_V:-}" ] && emergency "Only specify one of -P, -U, -V, or their long-form equivalents."

[ -z "${OPENCOARRAYS_SRC_DIR:-}" ] && emergency "Please set OPENCOARRAYS_SRC_DIR to the OpenCoarrays source directory path."


### Print bootstrapped magic variables to STDERR when LOG_LEVEL 
### is at the default value (6) or above.
#####################################################################

info "__file: ${__file}"
info "__dir: ${__dir}"
info "__base: ${__base}"
info "__os: ${__os}"
info "__usage: ${__usage}"
info "LOG_LEVEL: ${LOG_LEVEL}"

info "arg_b:  ${arg_b}"
info "arg_c:  ${arg_c}"
info "arg_C:  ${arg_C}"
info "arg_d:  ${arg_d}"
info "arg_e:  ${arg_e}"
info "arg_f:  ${arg_f}"
info "arg_h:  ${arg_h}"
info "arg_i:  ${arg_i}"
info "arg_I:  ${arg_I}"
info "arg_l:  ${arg_l}"
info "arg_L:  ${arg_L}"
info "arg_m:  ${arg_m}"
info "arg_M:  ${arg_M}"
info "arg_n:  ${arg_n}"
info "arg_p:  ${arg_p}"
info "arg_P:  ${arg_P}"
info "arg_t:  ${arg_t}"
info "arg_U:  ${arg_U}"
info "arg_v:  ${arg_v}"
info "arg_V:  ${arg_V}"

source "${OPENCOARRAYS_SRC_DIR:-}"/install_prerequisites/set_or_list_versions.sh
set_or_list_versions
[[ ! -z "${arg_p}" ]] && info "package (default version):  ${arg_p} (${default_version})"

