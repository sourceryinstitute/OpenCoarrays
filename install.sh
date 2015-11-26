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
build_path=${PWD}/opencoarrays-build

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
    echo "   $this_script /opt/opencoarrays 4"
    echo "   $this_script --help"
    echo ""
    exit 1
}

# Include stack management functions
. ./install_prerequisites/stack.sh
stack_new dependency_pkg
stack_new dependency_exe
stack_new dependency_path
stack_new acceptable_in_path
stack_new script_installed

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
    "flex:flex"
    "bison:yacc"
    "_unknown:0" 
  )
  for element in "${package_executable_array[@]}" ; do
     KEY="${element%%:*}"
     VALUE="${element##*:}"
     if [[ "$KEY" == "_unknown" ]]; then
       # No recognizeable argument passed so print usage information and exit:
       printf "$this_script: Package name ($package) not recognized in find_or_install function [exit 2].\n"
       exit 2
     elif [[ $package == "$KEY" ]]; then
       executable=$VALUE
       break
     fi  
  done

  if [[ "$package" == "$executable" ]]; then
    printf "$this_script: Checking whether $executable is in the PATH..."
  else
    printf "$this_script: Checking whether $package executable $executable is in the PATH..."
  fi
  if type $executable > /dev/null; then 
    printf "yes.\n"
    package_in_path=true
  else 
    printf "no.\n"
    package_in_path=false
  fi
  
  package_install_path=`./build $package --default --query` 

  printf "$this_script: Checking whether $this_script has installed $executable... "
  if type $package_install_path/bin/$executable > /dev/null; then 
    printf "yes.\n"
    script_installed_package=true
    stack_push script_installed $package $executable
  else 
    printf "no.\n"
  fi

  if [[ "$package_in_path" == true ]] || [[ "$script_installed_package" == true ]]; then

    # Start an infinite loop to demarcate a block of code that can be exited via the 'break' 
    # command.  Other exit points/mechanisms include 'return' from this fuction and 'exit'
    # from this script.
    never=false
    until [ "$never" == true ]; do

      # To avoid an infinite loop, every branch must end with return, exit, or break

      if [[ "$package" == "cmake" ]]; then

        # We only arrive here by the explicit, direct call 'find_or_install cmake' inside 
        # the build_opencoarrays function. (There is no possibility of arriving here by
        # by recursion).  From here, we will either return to the body of 
        # build_opencoarrays because this script successfully built cmake on an earlier 
        # pass or break to ./build this script hasn't yet built cmake and an insufficient 
        # cmake version is in the PATH or return to the body of build_opencoarrays because 
        # this script hasn't built cmake but a sufficient version is in the PATH.

        if [[ "$script_installed_package" == true ]]; then
          printf "$this_script: Using the $package installed by $this_script\n"
          export CMAKE=$package_install_path/bin/cmake
          stack_push script_installed $package $executable
          break 

        elif [[ "$package_in_path" == "true" ]]; then
          printf "$this_script: Checking whether $package in PATH is version < 3.2.0... "
          cmake_version=`cmake --version|head -1`
          if [[ "$cmake_version" < "cmake version 3.2.0" ]]; then
            printf "yes.\n"
            # no prerequisites to specify
          else
            printf "no.\n"
            printf "$this_script: Using the $executable found in the PATH.\n"
            export CMAKE=cmake
            stack_push acceptable_in_path $package $executable
          fi
          break

        else
          echo "$this_script: this line should never be reached [exit 3]"
          exit 3
        fi

      elif [[ $package == "flex" ]]; then

        # We only arrive here by recursion because the 'elif [[ $package == "gcc" ]]' 
        # block specified prerequisite="flex". From here, we will either return because 
        # this script installed flex on a previous pass or break to ./build flex after 
        # find_or_install $prerequisite "bison" or return because a sufficient version of 
        # flex is in the PATH.

        if [[ "$script_installed_package" == true ]]; then
          printf "$this_script: Using the $executable installed by $this_script\n"
          export FLEX=$package_install_path/bin/$executable
          stack_push script_installed $package $executable
          break 

        elif [[ "$package_in_path" == "true" ]]; then
          printf "Checking whether $package in PATH is version < 2.6.0... "
          flex_version=`flex --version`
          if [[ "$flex_version" < "flex 2.6.0" ]]; then
            printf "yes\n"
            acceptable_bison_in_path=`stack_print acceptable_in_path | grep bison`
            script_installed_bison=`stack_print script_installed | grep bison`
            if [[ -z $acceptable_bison_in_path && -z $script_installed_bison ]]; then
              stack_push dependency_pkg "bison"
              stack_push dependency_exe "yacc"
              stack_push dependency_path `./build bison --default --query` 
            fi

          else
            printf "no.\n"
            printf "$this_script: Using the $executable found in the PATH.\n"
            export FLEX=$executable
            stack_push acceptable_in_path $package $executable

          fi
          break 

        else
          echo "$this_script: this line should never be reached [exit 3]"
          exit 3
        fi

      elif [[ $package == "mpich" ]]; then

        # We only arrive here by the explicit, direct call 'find_or_install mpich' inside 
        # the build_opencoarrays function. (There is no possibility of arriving here by
        # by recursion).  From here, we will either return to the body of build_opencoarrays
        # because this script built mpich on a previous pass or return to the body of 
        # build_opencoarrays because a sufficient mpich version is in the PATH or break
        # to ./build mpich after find_or_install $prerequisite "gcc".

        if [[ "$script_installed_package" == true ]]; then
          printf "$this_script: Using the $package installed by $this_script\n"
          export MPIFC=$package_install_path/bin/mpif90
          export MPICC=$package_install_path/bin/mpicc
          export MPICXX=$package_install_path/bin/mpicxx
          stack_push script_installed $package $executable
          break 

        elif [[ "$package_in_path" == "true" ]]; then
          printf "Checking whether $executable in PATH wraps gfortran version 5.1.0 or later... "
          $executable -o acceptable_compiler acceptable_compiler.f90
          $executable -o print_true print_true.f90
          is_true=`./print_true`
          acceptable=`./acceptable_compiler`
          rm acceptable_compiler print_true
          if [[ "$acceptable" == "$is_true" ]]; then
            printf "yes.\n"
            printf "Using the $executable found in the PATH.\n"
            export MPIFC=mpif90
            export MPICC=mpicc
            export MPICXX=mpicxx
            stack_push acceptable_in_path $package $executable
          else
            printf "no.\n"
            acceptable_gcc_in_path=`stack_print acceptable_in_path | grep gcc`
            script_installed_gcc=`stack_print script_installed | grep gcc`
            if [[ -z $acceptable_gcc_in_path && -z $script_installed_gcc ]]; then
              stack_push dependency_pkg "gcc"
              stack_push dependency_exe "gfortran"
              stack_push dependency_path `./build gcc --default --query` 
            fi
          fi
          break

        else
          echo "$this_script: this line should never be reached [exit 4]"
          exit 4
        fi

      elif [[ $package == "gcc" ]]; then

        # We only arrive here by recursion because the 'elif [[ $package == "mpich" ]]' 
        # block specified prerequisite="gcc". From here, we will either return because 
        # this script installed gcc on a previous pass or break to ./build gcc after 
        # find_or_install $prerequisite "flex" or return because a sufficient version of 
        # gcc is in the PATH.

        if [[ "$script_installed_package" == true ]]; then
          printf "$this_script: Using the $package executable $executable installed by $this_script\n"
          export FC=$package_install_path/bin/gfortran
          export CC=$package_install_path/bin/gcc
          export CXX=$package_install_path/bin/g++
          stack_push script_installed $package $executable
          break 

        elif [[ "$package_in_path" == "true" ]]; then
          printf "$this_script: Checking whether $executable in PATH is version 5.1.0 or later..."
          $executable -o acceptable_compiler acceptable_compiler.f90
          $executable -o print_true print_true.f90
          is_true=`./print_true`
          acceptable=`./acceptable_compiler`
          rm acceptable_compiler print_true
          if [[ "$acceptable" == "$is_true" ]]; then
            printf "yes.\n"
            printf "$this_script: Using the $executable found in the PATH.\n"
            export FC=gfortran
            export CC=gcc
            export CXX=g++
            stack_push acceptable_in_path $package $exectuable
          else
            printf "no.\n"
            acceptable_flex_in_path=`stack_print acceptable_in_path | grep flex`
            script_installed_flex=`stack_print script_installed | grep flex`
            if [[ -z $acceptable_flex_in_path && -z $script_installed_flex ]]; then
              stack_push dependency_pkg "flex" 
              stack_push dependency_exe "flex" 
              stack_push dependency_path `./build flex --default --query` 
            fi
          fi
          break 

        else
          echo "$this_script: this line should never be reached [exit 5]"
          exit 5
        fi

      elif [[ $package == "bison" ]]; then

        # We only arrive here by recursion because the 'elif [[ $package == "flex" ]];' 
        # block specified prerequisite="bison". From here, we will either return because 
        # this script installed bison on a previous pass or break to ./build bison after
        # setting prerequisite="none" to halt the recursion or return because a sufficient 
        # version of the bison program 'yacc' is in the PATH.

        if [[ "$script_installed_package" == true ]]; then
          printf "$this_script: Using the $package executable $executable installed by $this_script\n"
          export YACC=$package_install_path/bin/yacc
          printf "$this_script: Using the $package executable $executable installed by $this_script\n"
          stack_push script_installed $package $executable
          break 

        elif [[ "$package_in_path" == "true" ]]; then
          printf "$this_script: Checking whether $package executable $executable in PATH is version < 3.0.4... "
          yacc_version=`yacc --version|head -1`
          if [[ "$yacc_version" < "bison (GNU Bison) 3.0.4" ]]; then
            printf "yes.\n"
          else
            printf "no.\n"
            printf "$this_script: Using the $package executable $executable found in the PATH.\n"
            YACC=yacc
            stack_push acceptable_in_path $package $executable
          fi
          break

        else
          echo "$this_script: this line should never be reached [exit 6]"
          exit 6
        fi

      else
        printf "$this_script: unknown package name ($package) passed to find_or_install function. [exit 7]\n" &&
        exit 7 
      fi && 
      printf "$this_script: an infinite until loop is missing a break, return, or exit (see outer conditional). [exit 8]\n" &&
      exit 8 # This should never be reached
    done

  else # $package not in PATH and not yet installed by this script

    if [[ $package == "mpich" ]]; then
      stack_push dependency_pkg "gcc"
      stack_push dependency_exe "gfortran"
      stack_push dependency_path `./build gcc --default --query` 
    elif [[ $package == "gcc" ]]; then
      stack_push dependency_pkg "flex"
      stack_push dependency_exe "flex"
      stack_push dependency_path `./build flex --default --query`
    elif [[ $package == "flex" ]]; then
      stack_push dependency_pkg "bison"
      stack_push dependency_exe "yacc"
      stack_push dependency_path `./build bison --default --query`
    elif [[ $package == "cmake" ||  $package == "bison" ]]; then
      # Halt the recursion by signaling that we're ready to start building software.
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"
    else
      echo "$this_script: unknown package ($package) passed to find_or_install function [exit 9]"
      exit 9
    fi

    CC=gcc
    CXX=g++
    FC=gfortran

  fi # End of else package not in path and not yet installed by this script
  
  stack_size dependency_pkg num_dependencies
  if [[ $num_dependencies != 0 ]]; then
    stack_pop  dependency_pkg  prerequisite_pkg
    stack_pop  dependency_exe  prerequisite_exe
    stack_pop  dependency_path prerequisite_path

    if [[ $prerequisite_pkg != "none" ]]; then
      stack_push dependency_pkg  $prerequisite_pkg
      stack_push dependency_exe  $prerequisite_exe
      stack_push dependency_path $prerequisite_path
      printf "$this_script: Building $package from source requires $prerequisite_pkg.\n"
      find_or_install $prerequisite_pkg
    fi
  fi

  echo "$this_script: ready to build $package"
  stack_print dependency_pkg  
  stack_print dependency_exe  
  stack_print dependency_path 

