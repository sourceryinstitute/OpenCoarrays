#!/bin/sh
SOURCE_PATH=../src
EXTRA_ARGS=$@

rm -f CMakeCache.txt
cmake \
  -D CMAKE_Fortran_COMPILER:FILEPATH=ifort \
  -D CMAKE_C_COMPILER:FILEPATH=icc \
  -D CMAKE_Fortran_FLAGS:STRING="-standard-semantics -coarray=shared -coarray-num-images=2 -O3" \
  -D CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE \
$EXTRA_ARGS \
$SOURCE_PATH
