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

module periodic_2nd_order_module
#include "compiler_capabilities.txt"
  use kind_parameters ,only : rkind, ikind
  use math_constants, only : local_grid_size
  use field_module ,only : field
  use ForTrilinos_assertion_utility, only : assert,error_message
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
  use co_object_interface, only : co_object
#endif

  implicit none
  private
  public :: periodic_2nd_order, initial_field

#ifdef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
  type periodic_2nd_order
#else
  type, extends(co_object) :: periodic_2nd_order
#endif
    private
    real(rkind), allocatable :: global_f(:)[:]
  contains
    procedure :: construct
    procedure :: assign        => copy
    procedure :: add           => add_field
    procedure :: multiply      => multiply_field
    procedure :: x             => df_dx
    procedure :: xx            => d2f_dx2
    procedure :: runge_kutta_2nd_step => rk2_dt
    procedure, nopass :: this_image_contains
    procedure :: has_a_zero_at
    generic   :: assignment(=) => assign
    generic   :: operator(+)   => add
    generic   :: operator(*)   => multiply
    procedure :: output
   !generic :: write=>output ! Fortran 2003 derived-type output
  end type

  real(rkind) ,parameter :: pi=acos(-1._rkind)
  real(rkind) local_grid(local_grid_size)

  abstract interface
    real(rkind) pure function initial_field(x)
      import :: rkind
      real(rkind) ,intent(in) :: x
    end function
  end interface

#ifdef TAU
  interface
   pure subroutine tau_pure_start(x)
     character(len=*), intent(in):: x
   end subroutine tau_pure_start 
   pure subroutine tau_pure_stop(x)
     character(len=*), intent(in):: x
   end subroutine tau_pure_stop 
  end interface
#endif

contains

  subroutine output(this,unit,iotype,v_list,iostat,iomsg)
    class(periodic_2nd_order), intent(in) :: this
    integer, intent(in) :: unit ! Unit on which output happens (negative for internal file)
    character(*), intent(in) :: iotype ! Allowable values: ’LISTDIRECTED’,’NAMELIST’, or ’DT’
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(*), intent(inout) :: iomsg
    integer(ikind) i
    ! Requires
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    call assert(this%user_defined(),error_message("periodic_2nd_order%output recieved unitialized object."))
#endif
    do i = 1, size(this%global_f)
      write (unit=unit,iostat=iostat,fmt="(i8,2(f12.4,2x))") &
      (this_image()-1)*size(this%global_f) + i, local_grid(i),this%global_f(i)
    end do
  end subroutine

  pure function has_a_zero_at(this, expected_location) result(zero_at_expected_location)
    class(periodic_2nd_order) ,intent(in) :: this
    real(rkind) ,intent(in) :: expected_location
    real(rkind), parameter :: tolerance = 1.0E-06_rkind
    integer :: nearest_grid_point
    logical :: zero_at_expected_location
    ! Requires
#ifdef TAU
    call tau_pure_start('has_a_zero_at')
#endif
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (this%user_defined()) then
#endif
      nearest_grid_point = minloc(abs(local_grid-expected_location),dim=1)
      zero_at_expected_location = merge(.true.,.false., abs(this%global_f(nearest_grid_point)) < tolerance  )
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    end if
#endif
#ifdef TAU
    call tau_pure_stop('has_a_zero_at')
#endif
  end function
   
  pure function this_image_contains(location) result(within_bounds)
    real(rkind), intent(in) :: location
    logical within_bounds
#ifdef TAU
    call tau_pure_start('this_image_contains')
#endif
    within_bounds = merge(.true.,.false., (location>=minval(local_grid) .and. location<=maxval(local_grid)) )
#ifdef TAU
    call tau_pure_stop('this_image_contains')
#endif
  end function

  subroutine construct (this, initial, num_grid_pts)
    class(periodic_2nd_order), intent(inout) :: this
    procedure(initial_field) ,pointer, intent(in) :: initial
    integer(ikind) ,intent(in) :: num_grid_pts
    integer :: i

#ifdef TAU
    call TAU_START('constructor')
#endif
    local_grid = grid()
    ! set up the global grid points
    allocate (this%global_f(local_grid_size)[*])

    do i = 1, local_grid_size
      this%global_f(i) = initial(local_grid(i))
    end do
    ! Ensures
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    call this%mark_as_defined
#endif
#ifdef TAU
    call TAU_START('sync_constructor')
#endif
    sync all
#ifdef TAU
    call TAU_STOP('sync_constructor')
    call TAU_STOP('constructor')
#endif

  contains
    pure function grid()
      integer(ikind) :: i
      real(rkind) ,dimension(local_grid_size) :: grid
#ifdef TAU
      call tau_pure_start('grid')
#endif
      do i=1,local_grid_size
        grid(i)  = 2.*pi*(local_grid_size*(this_image()-1)+i-1) &
                   /real(num_grid_pts,rkind)  
      end do
#ifdef TAU
      call tau_pure_stop('grid')
#endif
    end function
  end subroutine

  pure real(rkind) function rk2_dt(this,nu, num_grid_pts) 
    class(periodic_2nd_order) ,intent(in) :: this
    real(rkind) ,intent(in) :: nu
    integer(ikind) ,intent(in) :: num_grid_pts
    real(rkind)             :: dx, CFL, k_max 
    ! Requires
#ifdef TAU
    call tau_pure_start('rk2_dt')
