#!/bin/sh
echo "Building the code:"
make clean
make -f Makefile.inst

# Specify TAU parameters here:
export TAU_CALLPATH=1
export TAU_CALLPATH_DEPTH=100
#export TAU_SAMPLING=1

echo "Running the code:"
./burgers_static_caf

echo "Running the pprof command:"
pprof
echo "Running the TAU paraprof analyzer command:"
paraprof &


