#!/usr/bin/env bash
# BASH3 Boilerplate
#
# my-script.sh
#
#  - Is a template to write better bash scripts
#  - Demonstrates a use case for the bash3boilerplate main.sh in which all boilerplate
#    functionality and variables are imported in ounce source command, which need
#    never be modified when creating new scripts from this template.
#
# Usage: LOG_LEVEL=7 B3B_USE_CASE=/opt/bash3boilerplate/src/use-case ./my-script.sh -f script_input.txt 
#
# Workflow: 
# 1. If so desired, rename the current file, e.g., to "do-something.sh". 
# 2. Rename the usage file accordingly, e.g., to "do-something.sh-usage".
# 3. Edit the (renamed) usage file. 
# 4. Edit the (renamed) current script, adding all desired functionality
#    below the the block marked "do not edit".
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

[ -z "${LOG_LEVEL:-}" ] && emergency "Cannot continue without LOG_LEVEL. "

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
