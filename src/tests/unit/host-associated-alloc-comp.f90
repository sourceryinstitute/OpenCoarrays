! Demonstrate an internal compiler error (ICE) on EVENT POST of host-associated EVENT_TYPE coarray 
! gfortran PR 70696 (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70696)
module  foo_module
  implicit none
  type foo
    integer, allocatable :: array(:)
  end type
end module

module bar_module
  use foo_module, only : foo
  implicit none
  type(foo) :: x[*]
contains
  subroutine do_something()
    x%array = [0,1] 
  end subroutine
end module

program main
  
end program
