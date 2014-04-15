! Copyright (c) 2011, Damian Rouson, Jim Xia, and Xiaofeng Xu.
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!     * Redistributions of source code must retain the above copyright
!       notice, this list of conditions and the following disclaimer.
!     * Redistributions in binary form must reproduce the above copyright
!       notice, this list of conditions and the following disclaimer in the
!       documentation and/or other materials provided with the distribution.
!     * Neither the names of Damian Rouson, Jim Xia, and Xiaofeng Xu nor the
!       names of any other contributors may be used to endorse or promote products
!       derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL DAMIAN ROUSON, JIM XIA, and XIAOFENG XU BE LIABLE 
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
! (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
! LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
! ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module initializer
  use kind_parameters ,only : rkind
  implicit none
contains
  real(rkind) pure function u_initial(x)
    real(rkind) ,intent(in) :: x
    u_initial = 10._rkind*sin(x)
  end function
  real(rkind) pure function zero(x)
    real(rkind) ,intent(in) :: x
    zero = 0.
  end function
end module 

program main
  use iso_fortran_env, only : output_unit
  use kind_parameters ,only : rkind
  use ForTrilinos_assertion_utility 
  use math_constants, only: grid_resolution, local_grid_size
  use periodic_2nd_order_module, only : periodic_2nd_order, initial_field
  use initializer ,only : u_initial,zero
  implicit none
  type(periodic_2nd_order), save :: u,half_uu,u_half
  real(rkind) :: dt,half=0.5,t=0.,t_final=0.1,nu=1.
  integer ,parameter :: base_output_unit=output_unit+10
  integer step,iostat
  character(:), allocatable :: iotype ! Allowable values: ’LISTDIRECTED’,’NAMELIST’, or ’DT’
  character(:), allocatable :: iomsg
  integer, allocatable :: v_list(:)
  procedure(initial_field) ,pointer :: initial

  ! Test parameters
  real(rkind), parameter :: pi=acos(-1._rkind),expected_zero_location=pi
  real(rkind) :: t1, t2, t3, t4

#ifdef TAU
  ! Starting TAU 
  !call TAU_PROFILE_SET_NODE(this_image())  !ACISS
  call TAU_PROFILE_SET_NODE(this_image()-1) !Hopper 
#endif

! Checks that local grid = grid / num images
  call assert(local_grid_size*num_images()==grid_resolution,error_message("main: Grid size divides unevenly across images."))
  sync all; call CPU_TIME(t1); sync all

  initial => u_initial
  call u%construct(initial,grid_resolution)
  initial => zero
  call half_uu%construct(initial,grid_resolution)
  call u_half%construct(initial,grid_resolution)
  
  sync all; call CPU_TIME(t2); sync all
  !do while (t<=t_final) ! 2nd-order Runge-Kutta:
  do step=1,100
    dt = u%runge_kutta_2nd_step(nu ,grid_resolution)
    half_uu = u*u*half
    u_half = u + (u%xx()*nu - half_uu%x())*dt*half ! first substep
    half_uu = u_half*u_half*half
    u  = u + (u_half%xx()*nu - half_uu%x())*dt ! second substep
    t = t + dt
  end do
  sync all; call CPU_TIME(t3); sync all
  !call u%output(base_output_unit+this_image(),iotype,v_list,iostat,iomsg)
  sync all; call CPU_TIME(t4); sync all
  if (u%this_image_contains(expected_zero_location)) then
    if (.not. u%has_a_zero_at(expected_zero_location)) error stop "Test failed."
    print *,'Test passed.'
  end if
  if (this_image()==1) print *, 'START UP=', t2-t1, 'LOOP=', t3-t2, 'total=', t3-t1, 'Output=', t4-t3
end program
