#!/bin/sh
# Specify the path to OpenCoarrays source tree (be sure your build directory is not 
# inside the specific path):
SOURCE_PATH=../src
EXTRA_ARGS=$@

# If the LD_LIBRARY_PATH environment variable is not set on the platform where you 
# are building, either set it to the location of libcaf_mpi.a on your system or prepend 
# the path to the  quoted text in the CMAKE_Fortran_FLAGS argument below. For example,
# -D CMAKE_Fortran_FLAGS:STRING="-L/opt/opencoarrays/lib -fcoarray=lib -Wall -fcheck=all -Ofast " \

# The LEGACY_ARCHITECTURE argument is used only in the test suite to determine whether
# to link to an FFT library with the older Intel SSE instructions or newer AVX instructions.

# Edit the CMAKE_INSTALL_PREFIX argument to the installation destination of your choice.
# The default used below is the present working directory.

rm -rf CMakeCache.txt CMakeFiles
cmake \
  -D CMAKE_Fortran_COMPILER:FILEPATH=mpif90 \
  -D CMAKE_Fortran_FLAGS:STRING="-fcoarray=lib -Wall -fcheck=all -Ofast " \
  -D CMAKE_BUILD_TYPE:STRING=DEBUG \
  -D CMAKE_C_COMPILER:FILEPATH=gcc \
  -D CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE \
  -D LEGACY_ARCHITECTURE:BOOL=TRUE \
  -D CMAKE_INSTALL_PREFIX:PATH=${PWD} \
  -D CMAKE_TOOLCHAIN_FILE:FILEPATH=${SOURCE_PATH}/../buildScripts/linux-toolchain.cmake \
$EXTRA_ARGS \
$SOURCE_PATH