exit 99

 #stack_pop dependency_exe exe_prerequisite
 #stack_pop dependency_pkg exe_prerequisite
 #stack_pop dependency_path prerequisite_path
 #echo "popped $prerequisite_exe off the stack"
 #  printf "no.\n"
 #  script_installed_package=false
 #fi

  acceptable_version_in_path=`stack_print acceptable_in_path | grep $package`
  script_installed_version=`stack_print script_installed | grep $package`

  if [[ -z $acceptable_version_in_path ]] && [[ -z $script_installed_version ]]; then

    printf "$this_script: Ok to downloand, build, and install $package from source? (y/n) "
    read proceed_with_build
    if [[ $proceed_with_build != "y" ]]; then
      printf "\n$this_script: OpenCoarrays installation requires $package. Aborting. [exit 10]\n"
      exit 10
    else # permission granted to build
  
      printf "Downloading, building, and installing $package \n"
      echo "Build command: FC=$FC CC=$CC CXX=$CXX ./build $package --default $package_install_path $num_threads"
      FC=$FC CC=$CC CXX=$CXX ./build $package --default $package_install_path $num_threads
  
      printf "Checking whether $package executable $executable is in the PATH..."
      if [[ -f $package_install_path/bin/$executable ]]; then
        printf "yes.\n"
        printf "$this_script: Installation successful.\n"
        printf "$package is in $package_install_path/bin \n"
        export PATH=$package_install_path/bin:$PATH
      else
        printf "$this_script: Installation unsuccessful. "
        printf "$package is not in the following expected path:\n"
        printf "$package_install_path/bin \n"
        printf "Aborting. [exit 11]"
        exit 11
      fi # End 'if [[ -f $package_install_path/bin/$executable ]]' 

    fi # End 'if [[ $proceed_with_build == "y" ]]; then'
     
    # Each branch above should end with a 'return' or 'exit' so we should never reach here.
    # printf "$this_script: the dependency installation logic is missing a return or exit. [exit 12]\n"
    # exit 12

   fi # End 'if [[ "$acceptable_package_in_path" != true || "$script_installed_package" != true ]]; then'
}

