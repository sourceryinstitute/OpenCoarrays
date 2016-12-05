#!/bin/bash
#
# build-ofp
#
# -- This script downloads and installs Open Fortran Praser and its prerequisites
#
# OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
# Copyright (c) 2015-2016, Sourcery, Inc.
# Copyright (c) 2015-2016, Sourcery Institute
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
this_script=`basename $0`

# Download and uncompress the ofp-sdf tar ball
echo "$this_script: Downloading ofp-sdf"
wget https://dl.dropboxusercontent.com/u/7038972/ofp-sdf.tar.bz2
if [[ -f ofp-sdf.tar.bz2 ]]; then
  echo "$this_script: Download of ofp-sdf succeeded."
else
  echo "$this_script: Download of ofp-sdf failed."
  exit 10
fi
echo "$this_script: Uncompressing ofp-sdf."
tar xf ofp-sdf.tar.bz2
if [[ -d "ofp-sdf" ]]; then
  echo "$this_script: Decompression of ofp-sdf succeeded."
  OFP_HOME=${PWD}/ofp-sdf
else
  echo "$this_script: Decompression of ofp-sdf failed."
fi

# Download and uncompress the aterm tar ball
echo "$this_script: Downloading aterm"
wget http://releases.strategoxt.org/strategoxt-0.17/aterm/aterm-2.5pre21238-26ra85lr/aterm-2.5.tar.gz
echo "$this_script: Uncompressing sf2-bundle"
if [[ -f "aterm-2.5.tar.gz" ]]; then
  echo "$this_script: Download of aterm succeeded."
else
  echo "$this_script: Download of aterm failed. [exit 20]"
  exit 20
fi
tar xf aterm-2.5.tar.gz
if [[ -d "aterm-2.5" ]]; then
  echo "$this_script: Decompression of aterm-2.5 succeeded."
else
  echo "$this_script: Decompression of aterm-2.5 failed."
fi

# Download and uncompress the sf2-bundle tar ball
echo "$this_script: Downloading sf2-bundle"
wget http://releases.strategoxt.org/strategoxt-0.17/sdf2-bundle/sdf2-bundle-2.4pre212034-1ax0hbra/sdf2-bundle-2.4.tar.gz
if [[ -f "aterm-2.5.tar.gz" ]]; then
  echo "$this_script: Download of sdf2-bundle succeeded."
else
  echo "$this_script: Download of sdf2-bundle failed. [exit 30]"
  exit 30
fi
echo "$this_script: Uncompressing sf2-bundle"
tar xf sdf2-bundle-2.4.tar.gz
if [[ -d "sdf2-bundle-2.4" ]]; then
  echo "$this_script: Decompression of sdf2-bundle-2.4 succeeded."
else
  echo "$this_script: Decompression of sdf2-bundle-2.4 failed. [exit 40]"
  exit 40
fi

# Download and uncompress the strategoxt tar ball
echo "$this_script: Downloading strategoxt-0.17"
wget http://ftp.strategoxt.org/pub/stratego/StrategoXT/strategoxt-0.17/strategoxt-0.17.tar.gz
echo "$this_script: Uncompressing strategoxt-0.17"
tar xf strategoxt-0.17.tar.gz
if [[ -d "strategoxt-0.17" ]]; then
  echo "$this_script: Decompression of strategoxt-0.17 succeeded."
else
  echo "$this_script: Decompression of strategoxt-0.17 failed. [exit 50]"
  exit 50
fi

# Build everything
echo "Building parse table"
cd $OFP_HOME/fortran/syntax
make
cd $OFP_HOME/fortran/trans
make
