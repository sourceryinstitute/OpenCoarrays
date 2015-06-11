! Fortran 2015 feature support for Fortarn 2008 compilers
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
  use iso_c_binding, only : c_int,c_char,c_ptr,c_loc,c_long
  implicit none

  private
  public :: co_sum
#ifdef COMPILER_SUPPORTS_ATOMICS
  public :: event_type
  public :: event_post
  
  type event_type
    private
    integer(atomic_int_kind), allocatable :: atom[:]
  end type
#endif

  interface co_sum
     module procedure co_sum_integer_scalar
  end interface co_sum

  ! Bindings for OpenCoarrays C procedures
  interface 
    ! From ../libcaf.h 
    !
    ! C function prototype:
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
  end interface

  ! From ../libcaf-gfortran-descriptor.h 
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

  ! From ../libcaf-gfortran-descriptor.h 
  !typedef struct descriptor_dimension
  !{
  !  ptrdiff_t _stride;
  !  ptrdiff_t lower_bound;
  !  ptrdiff_t _ubound;
  !}
  !descriptor_dimension;

  integer, parameter :: ptrdiff_t=c_long

  type, bind(C) :: descriptor_dimension
     integer(ptrdiff_t) :: stride 
     integer(ptrdiff_t) :: lower_bound 
     integer(ptrdiff_t) :: ubound_ 
  end type

  ! From ../libcaf-gfortran-descriptor.h 
  !typedef struct gfc_descriptor_t {
  !  void *base_addr;
  !  size_t offset;
  !  ptrdiff_t dtype;
  !  descriptor_dimension dim[];
  !} gfc_descriptor_t;

  integer, parameter :: max_dimensions=15

  type, bind(C) :: gfc_descriptor_t  
    type(c_ptr) :: base_addr
    integer(ptrdiff_t) :: offset
    integer(ptrdiff_t) :: dtype
    type(descriptor_dimension) :: dim_(max_dimensions)
  end type
  ! --------------------

contains


  ! Proposed Fortran 2015 co_sum parallel collective sum reduction
  subroutine co_sum_integer_scalar(a,result_image,stat,errmsg)
    ! use iso_c_binding
    implicit none
    ! integer(c_int), intent(inout), volatile, target :: a(*)
    integer(c_int), intent(inout), volatile, target :: a
    integer(c_int), intent(in), optional :: result_image
    integer(c_int), intent(out), optional:: stat
    character(kind=1,len=*), intent(out), optional :: errmsg

    !gfc_descriptor struct array5_integer(kind=4) parm.7;
    type(gfc_descriptor_t),target :: a_descriptor

    integer :: i, l_stat = 0
    character(len=255) :: l_errmsg
    integer,allocatable :: upper_limits,lower_limits
    
    write(*,*) 'Inside extension opencoarrays'

    a_descriptor%dtype = 264
    a_descriptor%offset = -1
    a_descriptor%base_addr = c_loc(a)
    block
      integer(c_int) :: result_image_
      integer(c_int), parameter :: default_result_image=0
      ! Local replacement for the corresponding intent(in) dummy argument:
      result_image_ = merge(result_image,default_result_image,present(result_image)) 
      call caf_co_sum_integer(c_loc(a_descriptor),result_image_, l_stat, l_errmsg, len(l_errmsg)) 
    end block
    
  end subroutine

#ifdef COMPILER_SUPPORTS_ATOMICS
    Proposed Fortran 2015 event_post procedure
    subroutine event_post(this)
      class(event_type), intent(inout) ::  this
      if (.not.allocated(this%atom)) this%atom=0
      call atomic_define ( this%atom, this%atom + 1_atomic_int_kind )
    end subroutine
#endif 

end module

