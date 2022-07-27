program hello_coarrays
  implicit none
  type :: array_type
    integer, allocatable :: values(:)
  end type
  type(array_type) :: array[*]
  allocate(array%values(2), source=0)
  array%values = this_image()
  if (all(array%values(:) .EQ. this_image())) then
    print *,"Test passed."
  end if
end program

