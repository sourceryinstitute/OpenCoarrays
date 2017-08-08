program main
  !! summary: Test team_number intrinsic function
  implicit none

  interface
    function team_number(team_type_ptr) result(my_team_number) bind(C,name="_gfortran_caf_team_number")
       !! TODO: remove this interface block after the compiler supports team_number
       use iso_c_binding, only : c_int,c_ptr
       implicit none
       type(c_ptr), value :: team_type_ptr
       integer(c_int) :: my_team_number
    end function
  end interface

  check_initial_team_number: block
    use iso_fortran_env, only : team_type
    use iso_c_binding, only : c_loc
    integer, parameter :: standard_initial_value=-1
    type(team_type), target :: home
    call assert(team_number(c_loc(home))==standard_initial_value,"initial team number conforms with Fortran standard")
  end block check_initial_team_number

  sync all
  if (this_image()==1) print *,"Test passed."

contains

  elemental subroutine assert(assertion,description)
    !! TODO: move this to a common place for all tests to use
    logical, intent(in) :: assertion
    character(len=*), intent(in) :: description
    integer, parameter :: max_digits=12
    character(len=max_digits) :: image_number
    if (.not.assertion) then
      write(image_number,*) this_image()
      error stop "Assertion " // description // "failed on image " // trim(image_number)
    end if
  end subroutine

end program
