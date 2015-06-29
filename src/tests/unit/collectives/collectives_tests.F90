! Copyright (c) 2012-2015, Sourcery, Inc.
! All rights reserved.
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

! Unit tests for co_broadcast and co_sum
program main
  use iso_fortran_env, only : error_unit 
  use iso_c_binding, only : c_int,c_double
#ifdef USE_EXTENSIONS
  use opencoarrays, only : this_image,num_images,co_sum,co_broadcast
#endif
  implicit none               
  integer(c_int) :: me

  ! Store the executing image number
  me=this_image()

#ifdef USE_EXTENSIONS
  if (me==1) print *,"Using the extensions from the opencoarrays module."
#endif

  ! Verify broadcasting of integer data from image 1
  c_int_co_broadcast: block 
    integer(c_int), save :: integer_received[*]
    integer(c_int), parameter :: integer_sent=12345_c_int ! Integer test message
    if (me==1) integer_received=integer_sent
    sync all
    call co_broadcast(integer_received,source_image=1)
    if (integer_received/=integer_sent) then
      write(error_unit,*) "Incorrect co_broadcast(",integer_received,") on image",me
      error stop "Test failed." ! Halt all images
    end if
  end block c_int_co_broadcast

  ! Verify broadcasting of real data from image 1
  c_double_co_broadcast: block 
    real(c_double), save :: real_received[*]
    real(c_double), parameter :: real_sent=2.7182818459045_c_double ! Real test message
    if (me==1) real_received=real_sent
    sync all
    call co_broadcast(real_received,source_image=1)
    if (real_received/=real_sent) then
      write(error_unit,*) "Incorrect co_broadcast(",real_received,") on image",me
      error stop "Test failed." ! Halt all images
    end if
  end block c_double_co_broadcast

  ! Verify collective sum of integer data by tallying image numbers
  c_int_co_sum: block 
    integer(c_int) :: i
    integer(c_int), save :: image_number_tally[*]
    image_number_tally=me
    sync all
    call co_sum(image_number_tally)
    if (image_number_tally/=sum([(i,i=1,num_images())])) then
      write(error_unit,"(2(a,i2))") "Wrong result (",image_number_tally,") on image",image_number_tally
      error stop "Test failed." ! Halt all images
    end if
  end block c_int_co_sum

  ! Verify collective sum by calculuating pi
  c_double_co_sum: block 
    real(c_double), parameter :: four=4._c_double,one=1._c_double,half=0.5_c_double
    real(c_double), save :: pi[*],pi_local
    integer(c_int) :: i,points_per_image
    integer(c_int), parameter :: resolution=1024_c_int ! Number of points used in pi calculation
    ! Partition the calculation evenly across all images
    if (mod(resolution,num_images())/=0) error stop "number of images doesn't evenly divide into number of points"
    points_per_image=resolution/num_images()
    associate(n=>resolution,my_first=>points_per_image*(me-1)+1,my_last=>points_per_image*me)
      pi_local = sum([ (four/(one+((i-half)/n)**2),i=my_first,my_last) ])/n
    end associate
    pi=pi_local
    sync all
    ! Replace pi on each image with the sum of the pi contributions from all images
    call co_sum(pi)
    associate (pi_ref=>acos(-1._c_double),allowable_fractional_error=>0.000001_c_double)
      if (abs((pi-pi_ref)/pi_ref)>allowable_fractional_error) then
        write(error_unit,*) "Inaccurate pi (",pi,") result on image ",me
        error stop
      end if
    end associate
  end block c_double_co_sum

  if (this_image()==1) print *, "Test passed."
end program
