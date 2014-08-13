program sendrecv
  use iso_fortran_env
  implicit none

  integer :: me, np, i
  integer, parameter :: n=1000
  real(kind=real64), allocatable :: d(:)[:]

  allocate(d(n)[*])
  
  np = num_images()
  me = this_image()

  do i=1,n
     d(i) = i
  enddo

  sync all

  if(me == 1) d(:)[2] = d

  sync all
  
  deallocate(d)

end program
