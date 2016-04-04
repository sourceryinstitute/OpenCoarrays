#!/bin/bash
#
# install.sh
#
# -- This script installs OpenCoarrays and its prerequisites.
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

# This file is organized into three sections:
# 1. Command-line argument and environment variable processing.
# 2. Function definitions.
# 3. Main body.
# The script depends on several external programs, including a second script that
# builds prerequisite software.  Building prerequisites requires network access
# unless tar balls of the prerequisites are present.

# TODO:
# 1. Use getopts for more traditional Unix-style command-line argument processing.
# 2. Collapse the body of the main conditional branches in the find_or_install function
#    into one new function.
# 3. Verify that script-installed packages meet the minimum version number.
# 4. Add a script_transfer function to collapse the stack_pop x; stack_push z y
#    pattern into one statement
# 5. Consider adding mpich and cmake to the dependency stack before passing them to
#    find_or_install to make the blocks inside find_or_install more uniform.
#    Alternatively, check the dependency stacks for the package before entering the
#    main conditional blocks in find_or_install.
#
 
# Make sure we really exit with correct status when piping function output
set -o pipefail

# __________ Process command-line arguments and environment variables _____________

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

# Verify that the top-level OpenCoarrays source directory is either present working directory
# or is specified by a valid in the OPENCOARRAYS_SRC_DIR environment variable
if [[ -d "$OPENCOARRAYS_SRC_DIR" ]]; then
  opencoarrays_src_dir=$OPENCOARRAYS_SRC_DIR
else
  opencoarrays_src_dir=${PWD}
fi
cd "$opencoarrays_src_dir" || $directory_exists==false
if [[ "$directory_exists" == "false" ]]; then
  echo "The OPENCOARRAYS_SRC_DIR environment variable specifies a non-existent directory [exit 10]:"
  echo "$OPENCOARRAYS_SRC_DIR"
  exit 10
fi

build_path=$opencoarrays_src_dir/opencoarrays-build
build_script=$opencoarrays_src_dir/install_prerequisites/build
if [[ ! -x "$build_script" ]]; then
  echo "$this_script: $build_script script does not exist or the user lacks executable permission for it."
  echo "$this_script: Please run this_script in the top-level OpenCoarrays source directory or set the"
  echo "$this_script: OPENCOARRAYS_SRC_DIR environment variable to the top-level OpenCoarrays source path."
  exit 20
fi

# ___________________ Define functions for use in the Main Body ___________________

