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
  use iso_fortran_env, only : atomic_int_kind
  use iso_c_binding, only : c_int,c_char,c_ptr,c_loc,c_long
  implicit none

  private
  public :: co_sum
  public :: event_type
  public :: event_post
  
  type event_type
    private
    integer(atomic_int_kind), allocatable :: atom[:]
  end type

  interface co_sum
    procedure co_sum_integer
  end interface

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
  subroutine co_sum_integer(a,result_image,stat,errmsg)
    integer(c_int), intent(inout), volatile, target :: a(*)
    integer(c_int), intent(in), optional :: result_image
    integer(c_int), intent(out), optional:: stat 
    character(kind=c_char,len=*), intent(out), optional :: errmsg

    !gfc_descriptor struct array5_integer(kind=4) parm.7;
    type(gfc_descriptor_t) :: a_descriptor

    error stop "Build a_descriptor"
      
     ! C example:
     !parm_7%dtype = 269;
     !parm_7%dim(0).lbound = 1;
     !parm_7%dim(0).ubound = 1;
     !parm_7%dim(0).stride = 1;
     !parm_7%dim(1).lbound = 1;
     !parm_7%dim(1).ubound = 1;
     !parm_7%dim(1).stride = 1;
     !...
     !parm_7%data = (void *) &a[0];
     !parm_7%offset = -9;

      if (.not. present(stat)) stat=0
      if (.not.present(errmsg)) errmsg=""
      block 
        ! Local replacement for the corresponding intent(in) dummy argument:
        integer(c_int) :: result_image_
        integer(c_int), parameter :: default_result_image=0
        result_image_ = merge(result_image,default_result_image,present(result_image)) 
        call caf_co_sum_integer(c_loc(a),result_image, stat, errmsg, len(errmsg)) 
      end block
      
  end subroutine

  ! Proposed Fortran 2015 event_post procedure
  subroutine event_post(this)
    class(event_type), intent(inout) ::  this
    if (.not.allocated(this%atom)) this%atom=0
    call atomic_define ( this%atom, this%atom + 1_atomic_int_kind )
  end subroutine

end module

