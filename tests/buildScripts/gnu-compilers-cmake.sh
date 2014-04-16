#!/bin/sh
SOURCE_PATH=../src
EXTRA_ARGS=$@

rm -f CMakeCache.txt
cmake \
  -D CMAKE_BUILD_TYPE:STRING=DEBUG \
  -D CMAKE_Fortran_COMPILER:FILEPATH=gfortran \
  -D CMAKE_C_COMPILER:FILEPATH=gcc \
  -D CMAKE_Fortran_FLAGS:STRING="-std=f2008 -Wall -fcheck=all -fcoarray=single" \
  -D CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE \
$EXTRA_ARGS \
$SOURCE_PATH
