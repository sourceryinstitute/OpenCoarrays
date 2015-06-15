! Fortran 2015 feature support for Fortran 2008 compilers
! 
! Copyright (c) 2015, Sourcery, Inc.
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
#ifdef COMPILER_SUPPORTS_ATOMICS
  use iso_fortran_env, only : atomic_int_kind
#endif
  use iso_c_binding, only : c_int,c_char,c_ptr,c_loc,c_long,c_int32_t
  implicit none

  private
  public :: co_sum
  public :: this_image
  public :: num_images
  public :: error_stop
  public :: sync_all
#ifdef COMPILER_SUPPORTS_ATOMICS
  public :: event_type
  public :: event_post
  
  type event_type
    private
    integer(atomic_int_kind), allocatable :: atom[:]
  end type
#endif

  ! Generic interface to co_sum with implementations for various types, kinds, and ranks
  interface co_sum
     module procedure co_sum_integer_scalar
  end interface 

  ! Bindings for OpenCoarrays C procedures
  interface 

    ! C function prototype from ../libcaf.h:
    ! void
    ! PREFIX (co_sum) (gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
    !                  int errmsg_len)
    subroutine caf_co_sum_integer(a,result_image, stat, errmsg, errmsg_len) bind(C,name="_gfortran_caf_co_sum")
      import :: c_int,c_char,c_ptr
      type(c_ptr), intent(in), value :: a
      integer(c_int), intent(in),  value :: result_image
      integer(c_int), intent(out), optional :: stat
      character(len=1,kind=c_char), intent(out), optional :: errmsg
      integer(c_int), intent(in), value :: errmsg_len
    end subroutine

    ! C function prototype from ../libcaf.h:
    ! int PREFIX (this_image) (int);
    function caf_this_image(coarray) bind(C,name="_gfortran_caf_this_image") result(image_num)
      import :: c_int
      integer(c_int), value, intent(in) :: coarray
      integer(c_int)  :: image_num
    end function

    ! C function prototype from ../libcaf.h:
    ! int PREFIX (num_images) (int, int);
    function caf_num_images(coarray,dim_) bind(C,name="_gfortran_caf_num_images") result(num_images_)
      import :: c_int
      integer(c_int), value, intent(in) :: coarray,dim_
      integer(c_int) :: num_images_
    end function

    ! C function prototype from ../libcaf.h
    ! void PREFIX (error_stop) (int32_t) __attribute__ ((noreturn));
    subroutine caf_error_stop(stop_code) bind(C,name="_gfortran_caf_error_stop") 
      import :: c_int32_t
      integer(c_int32_t), value, intent(in) :: stop_code
    end subroutine 

    ! C function prototype from ../libcaf.h
    ! void PREFIX (sync_all) (int *, char *, int);
    subroutine caf_sync_all(stat,errmsg,unused) bind(C,name="_gfortran_caf_sync_all") 
      import :: c_int,c_char
      integer(c_int), intent(out) :: stat,unused
      character(c_char), intent(out) :: errmsg
    end subroutine 

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

  integer, parameter :: ptrdiff_t=c_long

  ! Fortran derived type interoperable with like-named C type:
  type, bind(C) :: descriptor_dimension
     integer(ptrdiff_t) :: stride 
     integer(ptrdiff_t) :: lower_bound 
     integer(ptrdiff_t) :: ubound_ 
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
    integer(ptrdiff_t) :: offset
    integer(ptrdiff_t) :: dtype
    type(descriptor_dimension) :: dim_(max_dimensions)
  end type
  ! --------------------

contains

  ! Proposed Fortran 2015 parallel collective sum reduction for integers of interoperable kind c_int
  subroutine co_sum_integer_scalar(a,result_image,stat,errmsg)
    implicit none
    integer(c_int), intent(inout), volatile, target, contiguous :: a(..)
    integer(c_int), intent(in), optional :: result_image
    integer(c_int), intent(out), optional:: stat
    character(kind=1,len=*), intent(out), optional :: errmsg
    ! Local variables
    integer :: my_rank
    integer, parameter :: scalar_dtype=264,scalar_offset=-1
    type(gfc_descriptor_t), target :: a_descriptor

    my_rank=rank(a)
    a_descriptor%dtype = scalar_dtype + my_rank
    a_descriptor%offset = scalar_offset + my_rank
    a_descriptor%base_addr = c_loc(a) ! data
    block
      integer(c_int) :: result_image_,i
      integer(c_int), parameter :: default_result_image=0,unit_stride=1
      do concurrent(i=1:rank(a))
        a_descriptor%dim_(i)%stride  = unit_stride
        a_descriptor%dim_(i)%lower_bound = lbound(a,i)
        a_descriptor%dim_(i)%ubound_ = ubound(a,i)
      end do
      ! Local replacement for the corresponding intent(in) dummy argument:
      result_image_ = merge(result_image,default_result_image,present(result_image)) 
      call caf_co_sum_integer(c_loc(a_descriptor),result_image_, stat, errmsg, len(errmsg)) 
    end block
    
  end subroutine

  ! Return the image number (MPI rank + 1)
  function this_image()  result(image_num)
    integer(c_int) :: image_num,unused
    image_num = caf_this_image(unused)
  end function

  ! Return the total number of images
  function num_images()  result(num_images_)
    integer(c_int) :: num_images_,unused_coarray,unused_scalar
    num_images_ = caf_num_images(unused_coarray,unused_scalar)
  end function

  ! Halt the execution of all images
  subroutine error_stop(stop_code)
    integer(c_int32_t), intent(in), optional :: stop_code
    integer(c_int32_t), parameter :: default_code=-1_c_int32_t
    integer(c_int32_t) :: code
    code = merge(stop_code,default_code,present(stop_code))
    call caf_error_stop(code)
  end subroutine 

  ! Impose a global execution barrier
  subroutine sync_all(stat,errmsg,unused)
    integer(c_int), intent(out), optional :: stat,unused
    character(c_char), intent(out), optional :: errmsg
    call caf_sync_all(stat,errmsg,unused)
  end subroutine 

#ifdef COMPILER_SUPPORTS_ATOMICS
   ! Proposed Fortran 2015 event_post procedure
   subroutine event_post(this)
     class(event_type), intent(inout) ::  this
     if (.not.allocated(this%atom)) this%atom=0
     call atomic_define ( this%atom, this%atom + 1_atomic_int_kind )
   end subroutine
#endif 

end module
