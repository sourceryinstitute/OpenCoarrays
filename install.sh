#!/bin/bash
#
# install.sh
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

# This file is organized into three sections:
# 1. Command-line argument processor.
# 2. Function definitions.
# 3. Main body. 
# The script depends on several external programs, including a second script that
# builds prerequisite software.  Building prerequisites requires network access
# unless tar balls of the prerequisites are present.  

# ___________________ Process command-line arguments ___________________

this_script=`basename $0` 

# Interpret the first command-line argument, if present, as the OpenCoarrays installation path.
# Otherwise, install in a subdirectory of the present working directory.
if [[ -z $1 ]]; then
  install_path=${PWD}/opencoarrays-installation
else
  install_path=$1
fi


#Interpret the second command-line argument, if present, as the number of threads for 'make'.
#Otherwise, default to single-threaded 'make'.
if [[ -z $2 ]]; then
  num_threads=1
else
  num_threads=$2
fi

# ___________________ Define functions for use in the main body ___________________

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

find_or_install()
{
  package=$1
  # This is a bash 3 hack standing in for a bash 4 hash (bash 3 is the lowest common
  # denominator because, for licensing reasons, OS X only has bash 3 by default.)
  # See http://stackoverflow.com/questions/1494178/how-to-define-hash-tables-in-bash
  package_executable_array=( 
    "gcc:gfortran"
    "cmake:cmake"
    "mpich:mpif90"
    "_unknown:0" 
  )
  for element in "${package_executable_array[@]}" ; do
     KEY="${element%%:*}"
     VALUE="${element##*:}"
     if [[ "$KEY" == "_unknown" ]]; then
       # No recognizeable argument passed so print usage information and exit:
       printf "$this_script: Package name not recognized in function find_or_install.\n"
       exit 1
     elif [[ $package == "$KEY" ]]; then
       executable=$VALUE
       break
     fi  
  done
  printf "Checking whether $executable is in the PATH... "
  if type $executable > /dev/null; then
    printf "yes.\n"

    if [[ $package == "mpich" ]]; then
      echo "Using the pre-installed mpicc and mpif90"
      MPICC=mpicc
      MPIFC=mpif90
    fi

    never=false
    until [ $never == true ]; do
    # To avoid an infinite loop every branch must end with return, exit, or break

      if [[ $package != "gcc" ]]; then

        return # return

      else #package is gcc

        printf "Checking whether gfortran is version 5.1.0 or later..."
        gfortran -o acceptable_compiler ./install_prerequisites/acceptable_compiler.f90
        gfortran -o print_true ./install_prerequisites/print_true.f90
        is_true=`./print_true`
        acceptable=`./acceptable_compiler`
        rm acceptable_compiler print_true
  
        if [[ $acceptable == $is_true ]]; then

          printf "yes\n"
          FC=gfortran
          CC=gcc
          CXX=g++

          return # found an acceptable gfortran in PATH

        else
          printf "no\n"

          break # need to install gfortran below
        fi 
        printf "$this_script: an infinite until loop is missing a break, return, or exit.\n"
        exit 1 # This should never be reached
      fi 
      printf "$this_script: an infinite until loop is missing a break, return, or exit.\n"
      exit 1 # This should never be reached
    done
    # The $executable is not an acceptable version
  fi 

  # Now we know the $executable is not in the PATH or is not an acceptable version
  printf "Ok to downloand, build, and install $package from source? (y/n) "
  read proceed
  if [[ $proceed == "y" ]]; then
    printf "Downloading, building, and installing $package \n"
    cd install_prerequisites &&
    if [[ $package == "gcc" ]]; then
      # Building GCC from source requires flex
      printf "Building requires the 'flex' package.\n"
      printf "Checking flex is in the PATH... "
      if type flex > /dev/null; then
        printf "yes\n"
      else
        printf "no\n"
        printf "Ok to downloand, build, and install flex from source? (y/n) "
        read build_flex
        if [[ $build_flex == "y" ]]; then
          FC=gfortran CC=gcc CXX=g++ ./build flex 
          flex_install_path=`./build flex --default --query` &&
          if [[ -f $flex_install_path/bin/flex ]]; then
            printf "Installation successful."
            printf "$package is in $flex_install_path/bin \n"
          fi
          PATH=$flex_install_path/bin:$PATH
        else
          printf "Aborting. Building GCC requires flex.\n"
          exit 1
        fi
      fi
    fi
    FC=gfortran CC=gcc CXX=g++ ./build $package --default $num_threads &&
    package_install_path=`./build $package --default --query` &&
    if [[ -f $package_install_path/bin/$executable ]]; then
      printf "Installation successful."
      printf "$package is in $package_install_path/bin \n"
      if [[ $package == "gcc" ]]; then
        FC=$package_install_path/bin/gfortran
        CC=$package_install_path/bin/gcc
        CXX=$package_install_path/bin/g++
      elif [[ $package == "mpich" ]]; then
        MPICC=$package_install_path/bin/mpicc
        MPIFC=$package_install_path/bin/mpif90
      fi
      PATH=$package_install_path/bin:$PATH
      return
    else
      printf "Installation unsuccessful."
      printf "$package is not in the expected path: $package_install_path/bin \n"
      exit 1
    fi
  else
    printf "Aborting. OpenCoarrays installation requires $package \n"
    exit 1
  fi
  printf "$this_script: the dependency installation logic is missing a return or exit.\n"
  exit 1
}

# ___________________ Define functions for use in the main body ___________________

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
  build_path=${PWD}/opencoarrays-build
  installation_record=install-opencoarrays.log
  time \
  {
    # Building OpenCoarrays with CMake requires MPICH and GCC; MPICH requires GCC. 
    find_or_install gcc   &&
    find_or_install mpich &&
    find_or_install cmake &&
    mkdir -p $build_path  &&
    cd $build_path        &&
    if [[ -z $MPICC || -z $MPIFC ]]; then
      echo "Empty MPICC=$MPICC or MPIFC=$MPIFC" 
      exit 1
    else
      CC=$MPICC FC=$MPIFC cmake .. -DCMAKE_INSTALL_PREFIX=$install_path &&
      make -j$num_threads &&
      make install        &&
      make clean
    fi
  } >&1 | tee $installation_record

  # Report installation success or failure
  printf "\n"
  if [[ -f $install_path/bin/caf && -f $install_path/bin/cafrun  && -f $install_path/bin/build ]]; then
    printf "$this_script: Done.\n\n"
    printf "The OpenCoarrays compiler wrapper (caf), program launcher (cafrun), and\n"
    printf "prerequisite package installer (build) are in the following directory:\n"
    printf "$install_path/bin.\n\n"
  else
    printf "$this_script: Done. \n\n"
    printf "Something went wrong. The OpenCoarrays compiler wrapper (caf),\n" 
    printf "program launcher (cafrun), and prerequisite package installer (build) \n"
    printf "are not in the expected location:\n"
    printf "$install_path/bin.\n"
    printf "Please review the '$installation_record' file for more information.\n\n"
  fi
fi
