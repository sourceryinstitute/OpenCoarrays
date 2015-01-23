#!/bin/sh
SOURCE_PATH=../opencoarrays
EXTRA_ARGS=$@

rm -rf CMakeCache.txt CMakeFiles
cmake \
  -D CMAKE_BUILD_TYPE:STRING=DEBUG \
  -D CMAKE_C_COMPILER:FILEPATH=gcc \
  -D CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE \
  -D CMAKE_INSTALL_PREFIX:PATH=${PWD}/root \
$EXTRA_ARGS \
$SOURCE_PATH

#  -D CMAKE_Fortran_COMPILER:FILEPATH=gfortran \
#  -D CMAKE_Fortran_FLAGS:STRING="-std=f2008 -Wall -fcheck=all -fcoarray=single" \
