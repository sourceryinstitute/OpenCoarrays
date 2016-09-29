program get_offset_1d
  implicit none

  integer,allocatable :: a(:)[:],b(:)
  integer :: me,np,i

  me = this_image()
  np = num_images()

  allocate(a(100)[*],b(10))

  a = (/ (i, i=1,100) /)

  do i=1,100
    a(i) = a(i) + me
  enddo

  sync all

  if(me < np) then
    b(:) = a(21:30)[me+1]
  endif

  if(me == 1) then
    do i=1,10
      if(b(i) /= 20+i+me+1) call abort()
    enddo
    write(*,*) 'Test passed.'
  endif
sync all
end program
