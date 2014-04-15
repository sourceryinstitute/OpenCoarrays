module local_field_module
  implicit none
  private
  public :: local_field

  type local_field
    private
    real, allocatable :: values(:)
  contains
    procedure, private :: multiply
    generic :: operator(*)=>multiply
    procedure :: state
    procedure, private :: assign_array
    generic :: assignment(=)=>assign_array
  end type

contains

  pure function multiply(lhs,rhs) result(product_)
    class(local_field), intent(in) :: lhs
    type(local_field) :: product_
    real, intent(in) :: rhs
    product_%values = lhs%values*rhs
  end function

  pure function state(this) result(this_values)
    class(local_field), intent(in) :: this
    real :: this_values(size(this%values))
    this_values = this%values
  end function

  pure subroutine assign_array(lhs,rhs)
    class(local_field), intent(out) :: lhs
    real, intent(in) :: rhs(:)
    lhs%values = rhs
  end subroutine

end module