usage()
{
    echo ""
    echo " $this_script - Bash script for installing OpenCoarrays and its prerequisites"
    echo ""
    echo " Usage (optional arguments in square brackets): "
    echo "      $this_script [<options>] or [<installation-path> <number-of-threads>]"
    echo ""
    echo " Options:"
    echo "   -h, --help         Show this help message"
    echo "   -v, -V, --version  Show version information"
    echo ""
    echo " Examples:"
    echo ""
    echo "   $this_script"
    echo "   $this_script /opt/opencoarrays 4"
    echo "   $this_script --help"
    echo ""
    echo "[exit 30]"
    exit 30
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
    "m4:m4"
    "_unknown:0"
  )
  for element in "${package_executable_array[@]}" ; do
     KEY="${element%%:*}"
     VALUE="${element##*:}"
     if [[ "$KEY" == "_unknown" ]]; then
       # No recognizeable argument passed so print usage information and exit:
       printf "$this_script: Package name ($package) not recognized in find_or_install function [exit 40].\n"
       exit 40
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
    package_version_in_path=`$executable --version|head -1`
  else
    printf "no.\n"
    package_in_path=false
  fi

  package_install_path=`./build $package --default --query-path`

  printf "$this_script: Checking whether $executable is in the directory in which $this_script\n"
  printf "            installs it by default and whether the user has executable permission for it..."
  if [[ -x "$package_install_path/bin/$executable" ]]; then
    printf "yes.\n"
    script_installed_package=true
    stack_push script_installed $package $executable
  else
    script_installed_package=false
    printf "no.\n"
  fi

  minimum_version=`./build $package --default --query-version`

  if [[ "$package" == "cmake" ]]; then

    # We arrive here only by the explicit, direct call 'find_or_install cmake' inside
    # the build_opencoarrays function. Because there is no possibility of arriving here
    # by recursion (no packages depend on cmake except OpenCoarrays, which gets built
    # after all dependencies have been found or installed), cmake must add itself to
    # the dependency stack if no acceptable cmake is found.

    # Every branch that discovers an acceptable pre-existing installation must set the
    # CMAKE environment variable. Every branch must also manage the dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package installed by $this_script\n"
      export CMAKE=$package_install_path/bin/$executable
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $package in PATH is version < $minimum_version... "

      if ! ./check_version.sh $package `./build $package --default --query-version`; then
        printf "yes.\n"
        # Here we place $package on the dependency stack to trigger the build of the above file:
        stack_push dependency_pkg $package "none"
        stack_push dependency_exe $package "none"
        stack_push dependency_path `./build cmake --default --query-path` "none"

      else
        printf "no.\n"
        printf "$this_script: Using the $executable found in the PATH.\n"
        export CMAKE=$executable
        stack_push acceptable_in_path $package $executable
        # Prevent recursion
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else # Build package ($package has no prerequisites)
      stack_push dependency_pkg $package "none"
      stack_push dependency_exe $package "none"
      stack_push dependency_path `./build $package --default --query-path` "none"
    fi

  elif [[ $package == "mpich" ]]; then

    # We arrive here only by the explicit, direct call 'find_or_install mpich' inside
    # the build_opencoarrays function. Because there is no possibility of arriving here
    # by recursion (no packages depend on mpich except OpenCoarrays, which gets built
    # after all dependencies have been found or installed), mpich must add itself to
    # the dependency stack if no acceptable mpich is found.

    # Every branch that discovers an acceptable pre-existing installation must set the
    # MPIFC, MPICC, and MPICXX environment variables. Every branch must also manage the
    # dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package installed by $this_script\n"
      export MPIFC=$package_install_path/bin/mpif90
      export MPICC=$package_install_path/bin/mpicc
      export MPICXX=$package_install_path/bin/mpicxx
      # Halt the recursion
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $executable in PATH wraps gfortran... "
      mpif90__version_header=`mpif90 --version | head -1`
      first_three_characters=`echo $mpif90__version_header | cut -c1-3`
      if [[ "$first_three_characters" != "GNU" ]]; then
        printf "no.\n"
        # Trigger 'find_or_install gcc' and subsequent build of $package
        stack_push dependency_pkg "none" $package "gcc"
        stack_push dependency_exe "none" $executable "gfortran"
        stack_push dependency_path "none" `./build $package --default --query-path` `./build gcc --default --query-path`
      else
        printf "yes.\n"
        printf "$this_script: Checking whether $executable in PATH wraps gfortran version 5.3.0 or later... "
        $executable acceptable_compiler.f90 -o acceptable_compiler
        $executable print_true.f90 -o print_true
        acceptable=`./acceptable_compiler`
        is_true=`./print_true`
        rm acceptable_compiler print_true
        if [[ "$acceptable" == "$is_true"  ]]; then
          printf "yes.\n"
          printf "$this_script: Using the $executable found in the PATH.\n"
          export MPIFC=mpif90
          export MPICC=mpicc
          export MPICXX=mpicxx
          stack_push acceptable_in_path $package $executable
          # Halt the recursion
          stack_push dependency_pkg "none"
          stack_push dependency_exe "none"
          stack_push dependency_path "none"
        else
          printf "no\n"
          # Trigger 'find_or_install gcc' and subsequent build of $package
          stack_push dependency_pkg "none" $package "gcc"
          stack_push dependency_exe "none" $executable "gfortran"
          stack_push dependency_path "none" `./build $package --default --query-path` `./build gcc --default --query-path`
        fi
      fi

    else # $package not in PATH and not yet installed by this script
      # Trigger 'find_or_install gcc' and subsequent build of $package
      stack_push dependency_pkg  "none" $package "gcc"
      stack_push dependency_exe  "none" $executable "gfortran"
      stack_push dependency_path "none" `./build $package --default --query-path` `./build gcc --default --query-path`
    fi

  elif [[ $package == "gcc" ]]; then

    # We arrive when the 'elif [[ $package == "mpich" ]]' block pushes "gcc" onto the
    # the dependency_pkg stack, resulting in the recursive call 'find_or_install gcc'

    # Every branch that discovers an acceptable pre-existing installation must set the
    # FC, CC, and CXX environment variables. Every branch must also manage the dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package executable $executable installed by $this_script\n"
      export FC=$package_install_path/bin/gfortran
      export CC=$package_install_path/bin/gcc
      export CXX=$package_install_path/bin/g++
      gfortran_lib_paths="$package_install_path/lib64/:$package_install_path/lib"
      if [[ -z "$LD_LIBRARY_PATH" ]]; then
        echo "$this_script: export LD_LIBRARY_PATH=\"$gfortran_lib_paths\""
                            export LD_LIBRARY_PATH="$gfortran_lib_paths"
      else
        echo "$this_script: export LD_LIBRARY_PATH=\"$gfortran_lib_paths:$LD_LIBRARY_PATH\""
                            export LD_LIBRARY_PATH="$gfortran_lib_paths:$LD_LIBRARY_PATH"
      fi
      # Remove $package from the dependency stack
      stack_pop dependency_pkg package_done
      stack_pop dependency_exe executable_done
      stack_pop dependency_path package_done_path
      # Put $package onto the script_installed log
      stack_push script_installed package_done
      stack_push script_installed executable_done
      stack_push script_installed package_done_path
      # Halt the recursion and signal that none of $package's prerequisites need to be built
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $executable in PATH is version 5.3.0 or later..."
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
        stack_push acceptable_in_path $package $executable
        # Remove $package from the dependency stack
        stack_pop dependency_pkg package_done
        stack_pop dependency_exe executable_done
        stack_pop dependency_path package_done_path
        # Put $package onto the script_installed log
        stack_push script_installed package_done
        stack_push script_installed executable_done
        stack_push script_installed package_done_path
        # Halt the recursion and signal that none of $package's prerequisites need to be built
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      else
        printf "no.\n"
        # Trigger 'find_or_install flex' and subsequent build of $package
        stack_push dependency_pkg "flex"
        stack_push dependency_exe "flex"
        stack_push dependency_path `./build flex --default --query-path`
      fi

    else # $package is not in PATH and not yet installed by this script
      # Trigger 'find_or_install flex' and subsequent build of $package
      stack_push dependency_pkg "flex"
      stack_push dependency_exe "flex"
      stack_push dependency_path `./build flex --default --query-path`
    fi

  elif [[ $package == "flex" ]]; then

    # We arrive here only if the 'elif [[ $package == "gcc" ]]' block has pushed "flex"
    # onto the dependency_pkg stack, resulting in the recursive call 'find_or_install flex'.
    # flex therefore does not need to add itself to the stack.

    # Every branch that discovers an acceptable pre-existing installation must set the
    # FLEX environment variable. Every branch must also manage the dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $executable installed by $this_script\n"
      export FLEX=$package_install_path/bin/$executable
      # Remove flex from the dependency stack
      stack_pop dependency_pkg package_done
      stack_pop dependency_exe executable_done
      stack_pop dependency_path package_done_path
      # Put $package onto the script_installed log
      stack_push script_installed package_done
      stack_push script_installed executable_done
      stack_push script_installed package_done_path
      # Halt the recursion and signal that no prerequisites need to be built
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then

      printf "$this_script: Checking whether $package in PATH is version < $minimum_version... "
      if ! ./check_version.sh $package `./build $package --default --query-version`; then
        printf "yes\n"

        export FLEX="$package_install_path/bin/$executable"
        # Trigger 'find_or_install bison' and subsequent build of $package
        stack_push dependency_pkg "bison"
        stack_push dependency_exe "yacc"
        stack_push dependency_path `./build bison --default --query-path`
      else
        printf "no.\n"
        printf "$this_script: Using the $executable found in the PATH.\n"
        export FLEX=$executable
        stack_push acceptable_in_path $package $executable
        # Remove $package from the dependency stack
        stack_pop dependency_pkg package_done
        stack_pop dependency_exe executable_done
        stack_pop dependency_path package_done_path
        # Put $package onto the script_installed log
        stack_push script_installed package_done
        stack_push script_installed executable_done
        stack_push script_installed package_done_path
        # Halt the recursion and signal that none of $package's prerequisites need to be built
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else  # $package is not in the PATH and not yet installed by $this_script
      # Trigger 'find_or_install bison' and subsequent build of $package
      stack_push dependency_pkg "bison"
      stack_push dependency_exe "yacc"
      stack_push dependency_path `./build bison --default --query-path`
    fi

  elif [[ $package == "bison" ]]; then

    # We arrive when the 'elif [[ $package == "flex" ]]' block pushes "bison" onto the
    # the dependency_pkg stack, resulting in the recursive call 'find_or_install bison'

    # Every branch that discovers an acceptable pre-existing installation must set the
    # YACC environment variable. Every branch must also manage the dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package executable $executable installed by $this_script\n"
      export YACC=$package_install_path/bin/yacc
      # Remove bison from the dependency stack
      stack_pop dependency_pkg package_done
      stack_pop dependency_exe executable_done
      stack_pop dependency_path package_done_path
      # Put $package onto the script_installed log
      stack_push script_installed package_done
      stack_push script_installed executable_done
      stack_push script_installed package_done_path
      # Halt the recursion and signal that there are no prerequisites to build
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $package executable $executable in PATH is version < $minimum_version... "
      if ! ./check_version.sh $package `./build $package --default --query-version`; then
        printf "yes.\n"
        export YACC="$package_install_path/bin/$executable"
        # Trigger 'find_or_install m4' and subsequent build of $package
        stack_push dependency_pkg "m4"
        stack_push dependency_exe "m4"
        stack_push dependency_path `./build m4 --default --query-path`
      else
        printf "no.\n"
        printf "$this_script: Using the $package executable $executable found in the PATH.\n"
        YACC=yacc
        stack_push acceptable_in_path $package $executable
        # Remove bison from the dependency stack
        stack_pop dependency_pkg package_done
        stack_pop dependency_exe executable_done
        stack_pop dependency_path package_done_path
        # Put $package onto the script_installed log
        stack_push script_installed package_done
        stack_push script_installed executable_done
        stack_push script_installed package_done_path
        # Halt the recursion and signal that there are no prerequisites to build
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else # $package not in PATH and not yet installed by this script
      # Trigger 'find_or_install m4' and subsequent build of $package
      stack_push dependency_pkg "m4"
      stack_push dependency_exe "m4"
      stack_push dependency_path `./build m4 --default --query-path`
    fi

  elif [[ $package == "m4" ]]; then

    # We arrive when the 'elif [[ $package == "bison" ]]' block pushes "m4" onto the
    # the dependency_pkg stack, resulting in the recursive call 'find_or_install m4'

    # Every branch that discovers an acceptable pre-existing installation must set the
    # M4 environment variable. Every branch must also manage the dependency stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package executable $executable installed by $this_script\n"
      export M4=$package_install_path/bin/m4
      # Remove m4 from the dependency stack
      stack_pop dependency_pkg package_done
      stack_pop dependency_exe executable_done
      stack_pop dependency_path package_done_path
      # Put $package onto the script_installed log
      stack_push script_installed package_done
      stack_push script_installed executable_done
      stack_push script_installed package_done_path
      # Halt the recursion and signal that there are no prerequisites to build
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $package executable $executable in PATH is version < $minimum_version... "
      if ! ./check_version.sh $package `./build $package --default --query-version`; then
        printf "yes.\n"
        export M4="$package_install_path/bin/m4"
        # Halt the recursion and signal that there are no prerequisites to build
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      else
        printf "no.\n"
        printf "$this_script: Using the $package executable $executable found in the PATH.\n"
        M4=m4
        stack_push acceptable_in_path $package $executable
        # Remove m4 from the dependency stack
        stack_pop dependency_pkg package_done
        stack_pop dependency_exe executable_done
        stack_pop dependency_path package_done_path
        # Put $package onto the script_installed log
        stack_push script_installed package_done
        stack_push script_installed executable_done
        stack_push script_installed package_done_path
        # Halt the recursion and signal that there are no prerequisites to build
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else # $package not in PATH and not yet installed by this script
      # Halt the recursion and signal that there are no prerequisites to build
      export M4="$package_install_path/bin/m4"
      stack_push dependency_pkg  "none"
      stack_push dependency_exe  "none"
      stack_push dependency_path "none"
    fi

  else
    if [[ -z "$package" ]]; then
      printf "$this_script: empty package name passed to find_or_install function. [exit 50]\n"
      exit 50
    else
      printf "$this_script: unknown package name ($package) passed to find_or_install function. [exit 55]\n"
      exit 55
    fi
  fi

  echo "$this_script: Updated dependency stack (top to bottom = left to right):"
  stack_print dependency_pkg

  stack_size dependency_pkg num_stacked
  let num_dependencies=num_stacked-1
  if [[ $num_dependencies < 0 ]]; then
    printf "The procedure named in the external call to find_or_install is not on the dependency stack. [exit 60]\n"
    exit 60
  elif [[ $num_dependencies > 0 ]]; then
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

  echo "$this_script: Remaining $package dependency stack (top to bottom = left to right):"
  stack_print dependency_pkg

  stack_pop dependency_pkg package
  stack_pop dependency_exe executable
  stack_pop dependency_path package_install_path

  if [[ $package != "none" ]]; then

    if [[ "$package" == "$executable" ]]; then
      echo "$this_script: Ready to build $executable in $package_install_path"
    else
      echo "$this_script: Ready to build $package executable $executable in $package_install_path"
    fi

    printf "$this_script: Ok to download (if necessary), build, and install $package from source? (Y/n) "
    read proceed_with_build

    if [[ "$proceed_with_build" == "n" || "$proceed_with_build" == "no" ]]; then
      printf "n\n"
      printf "$this_script: OpenCoarrays installation requires $package. Aborting. [exit 70]\n"
      exit 70

    else # permission granted to build
      printf "Y\n"

      # On OS X, CMake must be built with Apple LLVM gcc, which XCode command-line tools puts in /usr/bin
      if [[ `uname` == "Darwin" && $package == "cmake"  ]]; then
        if [[ -x "/usr/bin/gcc" ]]; then
          CC=/usr/bin/gcc
        else
          printf "$this_script: OS X detected.  Please install XCode command-line tools and \n"
          printf "$this_script: ensure that /usr/bin/gcc exists and is executable. Aborting. [exit 75]\n"
          exit 75
        fi
      # Otherwise, if no CC has been defined yet, use the gcc in the user's PATH
      elif [[ -z "$CC" ]]; then
        CC=gcc
      fi

      # On OS X, CMake must be built with Apple LLVM g++, which XCode command-line tools puts in /usr/bin
      if [[ `uname` == "Darwin" && $package == "cmake"  ]]; then
        if [[ -x "/usr/bin/g++" ]]; then
          CXX=/usr/bin/g++
        else
          printf "$this_script: OS X detected.  Please install XCode command-line tools \n"
          printf "$this_script: and ensure that /usr/bin/g++ exists and is executable. Aborting. [exit 76]\n"
          exit 76
        fi
      # Otherwise, if no CXX has been defined yet, use the g++ in the user's PATH
      elif [[ -z "$CC" ]]; then
        CXX=g++
      fi

      # If no FC has been defined yet, use the gfortran in the user's PATH
      if [[ -z "$FC" ]]; then
        FC=gfortran
      fi

      printf "$this_script: Downloading, building, and installing $package \n"
      echo "$this_script: Build command: FC=$FC CC=$CC CXX=$CXX ./build $package --default $package_install_path $num_threads"
      FC="$FC" CC="$CC" CXX="$CXX" ./build "$package" --default "$package_install_path" "$num_threads"

      if [[ -x "$package_install_path/bin/$executable" ]]; then
        printf "$this_script: Installation successful.\n"
        if [[ "$package" == "$executable" ]]; then
          printf "$this_script: $executable is in $package_install_path/bin \n"
        else
          printf "$this_script: $package executable $executable is in $package_install_path/bin \n"
        fi
       # TODO Merge all applicable branches under one 'if [[ $package == $executable ]]; then'
        if [[ $package == "cmake" ]]; then
          echo "$this_script: export CMAKE=$package_install_path/bin/$executable"
                              export CMAKE="$package_install_path/bin/$executable"
        elif [[ $package == "bison" ]]; then
          echo "$this_script: export YACC=$package_install_path/bin/$executable"
                              export YACC="$package_install_path/bin/$executable"
        elif [[ $package == "flex" ]]; then
          echo "$this_script: export FLEX=$package_install_path/bin/$executable"
                              export FLEX="$package_install_path/bin/$executable"
        elif [[ $package == "m4" ]]; then
          echo "$this_script: export M4=$package_install_path/bin/$executable"
                              export M4="$package_install_path/bin/$executable"
        elif [[ $package == "gcc" ]]; then
          echo "$this_script: export FC=$package_install_path/bin/gfortran"
                              export FC="$package_install_path/bin/gfortran"
          echo "$this_script: export CC=$package_install_path/bin/gcc"
                              export CC="$package_install_path/bin/gcc"
          echo "$this_script: export CXX=$package_install_path/bin/g++"
                              export CXX="$package_install_path/bin/g++"
          gfortran_lib_paths="$package_install_path/lib64/:$package_install_path/lib"
          if [[ -z "$LD_LIBRARY_PATH" ]]; then
            export LD_LIBRARY_PATH="$gfortran_lib_paths"
          else
            export LD_LIBRARY_PATH="$gfortran_lib_paths:$LD_LIBRARY_PATH"
          fi
        elif [[ $package == "mpich" ]]; then
          echo "$this_script: export MPIFC=$package_install_path/bin/mpif90"
                              export MPIFC="$package_install_path/bin/mpif90"
          echo "$this_script: export MPICC= $package_install_path/bin/mpicc"
                              export MPICC="$package_install_path/bin/mpicc"
          echo "$this_script: export MPICXX=$package_install_path/bin/mpicxx"
                              export MPICXX="$package_install_path/bin/mpicxx"
        else
          printf "$this_script: WARNING: $package executable $executable installed correctly but the \n"
          printf "$this_script:          corresponding environment variable(s) have not been set. This \n"
          printf "$this_script:          could prevent a complete build of OpenCoarrays. Please report this\n"
          printf "$this_script:          issue at https://github.com/sourceryinstitute/opencoarrays/issues\n"
        fi
        if [[ -z "$PATH" ]]; then
          export PATH="$package_install_path/bin"
        else
          export PATH="$package_install_path/bin:$PATH"
        fi
      else
        printf "$this_script: Installation unsuccessful. "
        printf "$executable is not in the following expected path or the user lacks executable permission for it:\n"
        printf "$package_install_path/bin \n"
        printf "Aborting. [exit 80]"
        exit 80
      fi # End 'if [[ -x "$package_install_path/bin/$executable" ]]'

    fi # End 'if [[ "$proceed_with_build" == "y" ]]; then'

  fi # End 'if [[ "$package" != "none" ]]; then'
}

