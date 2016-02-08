! transformed-derivative.f90
!
! -- This program demonstrates the derivative.f90 source manually transformed
!    analogously to the expected automatic transformation by the rules
!    defined in 3rd-party-tools/open-fortran-parser-sdf to support compiling
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
  use opencoarrays
  implicit none
  real, allocatable :: df_dx(:)
  type(coarray) :: f_coarray
  integer, parameter :: num_points=1024
  real, parameter :: pi=acos(-1.),dx=2.*pi/num_points,tolerance=0.0001
  integer :: my_num_points,i,my_first,my_last,left,right
  integer, allocatable :: neighbors(:)
  call caf_init()
  if (mod(num_points,num_images()) /= 0) error stop "not evenly divisible"
  my_num_points=num_points/num_images()
  my_first=(this_image()-1)*my_num_points + 1
  my_last = my_first + my_num_points - 1 
  allocate(df_dx(my_num_points)) 
  call f_coarray%caf_register(my_num_points) ! Allocate & call c_f_pointer(f_coarray%data,f_coarray%f_ptr,shape=[my_num_points])
  f_coarray%f_ptr(1:my_num_points) = sin( ([(i,i=my_first,my_last)]-1)*dx)
  left  = merge(this_image()-1,num_images(),this_image()/=1)
  right = merge(this_image()+1,1           ,this_image()/=num_images())
  neighbors = merge([left],[left,right],left==right)
  call sync_images(neighbors)
  df_dx(1)                =  ( f_coarray%f_ptr(2)         - f_coarray%caf_get(left,my_num_points))/(2.*dx) 
  df_dx(2:my_num_points-1)= [((f_coarray%f_ptr(i+1)       - f_coarray%f_ptr(i-1)                 )/(2.*dx),i=2,my_num_points-1)]
  df_dx(my_num_points)    = (  f_coarray%caf_get(right,1) - f_coarray%f_ptr(my_num_points-1)     )/(2.*dx) 
  if (any(abs(df_dx-cos([(i,i=my_first-1,my_last-1)]*dx))>tolerance)) call error_stop("inaccurate derivative")
  call sync_all()
  if (this_image()==1) print *, "Test passed."
  call f_coarray%caf_deregister()
  call caf_finalize()
end program
