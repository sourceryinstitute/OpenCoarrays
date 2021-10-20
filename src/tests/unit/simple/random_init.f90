! random init test
!
! Copyright (c) 2021-2021, Sourcery, Inc.
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
!

program test_random_init
  implicit none
  integer :: me,np
  integer(kind=4), dimension(:), allocatable :: random_num, from_master
  integer(kind=4) :: seed_size
  integer :: seed_eq

  me = this_image()
  np = num_images()

  if (np .lt. 1) then
    error stop "Need at least two images."
  end if

  call random_seed(size=seed_size)
  allocate(random_num(1:seed_size))
  allocate(from_master(1:seed_size))

  call random_init(.true., .true.)

  sync all
  call random_seed(get=random_num)
  if (me .eq. 1) then
    from_master = random_num
  end if
  call co_broadcast(from_master, 1)
  if (me .eq. 1) then
    seed_eq = 0
  else
    seed_eq = any(random_num .eq. from_master)
  end if
  call co_max(seed_eq, 1)

  if (me .eq. 1 .and. seed_eq .eq. 1) then
    error stop "Test failed. (T,T)"
  end if

  call random_init(.false., .true.)

  sync all
  call random_seed(get=random_num)
  if (me .eq. 1) then
    from_master = random_num
  end if
  call co_broadcast(from_master, 1)
  if (me .eq. 1) then
    seed_eq = 0
  else
    seed_eq = any(random_num .eq. from_master)
  end if
  call co_max(seed_eq, 1)

  if (me .eq. 1 .and. seed_eq .eq. 1) then
    error stop "Test failed. (F,T)"
  end if

  sync all

  call random_init(.false., .false.)

  sync all
  call random_seed(get=random_num)
  if (me .eq. 1) then
    from_master = random_num
  end if
  call co_broadcast(from_master, 1)
  seed_eq = all(random_num .eq. from_master)
  call co_min(seed_eq, 1)

print *,"me=", me, ", rand_num=", random_num, ", from_master=", from_master, ", seed_eq=", seed_eq
  if (me .eq. 1 .and. seed_eq .eq. 0) then
    error stop "Test failed. (F,F)"
  end if

  sync all

  call random_init(.true., .false.)

  sync all
  call random_seed(get=random_num)
  if (me .eq. 1) then
    from_master = random_num
  end if
  call co_broadcast(from_master, 1)
  seed_eq = all(random_num .eq. from_master)
  call co_min(seed_eq, 1)

  if (me .eq. 1 .and. seed_eq .eq. 0) then
    error stop "Test failed. (T,F)"
  end if

  sync all

  if (me .eq. 1) print *,"Test passed."

end program test_random_init

