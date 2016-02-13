#!/bin/sh
#
# installStrategoXT.sh
#
# -- This script downloads and installs the Open Fortran Parser and its prerequisites
#
# OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
# Copyright (c) 2016, Sourcery, Inc.
# Copyright (c) 2016, Sourcery Institute
# Copyright (c) 2016, Soren Rasmussen
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

# Default installation paths:
PREFIX=${PWD}
ATERMPREFIX=${PREFIX}/aterm
SDF2PREFIX=${PREFIX}/sdf2-bundle
STRATEGOXTPREFIX=${PREFIX}/strategoxt


STRATEGOXTDIR=strategoxt-0.17
STRATEGOXTTAR=${STRATEGOXTDIR}.tar.gz
STRATEGOXTURL=http://ftp.strategoxt.org/pub/stratego/StrategoXT/strategoxt-0.17/${STRATEGOXTTAR}
ATERMDIR=aterm-2.5
ATERMTAR=${ATERMDIR}.tar.gz
ATERMURL=http://releases.strategoxt.org/strategoxt-0.17/aterm/aterm-2.5pre21238-26ra85lr/${ATERMTAR}
ATERMURLALT=http://ftp.strategoxt.org/pub/stratego/sdf2/sdf2-bundle-2.3.3/sdf2-bundle-2.3.3.tar.gz
SDF2DIR=sdf2-bundle-2.4
SDF2TAR=${SDF2DIR}.tar.gz
SDF2URL=http://releases.strategoxt.org/strategoxt-0.17/sdf2-bundle/sdf2-bundle-2.4pre212034-37nm9z7p/${SDF2TAR}
SDF2URLALT=http://ftp.strategoxt.org/pub/stratego/sdf2/sdf2-bundle-2.3/sdf2-bundle-2.3.tar.gz

# Installing in 32 bit is important!
CFLAGS="-m32"

while [[ $# > 1 ]] ; do
key="$1"
arg="$2"

case $key in 
    # TO DO: help section
    -h)
        echo "Help Section", $key
        ;;
    -help)
        echo "Help Section", $key
        ;;
    --prefix)
        PREFIX=$arg
        shift
        ;;
    # TO DO
    # cflags=*)
    #    cflags = "-m 32" + "USER STRING"
    #    ;;
esac
shift
done

build_from_source()
{
  # download files if they aren't in current directory
  if [ ! -f ${STRATEGOXTTAR} ] ; then
      wget ${STRATEGOXTURL}
  fi
  if [ ! -f ${ATERMTAR} ] ; then
      wget ${ATERMURL}
  fi
  if [ ! -f ${SDF2TAR} ] ; then
      wget ${SDF2URL}
  fi
  
  # untar files if they aren't there
  if [ ! -f ${STRATEGOXTDIR} ] ; then
      tar zxf ${STRATEGOXTTAR}
  fi
  if [ ! -f ${ATERMDIR} ] ; then
      tar zxf ${ATERMTAR}
  fi
  if [ ! -f ${SDF2DIR} ] ; then
      tar zxf ${SDF2TAR}
  fi
  
  # install aterm
  cd ${ATERMDIR}
  F77=gfortran CC=gcc CXX=g++ ./configure -prefix=${PREFIX}/aterm CFLAGS=$CFLAGS
  make
  make install
  echo ; echo "ATERM INSTALLED" ; echo
  
  # install sdf2-bundle
  cd ../${SDF2DIR}
  ./configure --prefix=${PREFIX}/sdf2-bundle --with-aterm=${PREFIX}/aterm CFLAGS=$CFLAGS
  F77=gfortran CC=gcc CXX=g++ ./configure --prefix=${PREFIX}/sdf2-bundle --with-aterm=${PREFIX}/aterm CFLAGS=$CFLAGS
  make
  make install
  echo ; echo "SDF2 INSTALLED" ; echo
  
  # install strategoxt
  cd ../${STRATEGOXTDIR}
  #./configure --prefix=${PREFIX}/strategoxt --with-aterm=${PREFIX}/aterm --with-sdf=${PREFIX}/sdf2-bundle CFLAGS=$CFLAGS
  F77=gfortran CC=/usr/bin/gcc CXX=g++ ./configure --prefix=${PREFIX}/strategoxt --with-aterm=${PREFIX}/aterm --with-sdf=${PREFIX}/sdf2-bundle CFLAGS=$CFLAGS
  make
  make install
  #echo ; echo "STRATEGOXT INSTALLED" ; echo
}
  
install_binaries()
{
  wget http://ftp.strategoxt.org/pub/stratego/StrategoXT/strategoxt-0.17/macosx/strategoxt-superbundle-0.17-macosx.tar.gz
  tar xf strategoxt-superbundle-0.17-macosx.tar.gz
  cd strategoxt-superbundle-0.17-macosx/opt
  if [ ! -d "$PREFIX" ]; then
    mkdir $PREFIX 
  fi
  mv aterm sf2-bundle strategoxt ${PREFIX}
}

if [ `uname` == "Darwin" ]; then
  install_binaries
else
  build_from_source
fi
