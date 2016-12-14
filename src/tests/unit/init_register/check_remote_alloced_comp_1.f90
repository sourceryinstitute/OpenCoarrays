! Unit test for register procedure and remote allocated test.
!
! Test that scalar allocatable component in a derived typed coarray is
! registered correctly, delayed allocatable and deregisterable. The checks
! whether the component is allocated are done on the remote images.
!
! Copyright (c) 2012-2016, Sourcery, Inc.
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

program check_remote_alloced_comp_1
  implicit none

  type dt
    integer, allocatable :: i
  end type dt

  integer :: np = -2, me, remote, test
  type(dt), allocatable :: obj[:]

  np = num_images()
  me = this_image()

  ! Make sure that at least two images are available. This might not be
  ! necessary, but simplyfies writing the test.
  if (np < 2) error stop "Test failed. Need more than one image."

  ! Allocate only the container. obj%i must not be allocated hereafter.
  allocate(obj[*])
  if (.not. allocated(obj)) error stop "Test failed. 'obj' not allocated."
  if (allocated(obj%i)) error stop "Test failed. 'obj%i' is allocated."

  ! The print statements are in for debugging purposes only.

  ! All images have allocated the container. Now iterate over the images and
  ! when this image's number is equal to remote, allocate the component,
  ! else check the allocation status of all other remote components.
  do remote = 1, np
    print *, me, "/", np, ": remote=", remote
    if (remote == me) then
      print *, me, "/", np, ": allocating..."
      allocate(obj%i, source = me)
      print *, me, "/", np, ": allocated"
      ! Now both objects have to be allocated and obj%i set to this_image()
      if (.not. allocated(obj)) error stop "Test failed. 'obj' not allocated."
      if (.not. allocated(obj%i)) error stop "Test failed. 'obj%i' not allocated."
      if (obj%i /= me) error stop "Test failed. obj%i /= this_image()."
      sync all
    else
      sync all
      ! Iterate using test over the images to test:
      ! when test less or equal than remote, check that the remote image's
      !            component given by test is allocated,
      ! else check that the remote component is not yet allocated.
      do test = 1, np
        print *, me, "/", np, ": Checking", test, " for allocation status."
        if (test > remote) then
          if (allocated(obj[test]%i)) error stop "Test failed. 'obj%i' on remote image already allocated."
        else
          if (.not. allocated(obj[test]%i)) error stop "Test failed. 'obj%i' on remote image not allocated."
        end if
      end do
    end if
  enddo
end program

