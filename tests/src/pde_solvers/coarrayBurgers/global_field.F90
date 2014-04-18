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

module global_field_module
#include "compiler_capabilities.txt"
  use ForTrilinos_assertion_utility, only : assert,error_message
  use co_object_interface, only : co_object
  use kind_parameters ,only : rkind, ikind
  use local_field_module ,only : local_field

  implicit none
  private
  public :: global_field, initial_field
  type, extends(co_object) :: global_field
    private
    real(rkind), allocatable :: global_f(:)[:]
  contains
    procedure :: construct
    procedure :: assign        => copy
    procedure :: add           => add_local_field
    procedure :: multiply      => multiply_local_field
    procedure :: x             
    procedure :: xx            
    procedure :: runge_kutta_2nd_step => rk2_dt
    procedure, nopass :: set_time
    procedure, nopass :: get_time
    procedure :: output
   !generic   :: write=>output
    generic   :: assignment(=) => assign
    generic   :: operator(+)   => add
    generic   :: operator(*)   => multiply
    procedure :: has_a_zero_at
  end type

  real(rkind) ,parameter :: pi=acos(-1._rkind)
  real, save, allocatable :: local_grid(:)
  real, save :: time=0.

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

  subroutine set_time(time_stamp)
    real(rkind), intent(in) :: time_stamp
#ifdef TAU
    call TAU_START('global_field%set_time')
#endif
    time = time_stamp
#ifdef TAU
    call TAU_STOP('global_field%set_time')
#endif
  end subroutine

  pure function get_time() result(t)
    real(rkind) :: t
#ifdef TAU
    call tau_pure_start('global_field%get_time')
#endif
    t = time
#ifdef TAU
    call tau_pure_stop('global_field%get_time')
#endif
  end function

  subroutine output(this,unit,iotype,v_list,iostat,iomsg)
    class(global_field), intent(in) :: this
    integer, intent(in) :: unit ! Unit on which output happens (negative for internal file)
    character(*), intent(in) :: iotype ! Allowable values: ’LISTDIRECTED’,’NAMELIST’, or ’DT’
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(*), intent(inout) :: iomsg
    integer(ikind) i
#ifdef TAU
    call TAU_START('global_field%output')
#endif
    ! Requires
    call assert(this%user_defined(),error_message("global_field%output recieved unitialized object."))
    do i = 1, size(this%global_f)
      write (unit=unit,iostat=iostat,fmt="(i8,3(f12.4,2x))") &
      (this_image()-1)*size(this%global_f) + i, local_grid(i),time,this%global_f(i)
    end do
#ifdef TAU
    call TAU_STOP('global_field%output')
#endif
  end subroutine

  ! This procedure was not in the textbook:
  subroutine synchronize()
#ifdef TAU
    call TAU_START('global_field%synchronize')
#endif
    if (num_images()>1) then
      associate(me=>this_image())
        if (me==1) then
          sync images(me+1)
        else if (me==num_images()) then
          sync images(me-1)
        else 
          sync images([me-1,me+1])
        end if
      end associate
    end if
#ifdef TAU
    call TAU_STOP('global_field%synchronize')
#endif
  end subroutine

  subroutine construct (this, initial, num_grid_pts)
    class(global_field), intent(inout) :: this
    procedure(initial_field) ,pointer :: initial
    integer(ikind) ,intent(in) :: num_grid_pts
    integer :: i, local_grid_size
#ifdef TAU
    call TAU_START('global_field%construct')
#endif
    ! Requires
    call assert(mod(num_grid_pts,num_images())==0,error_message("Number of grid points must be divisible by number of images"))
    ! The above check was not in the textbook.

    local_grid_size = num_grid_pts / num_images()

    ! set up the global grid points
    allocate (this%global_f(local_grid_size)[*])

    local_grid = grid() ! This line was not in the textbook 
    this%global_f(:) = local_grid ! The textbook version directly assigns grid() to this%global_f(:)

#ifdef COMPILER_LACKS_DO_CONCURRENT
    do i = 1,local_grid_size
#else
    do concurrent(i = 1:local_grid_size)
#endif
      this%global_f(i) = initial(this%global_f(i))
    end do

    ! In the textbook, this was a "sync all". This call invokes "sync images" to improve performance:
    call synchronize()
    
    ! Ensures
    call this%mark_as_defined
#ifdef TAU
    call TAU_STOP('global_field%construct')
#endif

  contains
    pure function grid()
      integer(ikind) :: i
      real(rkind) ,dimension(:) ,allocatable :: grid
#ifdef TAU
    call tau_pure_start('global_field%grid')
#endif
      allocate(grid(local_grid_size))
#ifdef COMPILER_LACKS_DO_CONCURRENT
      do i=1,local_grid_size
#else
      do concurrent(i=1:local_grid_size)
#endif
        grid(i)  = 2.*pi*(local_grid_size*(this_image()-1)+i-1) &
                   /real(num_grid_pts,rkind)  
      end do
#ifdef TAU
    call tau_pure_stop('global_field%grid')
#endif
    end function
  end subroutine

  real(rkind) function rk2_dt(this,nu, num_grid_pts) 
    class(global_field) ,intent(in) :: this
    real(rkind) ,intent(in) :: nu
    integer(ikind) ,intent(in) :: num_grid_pts
    real(rkind)             :: dx, CFL, k_max 
#ifdef TAU
    call TAU_START('global_field%rk2_dt')
#endif
    dx=2.0*pi/num_grid_pts
    k_max=num_grid_pts/2.0_rkind
    CFL=1.0/(1.0-cos(k_max*dx))
    rk2_dt = CFL*dx**2/nu 
#ifdef TAU
    call TAU_STOP('global_field%rk2_dt')