print_header()
{
  clear
  echo ""
  echo "*** A default build of OpenCoarrays requires CMake 3.4.0 or later     ***"
  echo "*** and MPICH 3.1.4 wrapping GCC Fortran (gfortran) 5.3.0 or later.   ***"
  echo "*** Additionally, CMake, MPICH, and GCC have their own prerequisites. ***"
  echo "*** This script will check for most known requirements in your PATH   ***"
  echo "*** environment variable and in the default installation directory    ***"
  echo "*** to which this script installs each missing prerequisite.  The     ***"
  echo "*** script will recursively traverse the following dependency tree    ***"
  echo "*** and ask permission to download, build, and install any missing    ***"
  echo "*** prerequisites:                                                    ***"
  echo ""
  # Move to a directory tree whose structure mirrors the dependency tree
  pushd $opencoarrays_src_dir/doc/dependency_tree/ > /dev/null
  if type tree &> /dev/null; then
    # dynamically compute and print the tree, suppressing the final line
    tree opencoarrays  | sed '$d'
  else
    # print the most recently saved output of the above 'tree' command
    cat opencoarrays-tree.txt | sed '$d'
  fi
  popd > /dev/null
  echo ""
  echo "*** All prerequisites will be downloaded to, built in, and installed in ***"
  echo "$opencoarrays_src_dir/install_prerequisites"
  printf "*** OpenCoarrays will be installed "
  if [[ "$install_path" == "$opencoarrays_src_dir/opencoarrays-installation" ]]; then
    printf "in the above location also.        ***\n"
  else
    printf "in                                 ***\n"
    echo "$install_path"
  fi
  printf "Ready to proceed? (Y/n)"
  read install_now
  printf " $install_now\n"

  if [[ "$install_now" == "n"|| "$install_now" == "no" ]]; then
    printf "$this_script: Aborting. [exit 85]\n"
    exit 85
  fi
}

