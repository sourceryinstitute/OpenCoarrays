program test1caf
  implicit none
  integer, parameter :: n=3
  real :: a(n)[*]=0.1e0,b(n)[*]=0.2e0
!  real :: a(n)[*],b(n)[*]
  integer :: i,j,me,np,left,right

  me = this_image()
  np = num_images()
  
  if(me == 1) then
     left = np
  else
     left = me -1
  endif

  if(me == np) then
     right = 1
  else
     right = me +1
  endif
 
!  a = 0.1e0
!  b = 0.2e0

!  sync all

  do i=1,n
     if (mod(me,2).eq.0) then
        a(i)[right] = a(i)[right]+me
     else
        b(i)[left] = b(i)[left]+me
     endif
  enddo

  if(me==1) then
     write(*,*) me, a, b
  else
     sync images(me-1)
     write(*,*) me, a, b
  endif

  if(me < np) sync images(me+1)

end program test1caf
