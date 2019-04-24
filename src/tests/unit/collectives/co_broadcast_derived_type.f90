module object_interface
  implicit none
  private
  public :: object

  type object
    private
    integer :: foo=0
    logical :: bar=.false.
  contains
    procedure :: initialize
    procedure :: co_broadcast_me
    procedure :: not_equal
    procedure :: copy
    generic :: operator(/=)=>not_equal
    generic :: assignment(=)=>copy
  end type

  interface
    elemental impure module subroutine initialize(this,foo_,bar_)
      implicit none
      class(object), intent(out) :: this
      integer, intent(in) :: foo_
      logical, intent(in) :: bar_
    end subroutine

    elemental impure module subroutine co_broadcast_me(this,source_image)
      implicit none
      class(object), intent(inout) :: this
      integer, intent(in) :: source_image
    end subroutine

    elemental module function not_equal(lhs,rhs) result(lhs_ne_rhs)
      implicit none
      class(object), intent(in) :: lhs,rhs
      logical lhs_ne_rhs
    end function

    elemental impure module subroutine copy(lhs,rhs)
      implicit none
      class(object), intent(inout) :: lhs
      class(object), intent(in) :: rhs
    end subroutine
  end interface

end module

submodule(object_interface) object_implementation
  implicit none
contains
    module procedure co_broadcast_me
      call co_broadcast(this%foo,source_image)
      call co_broadcast(this%bar,source_image)
    end procedure

    module procedure initialize
      this%foo = foo_
      this%bar = bar_
    end procedure

    module procedure not_equal
      lhs_ne_rhs = (lhs%foo /= rhs%foo) .or. (lhs%bar .neqv. rhs%bar)
    end procedure

    module procedure copy
       lhs%foo = rhs%foo
       lhs%bar = rhs%bar
    end procedure
end submodule

program main
  use object_interface, only : object
  implicit none
  type(object) message

  call message%initialize(foo_=1,bar_=.true.)

  emulate_co_broadcast: block
    type(object) foobar
    if (this_image()==1) foobar = message
    call foobar%co_broadcast_me(source_image=1)
    if ( foobar /= message ) error stop "Test failed."
  end block emulate_co_broadcast

  desired_co_broadcast: block
    type(object) barfoo
    if (this_image()==1) barfoo = message
    call co_broadcast(barfoo,source_image=1)  ! OpenCoarrays terminates here with the message "Unsupported data type"
    if ( barfoo /= message ) error stop "Test failed."
  end block desired_co_broadcast

  sync all  ! Wait for each image to pass the test
  if (this_image()==1) print *,"Test passed."

end program main
