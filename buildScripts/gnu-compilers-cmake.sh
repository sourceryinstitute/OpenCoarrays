#!/bin/sh
SOURCE_PATH=../src
EXTRA_ARGS=$@

rm -rf CMakeCache.txt CMakeFiles
cmake \
  -D CMAKE_Fortran_COMPILER:FILEPATH=mpif90 \
  -D CMAKE_Fortran_FLAGS:STRING="-fcoarray=lib -Wall -fcheck=all -Ofast " \
  -D CMAKE_BUILD_TYPE:STRING=DEBUG \
  -D CMAKE_C_COMPILER:FILEPATH=gcc \
  -D CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE \
  -D LEGACY_ARCHITECTURE:BOOL=TRUE \
  -D CMAKE_INSTALL_PREFIX:PATH=${PWD} \
  -D CMAKE_TOOLCHAIN_FILE:FILEPATH=../toolchain.cmake \
$EXTRA_ARGS \
$SOURCE_PATH

# Edit the library path below and insert this line above if the LD_LIBRARY_PATH environment variable is not set:
#  -D CMAKE_Fortran_FLAGS:STRING="-L/opt/opencoarrays/lib -fcoarray=lib -Wall -fcheck=all -Ofast " \
