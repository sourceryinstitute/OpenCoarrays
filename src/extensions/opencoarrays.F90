! Fortran 2015 feature support for Fortran 2008 compilers
!
! Copyright (c) 2015-2018, Sourcery, Inc.
! Copyright (c) 2015-2018, Sourcery Institute
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!     * Redistributions of source code must retain the above copyright
!       notice, this list of conditions and the following disclaimer.
!     * Redistributions in binary form must reproduce the above copyright
!       notice, this list of conditions and the following disclaimer in the
!       documentation and/or other materials provided with the distribution.
!     * Neither the name of the Sourcery, Inc., nor the
!       names of its contributors may be used to endorse or promote products
!       derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE FOR ANY
! DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
! (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
! LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
! ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!
module opencoarrays
  use iso_c_binding, only : c_int,c_char,c_ptr,c_loc,c_int32_t,c_sizeof
  implicit none

  include 'mpif.h' ! Let's also try replacing this with 'use mpi_f08'

  private
  public :: co_sum
  public :: this_image
  public :: num_images
  public :: error_stop
  public :: sync_all
  public :: caf_init
  public :: caf_finalize

  integer, parameter :: c_ptrdiff_t=c_int

  interface co_sum
    !! Generic interface to co_sum with implementations for various types, kinds, and ranks
     module procedure co_sum_c_int
  end interface

  ! Enumeration from ../libcaf-gfortran-descriptor.h:
  !  enum
  !{ BT_UNKNOWN = 0, BT_INTEGER, BT_LOGICAL, BT_REAL, BT_COMPLEX,
  !  BT_DERIVED, BT_CHARACTER, BT_CLASS, BT_PROCEDURE, BT_HOLLERITH, BT_VOID,
  !  BT_ASSUMED
  !};

  enum ,bind(C)
    enumerator :: &
     BT_UNKNOWN = 0, BT_INTEGER, BT_LOGICAL, BT_REAL, BT_COMPLEX, &
     BT_DERIVED, BT_CHARACTER, BT_CLASS, BT_PROCEDURE, BT_HOLLERITH, BT_VOID, &
     BT_ASSUMED
  end enum

  ! Type definition from ../libcaf-gfortran-descriptor.h:
  !typedef struct descriptor_dimension
  !{
  !  ptrdiff_t _stride;
  !  ptrdiff_t lower_bound;
  !  ptrdiff_t _ubound;
  !}
  !descriptor_dimension;

  ! Fortran derived type interoperable with like-named C type:
  type, bind(C) :: descriptor_dimension
     integer(c_ptrdiff_t) :: stride
     integer(c_ptrdiff_t) :: lower_bound
     integer(c_ptrdiff_t) :: ubound_
  end type

  ! Type definition from ../libcaf-gfortran-descriptor.h:
  !typedef struct gfc_descriptor_t {
  !  void *base_addr;
  !  size_t offset;
  !  ptrdiff_t dtype;
  !  descriptor_dimension dim[];
  !} gfc_descriptor_t;

  integer, parameter :: max_dimensions=15

  ! Fortran derived type interoperable with like-named C type:
  type, bind(C) :: gfc_descriptor_t
    type(c_ptr) :: base_addr
    integer(c_ptrdiff_t) :: offset
    integer(c_ptrdiff_t) :: dtype
    type(descriptor_dimension) :: dim_(max_dimensions)
  end type

  ! C comment and source from ../libcaf.h
  ! /* When there is a vector subscript in this dimension, nvec == 0, otherwise,
  ! lower_bound, upper_bound, stride contains the bounds relative to the declared
  ! bounds; kind denotes the integer kind of the elements of vector[].  */
  ! type, bind(C) :: caf_vector_t {
  !   size_t nvec;
  !   union {
  !     struct {
  !       void *vector;
  !       int kind;
  !     } v;
  !     struct {
  !       ptrdiff_t lower_bound, upper_bound, stride;
  !     } triplet;
  !   } u;
  ! }
  ! caf_vector_t;

  type, bind(C) :: v_t
    type(c_ptr) :: vector
    integer(c_int) :: kind_
  end type

  type, bind(C) :: triplet_t
    integer(c_ptrdiff_t) :: lower_bound, upper_bound, stride
  end type

  type, bind(C) :: u_t
     type(v_t) :: v
     type(triplet_t) :: triplet
  end type

  type, bind(C) :: caf_vector_t
    integer(c_ptrdiff_t) :: nvec
    type(u_t) :: u
  end type

  ! --------------------

 !integer(c_int), save, volatile, bind(C,name="CAF_COMM_WORLD") :: CAF_COMM_WORLD
  integer(c_int), bind(C,name="CAF_COMM_WORLD") :: CAF_COMM_WORLD
  integer(c_int32_t), parameter  :: bytes_per_word=4_c_int32_t

  interface gfc_descriptor
    module procedure gfc_descriptor_c_int
  end interface

  interface
    !! Bindings for OpenCoarrays C procedures in the application binary interface (ABI) defined in libcaf*.h

    subroutine caf_init(argc,argv) bind(C,name="_gfortran_caf_init")
      !! C prototype (../mpi/mpi_caf.c): void init (int *argc, char ***argv)
      import :: c_int,c_ptr
      type(c_ptr), value :: argc
      type(c_ptr), value :: argv
    end subroutine

    subroutine caf_finalize() bind(C,name="_gfortran_caf_finalize")
      !! C prototype (../mpi/mpi_caf.c): void PREFIX (finalize) (void)
    end subroutine

    subroutine opencoarrays_co_sum(a, result_image, stat, errmsg, errmsg_len) bind(C,name="_gfortran_caf_co_sum")
      !! C prototype (../mpi/mpi_caf.c): 
      !! void co_sum (gfc_descriptor_t *a, int result_image, int *stat, char *errmsg, int errmsg_len)
      import :: c_int,c_char,c_ptr
      type(c_ptr), intent(in), value :: a
      integer(c_int), intent(in),  value :: result_image,errmsg_len
      integer(c_int), intent(out), optional :: stat
      character(kind=c_char), intent(out), optional :: errmsg(*)
    end subroutine

    function opencoarrays_this_image(coarray) bind(C,name="_gfortran_caf_this_image") result(image_num)
      !! C prototype (../mpi/mpi_caf.c): int PREFIX (this_image) (int);
      import :: c_int
      integer(c_int), value, intent(in) :: coarray
      integer(c_int)  :: image_num
    end function

    
    function opencoarrays_num_images(coarray,dim_) bind(C,name="_gfortran_caf_num_images") result(num_images_)
      !! C prototype (../mpi/mpi_caf.c): int PREFIX (num_images) (int, int);
      import :: c_int
      integer(c_int), value, intent(in) :: coarray,dim_
      integer(c_int) :: num_images_
    end function

    
    subroutine opencoarrays_error_stop(stop_code) bind(C,name="_gfortran_caf_error_stop")
      !! C prototype (../mpi_caf.c): void PREFIX (error_stop) (int32_t) __attribute__ ((noreturn));
      import :: c_int32_t
      integer(c_int32_t), value, intent(in) :: stop_code
    end subroutine

    
    subroutine opencoarrays_sync_all(stat,errmsg,unused) bind(C,name="_gfortran_caf_sync_all")
      !! C prototype (../mpi_caf.c): void PREFIX (sync_all) (int *, char *, int);
      import :: c_int,c_char
      integer(c_int), intent(out) :: stat,unused
      character(c_char), intent(out) :: errmsg(*)
    end subroutine

  end interface


contains

  ! __________ Descriptor constructors for for each supported type and kind ____________
  ! ____________________________________________________________________________________

  function my_dtype(type_,kind_,rank_) result(dtype_)
    integer,parameter :: GFC_DTYPE_SIZE_SHIFT = 8, GFC_DTYPE_TYPE_SHIFT=3
    integer(c_int32_t), intent(in) :: type_,kind_,rank_
    integer(c_int32_t) :: dtype_

    ! SIZE Type Rank
    ! 0000  000  000

    ! Rank is represented in the 3 least significant bits
    dtype_ = ior(0_c_int32_t,rank_)
    ! The next three bits represent the type id as expressed in libcaf-gfortran-descriptor.h
    dtype_ = ior(dtype_,ishft(type_,GFC_DTYPE_TYPE_SHIFT))
    ! The most significant bits represent the size of a the type (single or double precision).
    ! We can express the precision in terms of 32-bit words: 1 for single, 2 for double.
    dtype_ = ior(dtype_,ishft(kind_,GFC_DTYPE_SIZE_SHIFT))

  end function

  function gfc_descriptor_c_int(a) result(a_descriptor)
    integer(c_int), intent(in), target, contiguous :: a(:) ! should be assumed-rank
    type(gfc_descriptor_t) :: a_descriptor
    integer(c_int), parameter :: unit_stride=1,scalar_offset=-1
    integer(c_int) :: i, c_sizeof_a

    integer(c_int), parameter :: rank_a=1_c_int
      !! hardwired workaround for a lack of support for Fortran 2018 "rank" intrinsic function

    a_descriptor%dtype = my_dtype(type_=BT_INTEGER,kind_=int(c_sizeof(a)/bytes_per_word,c_int32_t),rank_=rank_a)
    a_descriptor%offset = scalar_offset
    a_descriptor%base_addr = c_loc(a) ! data
    do i=1,rank_a ! should be concurrent
      a_descriptor%dim_(i)%stride  = unit_stride
      a_descriptor%dim_(i)%lower_bound = lbound(a,i)
      a_descriptor%dim_(i)%ubound_ = ubound(a,i)
    end do

  end function

  ! ________ Assumed-rank co_sum wrappers for each supported type and kind _____________
  ! ____________________________________________________________________________________

  subroutine co_sum_c_int(a,result_image,stat,errmsg)
    integer(c_int), intent(inout), volatile, target, contiguous :: a(:) ! should be assumed-rank
    integer(c_int), intent(in), optional :: result_image
    integer(c_int), intent(out), optional:: stat
    character(kind=1,len=*), intent(out), optional :: errmsg
    ! Local variables and constants:
    integer(c_int), parameter :: default_result_image=0
    type(gfc_descriptor_t), target :: a_descriptor
    integer(c_int) :: result_image_ ! Local replacement for the corresponding intent(in) dummy argument

    a_descriptor = gfc_descriptor(a)
    result_image_ = merge(result_image,default_result_image,present(result_image))
    call opencoarrays_co_sum(c_loc(a_descriptor),result_image_, stat, errmsg, len(errmsg))

  end subroutine

  function this_image()  result(image_num)
    !! Return the image number (MPI rank + 1)
    use mpi, only : MPI_Comm_rank
    integer(c_int) :: image_num,ierr
    if (.true.) then
      call MPI_Comm_rank(CAF_COMM_WORLD,image_num,ierr)
      if (ierr/=0) call error_stop
      image_num = image_num + 1
    else
      !! alternative, simpler implementation (doesn't work with gfortran)
      ! image_num = opencoarrays_this_image(unused)
    end if
  end function

  function num_images()  result(num_images_)
    !! Result is the total number of images
    use mpi, only : MPI_Comm_size
    integer(c_int) :: num_images_,ierr
    if (.true.) then
      call MPI_Comm_size(CAF_COMM_WORLD,num_images_,ierr)
      if (ierr/=0) call error_stop
    else
      !! alternative, simpler implementation (doesn't work with gfortran)
      ! num_images_ = opencoarrays_num_images(unused_coarray,unused_scalar)
    end if
  end function

  subroutine error_stop(stop_code)
    !! Halt the execution of all images
    integer(c_int32_t), intent(in), optional :: stop_code
    integer(c_int32_t), parameter :: default_code=-1_c_int32_t
    integer(c_int32_t) :: code
    code = merge(stop_code,default_code,present(stop_code))
    call opencoarrays_error_stop(code)
  end subroutine

  subroutine sync_all(stat,errmsg,unused)
    !! Globally common segment boundary
    integer(c_int), intent(out), optional :: stat,unused
    character(c_char), intent(out), optional :: errmsg
    call opencoarrays_sync_all(stat,errmsg,unused)
  end subroutine

end module
