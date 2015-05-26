set(CMAKE_Fortran_FLAGS "-std=f2008 -fcoarray=lib -Wall -fcheck=all -Ofast -Wintrinsics-std")
set(CMAKE_SYSTEM_NAME Linux)
include(CMakeForceCompiler)
CMAKE_FORCE_Fortran_COMPILER(/opt/mpich/3.1.2/gnu/5.0/bin/mpif90 GNU)
