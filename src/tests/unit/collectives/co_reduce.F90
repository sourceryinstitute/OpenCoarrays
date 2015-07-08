! Copyright (c) 2012-2015, Sourcery, Inc.
! All rights reserved.
!
! Unit tests for co_min: verify parallel, collective minimum
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!     * Redistributions of source code must retain the above copyright
!       notice, this list of conditions and the following disclaimer.
!     * Redistributions in binary form must reproduce the above copyright
!       notice, this list of conditions and the following disclaimer in the
!       documentation and/or other materials provided with the distribution.
!     * Neither the name of Sourcery, Inc., nor the
!       names of any other contributors may be used to endorse or promote products
!       derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE 
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
! (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
! LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
! ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module co_all_module
  implicit none
  private
  public :: co_all
contains
  subroutine co_all(boolean)
    logical, intent(inout) :: boolean
    call co_reduce(boolean,both)
  contains
    pure function both(lhs,rhs) result(lhs_and_rhs)
      logical, intent(in) :: lhs,rhs
      logical :: lhs_and_rhs
      lhs_and_rhs = lhs .and. rhs 
    end function
  end subroutine
end module 

program main
  use iso_fortran_env, only : error_unit 
  use iso_c_binding, only : c_int,c_double
  use co_all_module, only : co_all
#ifdef USE_EXTENSIONS
  use opencoarrays
#endif
  implicit none               

#ifdef USE_EXTENSIONS
  if (this_image()==1) print *,"Using the extensions from the opencoarrays module."
#endif

  ! Verify that every image has a "true" variable with the value .true.
  verify_co_reduce: block 
    logical :: true=.true.
    sync all
    call co_all(true)
    if (.not.true) then
      write(error_unit,"(2(a,i2))") "co_min fails for integer(c_int) argument with result (",me,") on image",this_image()
      error stop
    end if
  end block verify_co_reduce

  ! Wait for all images to pass the tests
  sync all
  if (this_image()==1) print *, "Test passed."
end program