#endif
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (this%user_defined()) then
#endif
      dx=2.0*pi/num_grid_pts
      k_max=num_grid_pts/2.0_rkind
      CFL=1.0/(1.0-cos(k_max*dx))
      rk2_dt = CFL*dx**2/nu 
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    end if
#endif
#ifdef TAU
    call tau_pure_stop('rk2_dt')
#endif
  end function

  ! this is the assignment
  subroutine copy(lhs,rhs)
    class(periodic_2nd_order) ,intent(inout) :: lhs
    class(field) ,intent(in) :: rhs
#ifdef TAU
    call TAU_START('assign_field')
#endif
    ! Requires
    call assert(rhs%user_defined(),error_message("periodic_2nd_order%copy received undefind RHS."))
    ! update global field
    lhs%global_f(:) = rhs%state()
    ! Ensures
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    call lhs%mark_as_defined
#endif
#ifdef TAU
   call TAU_START('sync_assign_field')
#endif
   if (num_images()==1.or.num_images()==2) then
     sync all      
   else
#ifndef COMPILER_LACKS_MULTI_IMAGE_COARRAYS
      if (this_image()==1) then
       !sync images((/2,num_images()/))
       sync all
      elseif (this_image()==num_images()) then
       !sync images((/1,this_image()-1/))
       sync all
      else
       !sync images((/this_image()-1,this_image()+1/))
       sync all
      endif
#endif
    endif
#ifdef TAU
    call TAU_STOP('sync_assign_field')
    call TAU_STOP('assign_field')
#endif
  end subroutine

  function add_field (this, rhs)
    class(periodic_2nd_order), intent(in) :: this
    type(field), intent(in) :: rhs
    type(field) :: add_field
#ifdef TAU
    call TAU_START('add_field')
#endif
    ! Requires
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (rhs%user_defined() .and. this%user_defined()) then
#else
    if (rhs%user_defined()) then
#endif
      add_field = rhs%state()+this%global_f(:)
      ! Ensures
      call add_field%mark_as_defined
    end if
#ifdef TAU
    call TAU_STOP('add_field')
#endif
  end function

  function multiply_field (this, rhs)
    class(periodic_2nd_order), intent(in) :: this, rhs
    type(field) multiply_field
#ifdef TAU
    call TAU_START('multiply_field')
#endif
    ! Requires
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (this%user_defined() .and. rhs%user_defined()) then
#endif
      multiply_field = this%global_f(:)*rhs%global_f(:)
      ! Ensures
      call multiply_field%mark_as_defined
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    end if
#endif
#ifdef TAU
    call TAU_STOP('multiply_field')
#endif
  end function

  function df_dx(this)
    class(periodic_2nd_order), intent(in) :: this
    type(field)  df_dx
    integer(ikind) :: i,nx, me, east, west
    real(rkind) :: dx
    real(rkind) tmp_field_array(local_grid_size)
#ifdef TAU
    call TAU_START('df_dx')
#endif
    ! Requires
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (this%user_defined()) then
#endif
      nx=local_grid_size
      dx=2.*pi/(real(nx,rkind)*num_images())
  
      me = this_image()
  
      if (me == 1) then 
          west = num_images()
          east = merge(1,2,num_images()==1)
      else if (me == num_images()) then
        west = me - 1
        east = 1
      else
          west = me - 1
          east = me + 1
      end if
  
      tmp_field_array(1) = &
         0.5*(this%global_f(2)-this%global_f(nx)[west])/dx
  
      tmp_field_array(nx) = &
         0.5*(this%global_f(1)[east]-this%global_f(nx-1))/dx
  
      do i=2,nx-1
        tmp_field_array(i)=&
          0.5*(this%global_f(i+1)-this%global_f(i-1))/dx
      end do
  
      df_dx = tmp_field_array
      ! Ensures
      call df_dx%mark_as_defined
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    end if
#endif
#ifdef TAU
    call TAU_STOP('df_dx')
#endif
  end function

  pure function d2f_dx2(this)
    class(periodic_2nd_order), intent(in) :: this
    type(field)  :: d2f_dx2
    integer(ikind) :: i,nx, me, east, west
    real(rkind) :: dx
    real(rkind) :: tmp_field_array(local_grid_size)

#ifdef TAU
    call tau_pure_start('d2f_dx2')
#endif
    ! Requires
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    if (this%user_defined()) then
#endif

      nx=local_grid_size
      dx=2.*pi/(real(nx,rkind)*num_images())
  
      me = this_image()
  
      if (me == 1) then 
          west = num_images()
          east = merge(1,2,num_images()==1)
      else if (me == num_images()) then
          west = me - 1
          east = 1
      else
          west = me - 1
          east = me + 1
      end if
  
      tmp_field_array(1) = &
         (this%global_f(2)-2.0*this%global_f(1)+this%global_f(nx)[west])&
         /dx**2
  
      tmp_field_array(nx) =&
         (this%global_f(1)[east]-2.0*this%global_f(nx)+this%global_f(nx-1))&
         /dx**2
  
      do i=2,nx-1
        tmp_field_array(i)=&
          (this%global_f(i+1)-2.0*this%global_f(i)+this%global_f(i-1))&
          /dx**2
      end do
  
      d2f_dx2 = tmp_field_array
      ! Ensures
      call d2f_dx2%mark_as_defined
#ifndef LACKS_SUPPORT_FOR_PARENT_WITH_COARRAY
    end if
#endif
#ifdef TAU
  call tau_pure_stop('d2f_dx2')
#endif
  end function
end module
