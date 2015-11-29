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
# 3. Switch from hardwiring prerequisite minimum version numbers to querying 'build' 
#    script for default version numbers.
#
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

# Verify that the top-level OpenCoarrays source diretory is either present working directory 
# or is specified by a valid in the OPENCOARRAYS_SRC_DIR environment variable 
if [[ -d $OPENCOARRAYS_SRC_DIR ]]; then
  opencoarrays_src_dir=$OPENCOARRAYS_SRC_DIR 
else
  opencoarrays_src_dir=${PWD}
fi
cd $opencoarrays_src_dir || directory_exists=false
if [[ $directory_exists == "false" ]]; then
  echo "The OPENCOARRAYS_SRC_DIR environment variable specifies a non-existent directory [exit 10]:"
  echo "$OPENCOARRAYS_SRC_DIR"
  exit 10
fi

printf "\nPWD=${PWD}\n"
build_path=$opencoarrays_src_dir/opencoarrays-build
build_script=$opencoarrays_src_dir/install_prerequisites/build
if [[ -x $build_script ]]; then
  echo "$this_script: found $build_script script and user has executable permission for it."
else
  echo "$this_script: $build_script script does not exist or the user lacks executable permission for it."
  echo "$this_script: Please run this_script in the top-level OpenCoarrays source directory or set the"
  echo "$this_script: OPENCOARRAYS_SRC_DIR environment variable to the top-level OpenCoarrays source path."
  echo "$this_script: If you have specified an installation director that requires aminstrative privileges,"
  echo "$this_script: please prepend 'sudo -E' to your invocation of the script [exit 20]."
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
    echo "   -h, --help       Show this help message"
    echo "   -v, --version    Show version information"
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
  else 
    printf "no.\n"
    package_in_path=false
  fi
  
  package_install_path=`./build $package --default --query` 

  printf "$this_script: Checking whether $executable is in the default installation directory that $this_script uses for it..."
  if type $package_install_path/bin/$executable > /dev/null; then 
    printf "yes.\n"
    script_installed_package=true
    stack_push script_installed $package $executable
    echo "script_installed stack:"
    stack_print script_installed
  else 
    script_installed_package=false
    printf "no.\n"
  fi

  if [[ "$package" == "cmake" ]]; then

    # We arrive here only by the explicit, direct call 'find_or_install cmake' inside 
    # the build_opencoarrays function. Because there is no possibility of arriving here 
    # by recursion (no packages depend on cmake except OpenCoarrays, which gets built
    # after all dependencies have been found or installed), cmake must add itself to 
    # the dependency stack if no acceptable cmake is found.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package installed by $this_script\n"
      export CMAKE=$package_install_path/bin/cmake
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then
      printf "$this_script: Checking whether $package in PATH is version < 3.2.0... "

      if [[ `cmake --version|head -1` < "cmake version 3.2.0" ]]; then
        printf "yes.\n"
        stack_push dependency_pkg $package "none" 
        stack_push dependency_exe $package "none" 
        stack_push dependency_path `./build cmake --default --query` "none" 

      else 
        printf "no.\n"
        printf "$this_script: Using the $executable found in the PATH.\n"
        export CMAKE=$package
        stack_push acceptable_in_path $package $executable
        # Prevent recursion
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else # Build package ($package has no prerequisites)
      stack_push dependency_pkg $package "none"
      stack_push dependency_exe $package "none"
      stack_push dependency_path `./build $package --default --query` "none" 
    fi

  elif [[ $package == "mpich" ]]; then

    # We arrive here only by the explicit, direct call 'find_or_install mpich' inside 
    # the build_opencoarrays function. Because there is no possibility of arriving here 
    # by recursion (no packages depend on mpich except OpenCoarrays, which gets built
    # after all dependencies have been found or installed), mpich must add itself to 
    # the dependency stack if no acceptable mpich is found.

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
        # Halt the recursion
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      else
        printf "no.\n"
        acceptable_gcc_in_path=`stack_print acceptable_in_path | grep gcc`
        script_installed_gcc=`stack_print script_installed | grep gcc`
        if [[ -z $acceptable_gcc_in_path && -z $script_installed_gcc ]]; then
          # Build $package and prerequisites
          stack_push dependency_pkg $package "gcc"
          stack_push dependency_exe $package "gfortran"
          stack_push dependency_path $package `./build gcc --default --query` 
        else 
          # Build $package with existing prerequisites
          stack_push dependency_pkg $package "none"
          stack_push dependency_exe $executable "none"
          stack_push dependency_path `./build mpich --default --query` "none"
        fi
        if [[ ! -z $script_installed_gcc ]]; then
          gcc_install_path=`./build gcc --default --query`
          export CC=$gcc_install_path/bin/gcc
          export CXX=$gcc_install_path/bin/g++
          export FC=$gcc_install_path/bin/gfortran
          export FC=$gcc_install_path/bin/gfortran
        fi
      fi

    else # $package not in PATH and not yet installed by this script
      # This conditional prevents an infinite loop by ensuring gcc only pushes
      # flex onto the stack if flex has not already been found or installed.
      acceptable_gcc_in_path=`stack_print acceptable_in_path | grep gcc`
      script_installed_gcc=`stack_print script_installed | grep gcc`
      if [[ -z $acceptable_gcc_in_path && -z $script_installed_gcc ]]; then
        stack_push dependency_pkg  "mpich" "gcc" 
        stack_push dependency_exe  "mpich" "gfortran" 
        stack_push dependency_path `./build mpich --default --query`  `./build gcc --default --query`
      fi
    fi

  elif [[ $package == "gcc" ]]; then

    # We arrive when the 'elif [[ $package == "mpich" ]]' block pushes "gcc" onto the
    # the dependency_pkg stack, resulting in the recursive call 'find_or_install gcc'

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package executable $executable installed by $this_script\n"
      export FC=$package_install_path/bin/gfortran
      export CC=$package_install_path/bin/gcc
      export CXX=$package_install_path/bin/g++
      if [[ -z "$LD_LIBRARY_PATH" ]]; then 
        export LD_LIBRARY_PATH=$package_install_path/lib/
      else
        export LD_LIBRARY_PATH=$package_install_path/lib/:$LD_LIBRARY_PATH
      fi
      echo "Exporting LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
      # Remove $package from the dependency stack 
      stack_pop dependency_pkg trash
      stack_pop dependency_exe trash
      stack_pop dependency_path trash
      # Halt the recursion and signal that none of $package's prerequisites need to be built
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

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
        # Remove $package from the dependency stack 
        stack_pop dependency_pkg trash
        stack_pop dependency_exe trash
        stack_pop dependency_path trash
        # Halt the recursion and signal that none of $package's prerequisites need to be built
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      else
        printf "no.\n"
        # This conditional prevents an infinite loop by ensuring gcc only pushes
        # flex onto the stack if flex has not already been found or installed.
        acceptable_flex_in_path=`stack_print acceptable_in_path | grep flex`
        script_installed_flex=`stack_print script_installed | grep flex`
        if [[ -z $acceptable_flex_in_path && -z $script_installed_flex ]]; then
          stack_push dependency_pkg "flex" 
          stack_push dependency_exe "flex" 
          stack_push dependency_path `./build flex --default --query` 
        fi
      fi

    else # $package not in PATH and not yet installed by this script
      stack_push dependency_pkg "flex"
      stack_push dependency_exe "flex"
      stack_push dependency_path `./build flex --default --query`
    fi

  elif [[ $package == "flex" ]]; then

    # We arrive here only if the 'elif [[ $package == "gcc" ]]' block has pushed "flex" 
    # onto the dependency_pkg stack, resulting in the recursive call 'find_or_install flex'.
    # flex therefore does not need to add itself to the stack.

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $executable installed by $this_script\n"
      export FLEX=$package_install_path/bin/$executable
      # Remove flex from the dependency stack 
      stack_pop dependency_pkg trash
      stack_pop dependency_exe trash
      stack_pop dependency_path trash
      # Halt the recursion and signal that no prerequisites need to be built
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

    elif [[ "$package_in_path" == "true" ]]; then

      printf "$this_script: Checking whether $package in PATH is version < 2.6.0... "
      if [[ `flex --version` < "flex 2.6.0" ]]; then
        printf "yes\n"

        # Add the flex prequisite "bison" to the dependency stack if the flex version in PATH is insufficient
        # and the flex prerequisite is not already in the stack. (The 'if' prevents an infinite loop flex 
        # wherein flex repeateldy adds 'bison' after 'bison' removes itself from the stack.)
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
        # Halt the recursion by removing flex from the dependency stack and indicating 
        # that none of the prerequisites required to build flex are needed.
        stack_push dependency_pkg "none"
        stack_push dependency_exe "none"
        stack_push dependency_path "none"
      fi

    else 
      # Add the flex prequisite "bison" to the dependency stack if flex is not in the PATH 
      # and not yet installed by $this_script and the flex prerequisite is not already in 
      # the stack. (The 'if' prevents an infinite loop wherein flex repeateldy adds 'bison' 
      # after 'bison' removes itself from the stack.)
      acceptable_bison_in_path=`stack_print acceptable_in_path | grep bison`
      script_installed_bison=`stack_print script_installed | grep bison`
      if [[ -z $acceptable_bison_in_path && -z $script_installed_bison ]]; then
        stack_push dependency_pkg "bison"
        stack_push dependency_exe "yacc"
        stack_push dependency_path `./build bison --default --query` 
      fi
    fi

  elif [[ $package == "bison" ]]; then

    # We arrive when the 'elif [[ $package == "flex" ]]' block pushes "bison" onto the
    # the dependency_pkg stack, resulting in the recursive call 'find_or_install bison'

    if [[ "$script_installed_package" == true ]]; then
      printf "$this_script: Using the $package executable $executable installed by $this_script\n"
      export YACC=$package_install_path/bin/yacc
      # Remove bison from the dependency stack 
      stack_pop dependency_pkg trash
      stack_pop dependency_exe trash
      stack_pop dependency_path trash
      # Halt the recursion and signal that no prerequisites need to be built
      stack_push dependency_pkg "none"
      stack_push dependency_exe "none"
      stack_push dependency_path "none"

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

    else # $package not in PATH and not yet installed by this script
      stack_push dependency_pkg  "none"
      stack_push dependency_exe  "none"
      stack_push dependency_path "none"
    fi

  else 
    printf "$this_script: unknown package name ($package) passed to find_or_install function. [exit 50]\n"
    exit 50 
  fi 

  if [[ -z $CC ]]; then
    CC=gcc
  fi
  if [[ -z $CXX ]]; then
    CXX=g++
  fi
  if [[ -z $FC ]]; then
    FC=gfortran
  fi

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

  echo "$this_script: Remaining dependency stack (top to bottom = left to right):"
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

    printf "$this_script: Ok to downloand, build, and install $package from source? (y/n) "
    read proceed_with_build

    if [[ $proceed_with_build != "y" ]]; then

      printf "\n$this_script: OpenCoarrays installation requires $package. Aborting. [exit 70]\n"
      exit 70

    else # permission granted to build

      script_installed_gcc=`stack_print script_installed | grep gcc`
      if [[ ! -z $script_installed_gcc ]]; then
        gcc_install_path=`./build gcc --default --query`
        CC=$gcc_install_path/bin/gcc
        CXX=$gcc_install_path/bin/g++
        FC=$gcc_install_path/bin/gfortran
      fi
  
      printf "$this_script: Downloading, building, and installing $package \n"
      echo "$this_script: Build command: FC=$FC CC=$CC CXX=$CXX ./build $package --default $package_install_path $num_threads"
      FC=$FC CC=$CC CXX=$CXX ./build $package --default $package_install_path $num_threads
    
      if [[ -f $package_install_path/bin/$executable ]]; then
        printf "$this_script: Installation successful.\n"
        if [[ "$package" == "$executable" ]]; then
          printf "$this_script: $executable is in $package_install_path/bin \n"
        else
          printf "$this_script: $packge executable $executable is in $package_install_path/bin \n"
        fi
        export PATH=$package_install_path/bin:$PATH
      else
        printf "$this_script: Installation unsuccessful. "
        printf "$package is not in the following expected path:\n"
        printf "$package_install_path/bin \n"
        printf "Aborting. [exit 80]"
        exit 80
      fi # End 'if [[ -f $package_install_path/bin/$executable ]]' 

    fi # End 'if [[ $proceed_with_build == "y" ]]; then'
     
  fi # End 'if [[ "$package" != "none" ]]; then'
}