build_opencoarrays()
{
  print_header
  find_or_install mpich    &&
  find_or_install cmake    &&
  mkdir -p $build_path     &&
  cd $build_path           &&
  if [[ -z "$MPICC" || -z "$MPIFC" || -z "$CMAKE" ]]; then
    echo "Empty MPICC=$MPICC or MPIFC=$MPIFC or CMAKE=$CMAKE [exit 90]"
    exit 90
  else
    CC=$MPICC FC=$MPIFC $CMAKE .. -DCMAKE_INSTALL_PREFIX=$install_path &&
    make -j$num_threads &&
    if [[ ! -z $SUDO ]]; then
      printf "\nThe chosen installation path requires sudo privileges. Please enter password if prompted.\n"
    fi &&
    $SUDO make install
  fi
}

report_results()
{
  # Report installation success or failure:
  if [[ -x "$install_path/bin/caf" && -x "$install_path/bin/cafrun"  && -x "$install_path/bin/build" ]]; then

    # Installation succeeded
    echo "$this_script: Done."
    echo ""
    echo "*** The OpenCoarrays compiler wrapper (caf), program launcher (cafrun), and  ***"
    echo "*** prerequisite package installer (build) are in the following directory:   ***"
    echo ""
    echo "$install_path/bin."
    echo ""
    if [[ -f setup.sh ]]; then
      $SUDO rm setup.sh
    fi
    # Prepend the OpenCoarrays license to the setup.sh script:
    while IFS='' read -r line || [[ -n "$line" ]]; do
        echo "# $line" >> setup.sh
    done < "$opencoarrays_src_dir/LICENSE"
    echo "#                                                                      " >> setup.sh
    echo "# Execute this script via the following command:                       " >> setup.sh
    echo "# source $install_path/setup.sh                                        " >> setup.sh
    echo "                                                                       " >> setup.sh
    gcc_install_path=`./build gcc --default --query-path`
    if [[ -x "$gcc_install_path/bin/gfortran" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$gcc_install_path/bin\"                              " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$gcc_install_path/bin:\$PATH\"                       " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    if [[ -d "$gcc_install_path/lib" || -d "$gcc_install_path/lib64" ]]; then
      gfortran_lib_paths="$gcc_install_path/lib64/:$gcc_install_path/lib"
      echo "if [[ -z \"\$LD_LIBRARY_PATH\" ]]; then                              " >> setup.sh
      echo "  export LD_LIBRARY_PATH=\"$gfortran_lib_paths\"                     " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export LD_LIBRARY_PATH=\"$gfortran_lib_paths:\$LD_LIBRARY_PATH\"   " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    echo "                                                                       " >> setup.sh
    mpich_install_path=`./build mpich --default --query-path`
    if [[ -x "$mpich_install_path/bin/mpif90" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$mpich_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$mpich_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    cmake_install_path=`./build cmake --default --query-path`
    if [[ -x "$cmake_install_path/bin/cmake" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$cmake_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$cmake_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    flex_install_path=`./build flex --default --query-path`
    if [[ -x "$flex_install_path/bin/flex" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$flex_install_path/bin\"                             " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$flex_install_path/bin\":\$PATH                      " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    bison_install_path=`./build bison --default --query-path`
    if [[ -x "$bison_install_path/bin/yacc" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$bison_install_path/bin\"                            " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$bison_install_path/bin\":\$PATH                     " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    m4_install_path=`./build m4 --default --query-path`
    if [[ -x "$m4_install_path/bin/m4" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$m4_install_path/bin\"                               " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$m4_install_path/bin\":\$PATH                        " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    opencoarrays_install_path=$install_path
    if [[ -x "$opencoarrays_install_path/bin/caf" ]]; then
      echo "if [[ -z \"\$PATH\" ]]; then                                         " >> setup.sh
      echo "  export PATH=\"$opencoarrays_install_path/bin\"                     " >> setup.sh
      echo "else                                                                 " >> setup.sh
      echo "  export PATH=\"$opencoarrays_install_path/bin\":\$PATH              " >> setup.sh
      echo "fi                                                                   " >> setup.sh
    fi
    $SUDO mv setup.sh $opencoarrays_install_path && setup_sh_location=$opencoarrays_install_path || setup_sh_location=${PWD}
    echo "*** Before using caf, cafrun, or build, please execute the following command ***"
    echo "*** or add it to your login script and launch a new shell (or the equivalent ***"
    echo "*** for your shell if you are not using a bash shell):                       ***"
    echo ""
    echo " source $setup_sh_location/setup.sh"
    echo ""
    echo "*** Installation complete.                                                   ***"

  else # Installation failed

    echo "Something went wrong. Either the user lacks executable permissions for the"
    echo "OpenCoarrays compiler wrapper (caf), program launcher (cafrun), or prerequisite"
    echo "package installer (build), or these programs are not in the following, expected"
    echo "location:"
    echo "$install_path/bin."
    echo "Please review the following file for more information:"
    echo "$install_path/$installation_record"
    echo "and submit an bug report at https://github.com/sourceryinstitute/opencoarrays/issues"
    echo "[exit 100]"
    exit 100

  fi # Ending check for caf, cafrun, build not in expected path
}
# ___________________ End of function definitions for use in the Main Body __________________

# ________________________________ Start of the Main Body ___________________________________


if [[ $1 == '--help' || $1 == '-h' ]]; then

  # Print usage information if script is invoked with --help or -h argument
  usage | less

elif [[ $1 == '-v' || $1 == '-V' || $1 == '--version' ]]; then

  # Print script copyright if invoked with -v, -V, or --version argument
  cmake_project_line=`grep project CMakeLists.txt | grep VERSION`
  text_after_version_keyword="${cmake_project_line##*VERSION}"
  text_before_language_keyword="${text_after_version_keyword%%LANGUAGES*}"
  opencoarrays_version=$text_before_language_keyword
  echo "opencoarrays $opencoarrays_version"
  echo ""
  echo "OpenCoarrays installer"
  echo "Copyright (C) 2015-2016 Sourcery, Inc."
  echo "Copyright (C) 2015-2016 Sourcery Institute"
  echo ""
  echo "OpenCoarrays comes with NO WARRANTY, to the extent permitted by law."
  echo "You may redistribute copies of $this_script under the terms of the"
  echo "BSD 3-Clause License.  For more information about these matters, see"
  echo "http://www.sourceryinstitute.org/license.html"
  echo ""
else # Find or install prerequisites and install OpenCoarrays

  cd install_prerequisites &&
  installation_record=install-opencoarrays.log &&
  . set_SUDO.sh &&
  set_SUDO_if_necessary  &&
  CMAKE=$CMAKE build_opencoarrays 2>&1 | tee ../$installation_record
  report_results 2>&1 | tee -a ../$installation_record

fi
# ____________________________________ End of Main Body ____________________________________________
