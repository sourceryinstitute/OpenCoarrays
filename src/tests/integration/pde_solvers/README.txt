This directory contains three one-dimensional partial differential equation
(PDE) solvers that exercise the parallel programming features of Fortran
2008: coarrays and "do concurrent".  The solvers are in the following three
directories:

   (1) coarrayHeatSimplified
   (2) coarrayBurgers
   (3) coarrayBurgersStaticTau

The first second and third directories above depend on files in the include-files
and library directories.

The heat equation solver (1) is the most stringent in terms of correctness
because it checks for the exact solution at every point across the problem
domain.  I suggest starting with the heat equation solver to check compiler
correctness before moving to the other solvers.

The raw Burgers equation solver (2) is slightly more complicated than the
heat equation solver in that the Burgers solver adds one additional term
(a nonlinear term), the calculation of which imposes an additional
synchronization requirement.  However, the correctness test in the Burgers
equation solver is weak: it checks only one value in the problem domain and
that point is a point that should remain zero for all time.

The instrumented Burgers solver (3) is best for performance studies.  It has
been instrumented for performance analysis witih the Tuning and Analysis
Utilities (TAU), which is open-source software available at
http://tau.uoregon.edu.  Because of a memory leak in the Intel compiler,
the instrumented solver uses static rather than dynamic memory.  For this
reason, the code needs to be recompiled for each desired problem size.  See
coarrayBurgersStaticTau/math_constants.F90 for an important comment regarding
matching the problem size with the number of images.

The instrumented Burgers solver has been demonstrated to execute with 99%
parallel efficiency on over 13,000 cores and 87% parallel efficiency on 16,384
cores in weak scaling when compiled with the Cray Compiler Environment.  For
new scalabiliby studies, it is important to run problems of sufficient size.
See one of the following two publications for additional information:

[1] Haveraaen, M., K. Morris, D. W. I. Rouson, H. Radhakrishnan, and C. Carson
(2014) “High- performance design patterns for modern Fortran,” Scientific
Programming, in review.

[2] Haveraaen, M., K. Morris, and D. W. I. Rouson (2013) “High-performance design
patterns for modern Fortran,” First International Workshop on Software
Engineering for High Performance Computing in Computational Science and
Engineering, Denver, Colorado, USA. November 22.
