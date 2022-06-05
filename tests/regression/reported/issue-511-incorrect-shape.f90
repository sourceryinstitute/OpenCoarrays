program main
   implicit none

   type foo
      logical, allocatable :: x(:, :)[:]
   end type

   type(foo) :: bar
   integer :: shp_loc(2), shp_rem(2)

   allocate(bar % x(2, 1)[*])

   shp_loc = shape(bar % x) ! local shape
   shp_rem = shape(bar % x(:, :)[1]) ! remote shape on this image
   print *, shp_loc, shp_rem

   if (any(shp_loc /= shp_rem)) then
      write(*, *) 'Test failed!'
      error stop 5
   else
      write(*, *) 'Test passed.'
   end if
end program
