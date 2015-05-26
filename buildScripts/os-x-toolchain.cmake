set(CMAKE_Fortran_FLAGS "-std=f2008 -fcoarray=lib -Wall -fcheck=all -Ofast -Wintrinsics-std")
set(CMAKE_SYSTEM_NAME Darwin)
include(CMakeForceCompiler)
CMAKE_FORCE_Fortran_COMPILER(mpif90 GNU)