#endif
  end function

  ! this is the assignment
  subroutine copy(lhs,rhs)
    class(global_field) ,intent(inout) :: lhs
    class(local_field) ,intent(in) :: rhs
#ifdef TAU
    call TAU_START('global_field%copy')
#endif
    lhs%global_f(:) = rhs%state()

    ! In the textbook, this was a "sync all". This call invokes "sync images" to improve performance:
    call synchronize()
#ifdef TAU
    call TAU_STOP('global_field%copy')
#endif
  end subroutine

  function add_local_field (lhs,rhs)
    class(global_field), intent(in) :: lhs 
    class(local_field), intent(in) :: rhs
    type(local_field) :: add_local_field
#ifdef TAU
    call TAU_START('global_field%add_local_field')
#endif
    if (rhs%user_defined()) then
      add_local_field = rhs%state()+lhs%global_f(:)
      ! Ensures
      call add_local_field%mark_as_defined
    end if
#ifdef TAU
    call TAU_STOP('global_field%add_local_field')
#endif
  end function

  function multiply_local_field (lhs,rhs)
    class(global_field), intent(in) :: lhs, rhs
    type(local_field) :: multiply_local_field
#ifdef TAU
    call TAU_START('global_field%multiply_local_field')
#endif
    if (rhs%user_defined()) then
      multiply_local_field = lhs%global_f(:)*rhs%global_f(:)
      ! Ensures
      call multiply_local_field%mark_as_defined
    end if
#ifdef TAU
    call TAU_STOP('global_field%multiply_local_field')
#endif
  end function

  function west_of(me) result(which_way)
    integer(ikind), intent(in) :: me
    integer(ikind) :: which_way
#ifdef TAU
    call TAU_START('global_field%west_of')
#endif
    which_way = merge(num_images(),me-1,me==1)
#ifdef TAU
    call TAU_STOP('global_field%west_of')
#endif
  end function

  function east_of(me) result(which_way)
    integer(ikind), intent(in) :: me
    integer(ikind) :: which_way
#ifdef TAU
    call TAU_START('global_field%east_of')
#endif
    which_way = merge(1,me+1,me==num_images())
#ifdef TAU
    call TAU_STOP('global_field%east_of')
#endif
  end function

  function x(this) result(df_dx)
    class(global_field), intent(in) :: this
    type(local_field) :: df_dx
    integer(ikind) :: i,nx 
    real(rkind) :: dx
    real(rkind), allocatable :: tmp_local_field_array(:)
   
#ifdef TAU
    call TAU_START('global_field%x')
#endif
    ! Requires
    if (this%user_defined()) then 

      nx=size(this%global_f)
      dx=2.*pi/(real(nx,rkind)*num_images())
  
      allocate(tmp_local_field_array(nx))
  
      tmp_local_field_array(1) = &
         0.5*(this%global_f(2)-this%global_f(nx)[west_of(this_image())])/dx
 
      tmp_local_field_array(nx) = &
         0.5*(this%global_f(1)[east_of(this_image())]-this%global_f(nx-1))/dx

#ifdef COMPILER_LACKS_DO_CONCURRENT
      do i=2,nx-1
#else
      do concurrent(i=2:nx-1)
#endif
        tmp_local_field_array(i)=&
          0.5*(this%global_f(i+1)-this%global_f(i-1))/dx
      end do

      df_dx = tmp_local_field_array
      ! Ensures
      call df_dx%mark_as_defined
    end if
#ifdef TAU
    call TAU_STOP('global_field%x')
#endif
  end function

  function xx(this) result(d2f_dx2)
    class(global_field), intent(in) :: this
    type(local_field) :: d2f_dx2
    integer(ikind) :: i,nx
    real(rkind) :: dx
    real(rkind), allocatable :: tmp_local_field_array(:)

#ifdef TAU
    call TAU_START('global_field%xx')
#endif
    ! Requires
    if (this%user_defined()) then 
      nx=size(this%global_f)
      dx=2.*pi/(real(nx,rkind)*num_images())
  
      allocate(tmp_local_field_array(nx))
  
      tmp_local_field_array(1) = &
         (this%global_f(2)-2.0*this%global_f(1)+this%global_f(nx)[west_of(this_image())])&
         /dx**2
  
      tmp_local_field_array(nx) =&
         (this%global_f(1)[east_of(this_image())]-2.0*this%global_f(nx)+this%global_f(nx-1))&
         /dx**2
  
#ifdef COMPILER_LACKS_DO_CONCURRENT
      do i=2,nx-1
#else
      do concurrent(i=2:nx-1)
#endif
        tmp_local_field_array(i)=&
          (this%global_f(i+1)-2.0*this%global_f(i)+this%global_f(i-1))&
          /dx**2
      end do
  
       d2f_dx2 = tmp_local_field_array
      ! Ensures
      call d2f_dx2%mark_as_defined
    end if
#ifdef TAU
    call TAU_STOP('global_field%xx')
#endif
  end function

  ! The following procedures were not included in the initial publication of the textbook:

  pure function has_a_zero_at(this, expected_location) result(zero_at_expected_location)
    class(global_field) ,intent(in) :: this
    real(rkind) ,intent(in) :: expected_location
    real(rkind), parameter :: tolerance = 1.0E-06_rkind
    integer :: nearest_grid_point
    logical :: zero_at_expected_location
#ifdef TAU
    call tau_pure_start('global_field%has_a_zero_at')
#endif
    nearest_grid_point = minloc(abs(local_grid-expected_location),dim=1)
    zero_at_expected_location = merge(.true.,.false., abs(this%global_f(nearest_grid_point)) < tolerance  )
#ifdef TAU
    call tau_pure_stop('global_field%has_a_zero_at')
#endif
  end function

end module
