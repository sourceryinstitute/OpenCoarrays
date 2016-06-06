#!/bin/bash
#
# gitkeep.sh
#

description=\
'bash script for adding empty ".gitkeep" files at the branch ends of empty directory trees'

use_case=\
"Use case:

    Force git to track otherwise empty directories such as doc/dependency_tree/opencoarrays,
    which exists solely for purposes of displaying the OpenCoarrays dependency tree via the
    command 'tree opencoarrays'."
#
# OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
# Copyright (c) 2015, 2016, Sourcery, Inc.
# Copyright (c) 2015, 2016, Sourcery Institute
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice, this
#    list of conditions and the following disclaimer in the documentation and/or
#    other materials provided with the distribution.
# 3. Neither the names of the copyright holders nor the names of their contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
this_script=`basename $0`

usage()
{
    echo ""
    echo " $this_script - $description"
    echo ""
    echo " Usage: "
    echo "      $this_script <path-to-modify>"
    echo ""
    echo " Examples:"
    echo ""
    echo "   $this_script ."
    echo ""
    printf "$use_case"
    exit 1
}

# If this script is invoked without arguements, print usage information
# and terminate execution of the script.
if [[ $# == 0 || "$1" == "-h" || "$1" == "--help" ]]; then
  usage | less
  exit 1
fi

# Interpret the first argument as the name of the directory tree to fill
export path_to_modify=$1

# Create an empty ".gitkeep" file in all empty subdirectories
find $path_to_modify -type d -empty -exec touch {}/.gitkeep \;
