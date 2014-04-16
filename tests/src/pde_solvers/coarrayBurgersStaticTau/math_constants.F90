module math_constants
#include "compiler_capabilities.txt"
  use kind_parameters, only : ikind
  implicit none
  public :: grid_resolution, local_grid_size

  ! Because of a memory leak in the Intel compiler, we use static memory rather than dynamic.  This necessitates
  ! hardwiring the relationship between the grid_resolution, local_grid_size, and the number of images.
  ! An assertion in main.f90 checks whether local_grid_size*num_images() == grid_resolution.  Set the number of
  ! images either with the compiler flag "-coarray_num_images=" or override any compile-time setting by defining
  ! the environment variable FOR_COARRAY_NUM_IMAGES (e.g., use "export FOR_COARRAY_NUM_IMAGES=4" in the bash shell).
#ifndef COMPILER_LACKS_MULTI_IMAGE_COARRAYS
  integer(ikind), parameter :: grid_resolution = 1024 !134217728_ikind !8589934592_ikind !12884901888 !2097152 !100663296
  integer(ikind), parameter :: local_grid_size = 256
#else
  integer(ikind), parameter :: grid_resolution = 1024 !134217728_ikind !8589934592_ikind !12884901888 !2097152 !100663296
  integer(ikind), parameter :: local_grid_size = 1024
#endif
  
end module
