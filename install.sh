#!/bin/bash
#
# build
#
# -- This script installs OpenCoarrays and its prerequisites.
#
# OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
# Copyright (c) 2015, Sourcery, Inc.
# Copyright (c) 2015, Sourcery Institute
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

usage()
{
    echo ""
    echo " $this_script - Bash script for installing OpenCoarrays and its prerequisites"
    echo ""
    echo " Usage (optional arguments in square brackets): "
    echo "      $this_script [<options>] or [<installation-path> <number-of-threads>]"
    echo ""
    echo " Options:"
    echo "   --help, -h           Show this help message"
    echo ""
    echo " Examples:"
    echo ""
    echo "   $this_script"
    echo "   $this_script ${PWD}"
    echo "   $this_script /opt/opencoarrays 4"
    echo "   $this_script --help"
    echo ""
    exit 1
}

# Interpret the first command-line argument, if present, as the installation path.
# Otherwise, install in a subdirectory of the present working directory.
default_install_path=${PWD}/opencoarrays-installation
if [[ -z $1 ]]; then
  install_path=$default_install_path
else
  install_path=$1
fi

build_path=${PWD}/opencorrays-build

# Interpret the second command-line argument, if present, as the number of threads for 'make'.
# Otherwise, default to single-threaded 'make'.
if [ -z $2 ]; then
  num_threads=1
else
  num_threads=$2
fi

CC_DEFAULT=gcc
FC_DEFAULT=gfortran
MPICC_DEFAULT=mpicc
MPIFC_DEFAULT=mpif90

printf "Checking whether CC and FC are both defined..."
if [[ -z $CC || -z $FC ]]; then
  CC=$CC_DEFAULT
  FC=$FC_DEFAULT
  printf "no.\n"
  printf "Using default CC=$CC, FC=$FC\n"
fi

printf "Checking whether MPICC and MPIFC are both defined..."
if [[ -z $MPICC || -z $MPIF90 ]]; then
  MPICC=$MPICC_DEFAULT
  MPIFC=$MPIFC_DEFAULT
  printf "no.\n"
  printf "Using default MPICC=$MPICC, MPIFC=$MPIFC\n"
fi

find_or_install()
{
  package_to_check=$1
  printf "Checking whether $package_to_check is in the PATH... "
  if type $package_to_check > /dev/null; then
    printf "yes.\n"
    return
  fi
  # Set the MPI Fortran compiler for building OpenCoarrays prerequisites.
  if [ ! -z $MPIF90 ]; then

    # MPIF90 environment variable is not empty
    printf "Environment variable MPIF90=$MPIF90.\n"
    if type $MPIF90 > /dev/null; then
      # MPIF90 environment variable specifies an executable program
      printf "Using MPIF90=$MPIF90 as the MPI Fortran compiler.\n"
    else
      # The specified MPIF90 was not found or does not execute. Time to bail.
      printf "MPIF90=$MPIF90 not found.  Please set the MPIF90 environment variable to the \n"
      printf "name of an MPI Fortran compiler and prepend the path if it is not in your PATH.\n"
      exit 1
    fi

  else # MPIF90 environment variable is empty

    printf "Environment variable MPIF90 is empty. Trying mpif90'.\n"

    if type $MPIF90_DEFAULT > /dev/null; then
      # mpif90 is an executable program in the user's PATH
      printf "Found mpif90 in your path. Using mpif90 as the MPI Fortran compiler.\n"
      MPIF90=$MPIF90_DEFAULT

    else  #
      printf "No $MPIF90_DEFAULT in your path. Ok to download and install the default MPI (MPICH)? (y/n).\n"
      read proceed
      if [[ $proceed == "y" ]]; then
        printf "Downloading, building, and installing MPICH.\n"
        cd install_prerequisites &&
        FC=$FC_DEFAULT CC=$CC_DEFAULT ./build mpich &&
        mpich_install_path=`./build mpich --default --query` &&
        if [[ -f "$mpich_install_path/bin/mpif90" ]]; then 
          printf "Installed $MPI_F90 in $mpich_install_path (You might add the 'bin' subdirectory to your PATH).\n"
        else
          printf "Can't find mpif90 in the expected location: $mpich_install_path/bin/mpif90 \n"
        fi
      else 
        printf "Please install an MPI Fortran compiler and restart $this_script.\n"
        exit 1
      fi
    fi
     
    exit 1
  fi  
  exit 1
}

if [[ $1 == '--help' || $1 == '-h' ]]; then
  # Print usage information if script is invoked with --help or -h argument
  usage | less
elif [[ $1 == '-v' || $1 == '-V' || $1 == '--version' ]]; then
  # Print script copyright if invoked with -v, -V, or --version argument
  echo ""
  echo "OpenCoarrays installer"
  echo "Copyright (C) 2015 Sourcery, Inc."
  echo "Copyright (C) 2015 Sourcery Institute"
  echo ""
  echo "$this_script comes with NO WARRANTY, to the extent permitted by law."
  echo "You may redistribute copies of $this_script under the terms of the"
  echo "BSD 3-Clause License.  For more information about these matters, see"
  echo "http://www.sourceryinstitute.org/license.html"
  echo ""
else

  # Find or install prerequisites and install OpenCoarrays
  installation_record=install-opencoarrays.log
  time \
  {
    find_or_install "cmake"     &&
    find_or_install "$MPIFC"    &&
    find_or_install "$MPICC"    &&
    mkdir -p opencoarrays-build &&
    cd opencoarrays-build       &&
    CC=$MPICC FC=$MPIFC cmake .. -DCMAKE_INSTALL_PREFIX=$install_path &&
    make &&
    make install 
  } >&1 | tee $installation_record

  # Report installation success or failure
  printf "\n"
  if [[ -f $install_path/bin/caf && -f $install_path/bin/cafrun  && -f $install_path/bin/build ]]; then
    printf "$this_script: Done.\n"
    printf "The OpenCoarrays compiler wrapper (caf), program launcher (cafrun), and\n"
    printf "prerequisite package installer (build) are in the following directory:\n"
    printf "$install_path/bin.\n"
  else
    printf "Something went wrong. The OpenCoarrays compiler wrapper (caf),\n" 
    printf "program launcher (cafrun), and prerequisite package installer (build) \n"
    printf "are not in the expected location:\n"
    printf "$install_path/bin.\n"
    printf "Please review the '$installation_record' file for more information.\n"
  fi
fi
