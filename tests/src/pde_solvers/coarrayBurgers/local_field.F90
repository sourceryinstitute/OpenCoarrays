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

module local_field_module
  use ForTrilinos_assertion_utility, only : assert,error_message
  use object_interface, only : object
  use kind_parameters ,only : rkind, ikind
  implicit none
  private
  public :: local_field
  type, extends(object) :: local_field 
    real(rkind), allocatable :: f(:)
  contains
    procedure :: subtract       => difference      
    procedure :: multiply_real  => multiple
    procedure :: assign_local_field_f
    procedure :: state          => local_field_values
    procedure :: output 
    generic   :: operator(-)    => subtract
    generic   :: operator(*)    => multiply_real
    generic   :: assignment(=)  => assign_local_field_f
  end type

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
    class(local_field), intent(in) :: this
    integer, intent(in) :: unit ! Unit on which output happens (negative for internal file
    character(*), intent(in) :: iotype
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(*), intent(inout) :: iomsg
    integer(ikind) i
    ! Requires
#ifdef TAU
    call TAU_START('local_field%output')
#endif
    call assert(this%user_defined(),error_message("local_field%output received uninitialized object"))
    do i=1,size(this%f)
      write(unit,iostat=iostat) (this_image()-1)*size(this%f) + i, this%f(i)
    end do
#ifdef TAU
    call TAU_STOP('local_field%output')
#endif
  end subroutine

  function local_field_values (this)
    class(local_field), intent(in) :: this
    real(rkind), allocatable :: local_field_values(:)
#ifdef TAU
    call TAU_START('local_field%local_field_values')
#endif
    local_field_values = this%f
#ifdef TAU
    call TAU_STOP('local_field%local_field_values')
#endif
  end function

 subroutine assign_local_field_f (lhs, rhs)
    class(local_field), intent(inout) :: lhs
    real(rkind), intent(in) :: rhs(:)
#ifdef TAU
    call TAU_START('local_field%assign_local_field_f')
#endif
    lhs%f = rhs
    ! Ensures
    call lhs%mark_as_defined
#ifdef TAU
    call TAU_STOP('local_field%assign_local_field_f')
#endif
  end subroutine

  pure function difference(lhs,rhs)
    class(local_field) ,intent(in) :: lhs
    class(local_field) ,intent(in)  :: rhs
    type(local_field) :: difference
#ifdef TAU
    call tau_pure_start('local_field%difference')
#endif
    ! Requires
    if (lhs%user_defined() .and. rhs%user_defined()) then
      difference%f = lhs%f - rhs%f
      ! Ensures
      call difference%mark_as_defined
    end if
#ifdef TAU
    call tau_pure_stop('local_field%difference')
#endif
  end function

  pure function multiple(lhs,rhs)
    class(local_field) ,intent(in) :: lhs
    real(rkind) ,intent(in)  :: rhs
    type(local_field) :: multiple
#ifdef TAU
    call tau_pure_start('local_field%multiple')
#endif
    ! Requires
    if (lhs%user_defined()) then
      multiple%f = lhs%f * rhs
      ! Ensures
      call multiple%mark_as_defined
    end if
#ifdef TAU
    call tau_pure_stop('local_field%multiple')
#endif
  end function
end module
