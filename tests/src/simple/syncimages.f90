program syncimages
  implicit none

  integer :: me,ne,i
  integer :: p[*] = 0
  logical :: test[*] = .true.

  me = this_image()
  ne = num_images()

  if(me == 1) then
    p = 1
  else
    sync images( me-1 )
    p = p[me-1] +1
  endif

  if(me<ne) sync images( me+1 )

  if(me /= p) test = .false.

  sync all

  if(me == 1) then
    do i=1,ne
      if(test[i].eqv..false.) error stop "Test failed."
    enddo
  endif

  if(me==1) write(*,*) print *,"Test passed."

end program
