#!/bin/sh

if ! command -v mpifort > /dev/null 2>&1; then
  brew install open-mpi
fi

fpm $@ \
  --compiler `which mpifort` \
  --c-compiler `which mpicc` \
  --c-flag "-DPREFIX_NAME=_gfortran_caf_ -DGCC_GE_7 -DGCC_GE_8" \
  --flag "-fcoarray=lib" \
  --runner "mpirun -n 4"
