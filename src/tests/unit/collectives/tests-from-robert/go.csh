#!/bin/csh
#
if ( `caf -V | head -2 | tail -1 | awk '{print substr($0,1,4)}'` != "Open" ) then
    CAF_COMPILER
endif
#
rm test1
rm test1.log
rm test2
rm test2.log
rm *.o
rm *.mod
#
caf -ffree-form -ffree-line-length-0 -fbacktrace -g3 -O0 -o test1 test1.f90
cafrun -np 2 test1 >& test1.log
#
caf -ffree-form -ffree-line-length-0 -fbacktrace -g3 -O0 -o test2 test2.f90
cafrun -np 2 test2 >& test2.log
#
echo 'test1 allocation sizes on compute image'
grep "Values:" test1.log
echo 'test1 co_broadcasts on master image'
grep "co_bro:" test1.log
echo 'test1 Read and Write data'
grep "L1:" test1.log
#
echo 'test2 allocation sizes on compute image'
grep "Values:" test2.log
echo 'test2 co_broadcasts on master image'
grep "co_bro:" test2.log
echo 'test2 Read and Write data'
grep "L:" test2.log
