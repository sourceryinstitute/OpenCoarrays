program testred
  use mpi
  implicit none

  integer, allocatable :: x(:)[:],y(:)
  integer, parameter   :: n = 10
  integer              :: ierr,me,np,i,res

  me = this_image()
  np = num_images()

  allocate(x(n)[*])

  do i=1,n
     x(i) = i+(me-1)
  enddo

  sync all

  call co_sum(x,result_image=1)

  if(me == 1) write(*,*) x

end program testred