build_opencoarrays()
{
  # A default OpenCoarrays build requires CMake and MPICH. MPICH requires GCC.
  find_or_install mpich  &&
  find_or_install cmake &&
  mkdir -p $build_path  &&
  cd $build_path        &&
  if [[ -z $MPICC || -z $MPIFC ]]; then
    echo "Empty MPICC=$MPICC or MPIFC=$MPIFC" 
    exit 1
  else
    if [[ -z "$CMAKE" ]]; then
      echo "$this_script: empty CMAKE in build_opencoarrays function  [exit 21]"
      exit 21
    fi
    CC=$MPICC FC=$MPIFC $CMAKE .. -DCMAKE_INSTALL_PREFIX=$install_path &&
    make -j$num_threads &&
    make install        &&
    make clean
  fi
}
# ___________________ End of function definitions for use in the main body __________________

# __________________________________ Start of Main Body _____________________________________

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

else # Find or install prerequisites and install OpenCoarrays
  
  cd install_prerequisites &&
  installation_record=install-opencoarrays.log &&
  build_opencoarrays >&1 | tee ../$installation_record
  printf "\n"

  # Report installation success or failure:
  if [[ -f $install_path/bin/caf && -f $install_path/bin/cafrun  && -f $install_path/bin/build ]]; then

    # Installation succeeded
    printf "$this_script: Done.\n\n"
    printf "The OpenCoarrays compiler wrapper (caf), program launcher (cafrun), and\n"
    printf "prerequisite package installer (build) are in the following directory:\n"
    printf "$install_path/bin.\n\n"

  else # Installation failed
    
    printf "$this_script: Done. \n\n"
    printf "Something went wrong. The OpenCoarrays compiler wrapper (caf),\n" 
    printf "program launcher (cafrun), and prerequisite package installer (build) \n"
    printf "are not in the following expected location:\n"
    printf "$install_path/bin.\n"
    printf "Please review the following file for more information:\n"
    printf "$install_path/$installation_record \n\n\n"

  fi # Ending check for caf, cafrun, build not in expected path
fi # Ending argument checks
# ____________________________________ End of Main Body ____________________________________________
