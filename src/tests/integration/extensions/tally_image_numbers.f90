! Parallel collective sum test of support for non-OpenCoarrays-aware compilers
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

program main
  use iso_fortran_env, only : error_unit
  use iso_c_binding, only : c_int ,c_int32_t
  ! Import OpenCoarrays (use only with compilers that are not OpenCoarrays-aware)
  use opencoarrays
  implicit none    
  integer(c_int) :: me,image
  me=this_image()
  ! Parallel collective sum of image numbers:
  call co_sum(me)
  ! Verify co_sum by comparing to the expected result:
  if (me/=sum([(image,image=1,num_images())])) then
    ! Report incorrect result:
    write(error_unit,"(2(a,i2))") "Incorrect sum ",me," on image ",this_image()
    ! Halt all images:
    error stop
  end if
  ! Wait for every image to pass the test:
  sync all
  ! Image 1 reports success:
  if (this_image()==1) print *, "Test passed."
end program