build_opencoarrays()
{
  # A default OpenCoarrays build requires CMake and MPICH. Secondary 
  # dependencies will be found or installed along the way.
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
    make install        
  fi
}

report_results()
{
  # Report installation success or failure:
  if [[ -x $install_path/bin/caf && -x $install_path/bin/cafrun  && -x $install_path/bin/build ]]; then

    # Installation succeeded
    echo "$this_script: Done."
    echo ""
    echo "*** The OpenCoarrays compiler wrapper (caf), program launcher (cafrun), and  ***"
    echo "*** prerequisite package installer (build) are in the following directory:   ***"
    echo ""
    echo "$install_path/bin."
    echo ""
    # Prepend the OpenCoarrays license to the setup.sh script:
    while IFS='' read -r line || [[ -n "$line" ]]; do
        echo "# $line" >> $install_path/setup.sh
    done < "$opencoarrays_src_dir/COPYRIGHT-BSD3"
    echo "# Execute this script via the folowing commands:                     " >> $install_path/setup.sh
    echo "# cd $install_path                                                   " >> $install_path/setup.sh
    echo "# source setup.sh                                                    " >> $install_path/setup.sh
    echo "                                                                     " >> $install_path/setup.sh
    echo "if [[ -z \$PATH ]]; then                                             " >> $install_path/setup.sh
    echo "  export PATH=$install_path/bin                                      " >> $install_path/setup.sh
    echo "else                                                                 " >> $install_path/setup.sh
    echo "  export PATH=$install_path/bin:\$PATH                               " >> $install_path/setup.sh
    echo "fi                                                                   " >> $install_path/setup.sh
    echo "                                                                     " >> $install_path/setup.sh
    gcc_install_path=`./build gcc --default --query`
    if [[ -d $gcc_install_path/lib ]]; then 
      echo "if [[ -z \$LD_LIBRARY_PATH ]]; then                                " >> $install_path/setup.sh
      echo "  export LD_LIBRARY_PATH=$gcc_install_path/lib                     " >> $install_path/setup.sh
      echo "else                                                               " >> $install_path/setup.sh
      echo "  export LD_LIBRARY_PATH=$gcc_install_path/lib:\$LD_LIBRARY_PATH   " >> $install_path/setup.sh
      echo "fi                                                                 " >> $install_path/setup.sh
    fi
    echo "                                                                     " >> $install_path/setup.sh
    mpich_install_path=`./build mpich --default --query`
    if [[ -x $mpich_install_path/bin/mpif90 ]]; then 
      echo "if [[ -z \$PATH ]]; then                                           " >> $install_path/setup.sh
      echo "  export PATH=$mpich_install_path/bin                              " >> $install_path/setup.sh
      echo "else                                                               " >> $install_path/setup.sh
      echo "  export PATH=$mpich_install_path/bin:\$PATH                       " >> $install_path/setup.sh
      echo "fi                                                                 " >> $install_path/setup.sh
    fi
    echo "*** Before using caf, cafrun, or build, please execute the following command ***"
    echo "*** or add it to your login script and launch a new shell (or the equivalent ***"
    echo "*** for your shell if you are not using a bash shell):                       ***"
    echo ""
    echo " source $install_path/setup.sh"

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
  echo "OpenCoarrays installer"
  echo "Copyright (C) 2015 Sourcery, Inc."
  echo "Copyright (C) 2015 Sourcery Institute"
  echo ""
  echo "$this_script comes with NO WARRANTY, to the extent permitted by law."
  echo "You may redistribute copies of $this_script under the terms of the"
  echo "BSD 3-Clause License.  For more information about these matters, see"
  echo "http://www.sourceryinstitute.org/license.html"
  echo ""
  echo "For version information, please see the CMakeLists.txt file in the top-level "
  echo "OpenCoarrays source directory or execute 'caf --version' or 'caf -v' after "
  echo "a successful execution of this installer."

else # Find or install prerequisites and install OpenCoarrays
  
  cd install_prerequisites &&
  installation_record=install-opencoarrays.log &&
  build_opencoarrays 2>&1 | tee ../$installation_record
  report_results 2>&1 | tee -a ../$installation_record

fi 
# ____________________________________ End of Main Body ____________________________________________
