#!/bin/sh

if ! command -v mpifort > /dev/null 2>&1; then
  brew install open-mpi
fi

MPIFC=`which mpifort`
MPICC=`which mpicc`

fpm build \
  --compiler $MPIFC \
  --c-compiler $MPICC \
  --c-flag "-DPREFIX_NAME=_gfortran_caf_ -DGCC_GE_7 -DGCC_GE_8" \
  --flag "-fcoarray=lib"

fpm run --example \
  --compiler $MPIFC \
  --c-compiler $MPICC \
  --c-flag "-DPREFIX_NAME=_gfortran_caf_ -DGCC_GE_7 -DGCC_GE_8" \
  --flag "-fcoarray=lib" \
  --runner "mpirun -n 4"
