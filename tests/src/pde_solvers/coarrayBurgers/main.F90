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
  use kind_parameters ,only : rkind
  use global_field_module, only : global_field, initial_field
  use initializer ,only : u_initial,zero
  use iso_fortran_env, only : output_unit

  implicit none
  type(global_field), save :: u,half_uu,u_half
  real(rkind) :: dt
  real(rkind), parameter :: half=0.5_rkind,t_final=.1_rkind,nu=1._rkind
  real(rkind), parameter :: pi=acos(-1._rkind),expected_zero_location=pi
  integer ,parameter     :: num_steps=100,grid_resolution=1024
  integer :: iostat,step
  procedure(initial_field) ,pointer :: initial
  character(len=256) :: iomsg
  logical, parameter :: performance_analysis=.true.

  ! Starting TAU 
#ifdef TAU_INTEL
  call TAU_PROFILE_SET_NODE(this_image())  !Intel coarray implementation
#endif
#ifdef TAU_CRAY
  call TAU_PROFILE_SET_NODE(this_image()-1) !Cray coarray implementation
#endif

  initial => u_initial
  call u%construct(initial,grid_resolution)
  initial => zero
  call half_uu%construct(initial,grid_resolution)
  call u_half%construct(initial,grid_resolution)

  dt = u%runge_kutta_2nd_step(nu ,grid_resolution)
  call u%set_time(time_stamp=0._rkind)
  if (performance_analysis) then
    ! Integrate over a fixed number of time steps so we do a fixed number of FLOPS
    do step=1,num_steps 
      call runge_kutta_2nd_order_time_step
    end do
  else 
    ! Integrate over the time interval [0,t_final)
    do while (u%get_time()<=t_final) ! 2nd-order Runge-Kutta:
      call runge_kutta_2nd_order_time_step
    end do
  end if
  call u%output(output_unit,'DT',[0],iostat,iomsg)
  if (this_image_contains_midpoint()) then
    if (.not. u%has_a_zero_at(expected_zero_location)) error stop "Test failed."
    print *,'Test passed.'
  end if
contains
  subroutine runge_kutta_2nd_order_time_step
    half_uu = u*u*half
    u_half = u + (u%xx()*nu - half_uu%x())*dt*half ! first substep
    half_uu = u_half*u_half*half
    u  = u + (u_half%xx()*nu - half_uu%x())*dt ! second substep
    call u%set_time(u%get_time()+dt)
  end subroutine
  function this_image_contains_midpoint() result(within_bounds)
    logical within_bounds
    within_bounds = merge(.true.,.false., (this_image()==num_images()/2+1) )
  end function
end program
