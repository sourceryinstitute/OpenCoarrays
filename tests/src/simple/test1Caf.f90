program test1caf
  implicit none
  integer, parameter :: num_local_elems=3,a_initial=1,b_initial=2
  integer :: a(num_local_elems)[*]=a_initial,b(num_local_elems)[*]=b_initial
  integer :: i,me,np,left,right

  me = this_image()
  np = num_images()
  
  left  = merge(np,me-1,me==1)
  right = merge(1,me+1,me==np)
 
  if (mod(me,2).eq.0) then
     a(:)[right] = a(:)[right]+me
  else
     b(:)[left] = b(:)[left]+me
  end if

  if(me==1) then
     write(*,*) me, a, b
  else
     sync images(me-1)
     write(*,*) me, a, b
  end if

  if(me < np) sync images(me+1)

  if (mod(me,2).eq.0) then
    if ( any(a(:)[right]/=a_initial+me)) error stop "Test failed."
  else
    if ( any(b(:)[left]/=b_initial+me)) error stop "Test failed."
  end if
  
  if (me==1) print *,"Test passed."

end program test1caf
