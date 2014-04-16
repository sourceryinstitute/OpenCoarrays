module burgers_module
  use ForTrilinos_assertion_utility, only : assert,error_message
  use kind_parameters, only : rkind,ikind
  use object_interface, only : object

  implicit none

  private 
  public :: burgers

  type, extends(object) :: burgers
    private
    real(rkind) :: diffusion=1._rkind
  contains
    procedure :: solve
    procedure :: output 
    procedure, private :: construct_problem
    generic :: burgers => construct_problem
  end type
contains
  ! Set the one problem parameter: the diffusion coefficient
  subroutine construct_problem(this,diffusion_coefficient)
    class(burgers), intent(out) :: this
    real(rkind), intent(in) :: diffusion_coefficient
    this%diffusion = diffusion_coefficient
    ! Ensures
    call this%mark_as_defined
  end subroutine

  ! Return the solution to the 1D heat equation with periodic boundary conditions
  ! See Rouson, Xia, and Xu (2011), p. 105.
  function solve(this,x,t) result(u)
    use math_constants, only : pi
    class(burgers), intent(in) :: this
    real(rkind), intent(in) :: x(:),t
    real(rkind), dimension(size(x)) :: phi,dPhi_dx,u
    real(rkind), parameter :: two_pi=2._rkind*pi, four_pi = 4._rkind*pi
    integer(ikind) :: i
    integer(ikind) , parameter :: n_max=100,n(-n_max:n_max)=[(i,i=-n_max,n_max)] 
    ! Requires
    call assert(this%user_defined(),error_message("solve: received undefined burgers object"))
    associate( nu=>this%diffusion)
      associate( exponential=>exp(-((x(i)-two_pi*n)**2)/(4._rkind*nu*t)) )
        do concurrent(i=1:size(x))
          phi(i) = sum( exponential )/sqrt(four_pi*nu)
          dPhi_dx(i) = sum( exponential*(-2._rkind*((x(i)-two_pi*n))/(4._rkind*nu*t)) )/sqrt(four_pi*nu)
          u = -2._rkind*nu*dPhi_dx/phi
        end do
      end associate
    end associate
  end function

  subroutine output(this,unit,iotype,v_list,iostat,iomsg)
    class(burgers), intent(in) :: this
    integer, intent(in) :: unit
    character(len=*), intent(in) :: iotype
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(len=*), intent(inout) :: iomsg
    print *,this%diffusion
    iostat = 0
  end subroutine

end module
