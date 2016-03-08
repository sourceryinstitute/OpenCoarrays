! derivative.f90
!
! -- This program provides a test target for the Open Fortran Parser source
!    transformation rules defined in 3rd-party-tools/open-fortran-parser-sdf/trans
!    to support compiling coarray Fortran (CAF) programs with non-CAF compilers.
!    The desired transformed source is in transformed-derivative.f90.
!
! OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
! Copyright (c) 2016, Sourcery Inc.
! Copyright (c) 2016, Sourcery Institute
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are
! met:
!
! 1. Redistributions of source code must retain the above copyright
! notice, this list of conditions and the following disclaimer.
!
! 2. Redistributions in binary form must reproduce the above copyright
! notice, this list of conditions and the following disclaimer in the
! documentation and/or other materials provided with the distribution.
!
! 3. Neither the name of the copyright holder nor the names of its
! contributors may be used to endorse or promote products derived from
! this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
! "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
! LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
! A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
! HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
! SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
! LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
! DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
! THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
program main
  implicit none
  real, allocatable :: f(:)[:],df_dx(:)
  integer, parameter :: num_points=1024
  real, parameter :: pi=acos(-1.),dx=2.*pi/num_points,tolerance=0.0001
  integer :: my_num_points,i,my_first,my_last,left,right
  integer, allocatable :: neighbors(:)
  if (mod(num_points,num_images()) /= 0) error stop "not evenly divisible"
  my_num_points=num_points/num_images()
  my_first=(this_image()-1)*my_num_points 
  my_last = my_first + my_num_points - 1

  ! In the transformed source, this coarray allocation must be done by
  ! invoking caf_allocate to
  ! 1. Construct the array descriptor
  ! 2. Invoke caf_register, which
  !    a.) allocates memory
  !    b.) performs an implicit synchronization
  ! 3. Associate a Fortran pointer with the C-allocated target
  allocate(f(my_first:my_last)[*],df_dx(my_first:my_last))

  ! f is now a pointer, which hopefully obviates the need to transform any
  ! further non-coarray code involving f requires

  f(my_first:my_last) = sin(([(i,i=my_first,my_last)])*dx)

  ! compute neighboring image numbers
  left  = merge(this_image()-1,num_images(),this_image()/=1)
  right = merge(this_image()+1,1           ,this_image()/=num_images())
  if (left==right) then
    neighbors = [left]
  else
    neighbors = [left,right]
  end if

  ! change "sync images" this to "call sync_images(neighbors)"
  sync images(neighbors)
 
  !                             access local data / get remote data
  df_dx(my_first)             = (f(my_first+1) - f(my_last)[left])/(2.*dx)

  !                             access local data
  df_dx(my_first+1:my_last-1) = [ ((f(i+1) - f(i-1))/(2.*dx), i=my_first+1,my_last-1) ]

  !                             get remote data     / access local data 
  df_dx(my_last)              = (f(my_first)[right] - f(my_last-1))/(2.*dx)

  !  change 'error stop "..."' to 'call error_stop("...")'
  if (any(abs(df_dx-cos([(i,i=my_first,my_last)]*dx))>tolerance)) error stop "inaccurate derivative"
  
  ! Wait for every image to pass the test:
  ! change "sync all" a "call sync_all()"
  sync all
  if (this_image()==1) print *, "Test passed."
end program
