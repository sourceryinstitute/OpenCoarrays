! This program tests the capability of copying data on the
! same memory location and within the same image from
! different memory locations.
! NOTE:
! In order to run this test successfully the efficient
! strided transfer support must be disabled.
program sameloc
  implicit none

  integer,codimension[*] :: a
  integer,dimension(10),codimension[*] :: b,c
  integer,dimension(9,10),codimension[*] :: m
  integer,dimension(10) :: t
  integer :: i,j

  a = 10
  b(1:5) = 1
  b(6:10) = -1
  c(1:5) = 1
  c(6:10) = -1

  t(:) = b(:)
  t(1:5) = b(2:6)

  do i=1,9
    m(i,:) = (/ (j, j = 1, 10) /)
  enddo

  sync all

  a = a[1]
  if (this_image() == 1) write(*,*) 'OK',a

  t = (/ (j, j = 1, 10) /)

  if(this_image() == 1) then
    c = m(1,:)[1]
    if(any(c(:) /= t(:))) then
      call abort()
    else
      write(*,*) 'ok get row'
    endif
  endif

  sync all

  if(this_image() == 1) then
    do i=1,10
      if(m(9,i)[1] /= t(i)) then
	write(*,*) 'pos',i,'value get',m(9,i)[1],'value t',t(i)
	call abort()
      endif
    enddo
  endif

  if(this_image() == 1) write(*,*) 'Ok get element from matrix'

  sync all

  m(9,:) = 1

  if(this_image() == 1) then
    do i=1,10
      m(9,i)[1] = i
      if(m(9,i)[1] /= t(i)) then
        write(*,*) 'pos',i,'value get',m(9,i)[1],'value t',t(i)
        call abort()
      endif
    enddo
  endif

  if(this_image() == 1 ) write(*,*) 'Ok put element from matrix'

  t(:) = b(:)
  t(1:5) = b(2:6)

  c(1:5) = 1
  c(6:10) = -1

  sync all

  if(this_image() == 1) then
    b(1:5)[1] = b(2:6)
    if(any(b(:) /= t(:))) then
      call abort()
    else
      write(*,*) 'OK put overlapped'
    endif
  endif

  b(1:5) = 1
  b(6:10) = -1

  sync all

  if(this_image() == 1) then
    b(1:5)[1] = b(2:6)[1]
    if(any(b(:) /= t(:))) then
      call abort()
    else
      write(*,*) 'OK putget overlapped'
    endif
  endif

  t(:) = c(:)
  t(10:1:-1) = t(:)

  sync all

  if(this_image() == 1) then
    c(10:1:-1)[1] = c(:)
    if(any(t(:) /= c(:))) then
      write(*,*) 'Error in put reversed'
      write(*,*) c
      write(*,*) t
      call abort()
    else
      write(*,*) 'OK put reversed'
    endif
  endif

  c(1:5) = 1
  c(6:10) = -1

  t(:) = c(:)
  t(10:1:-1) = t(:)

  if(this_image() == 1) then
    c(:) = c(10:1:-1)[1]
    if(any(t(:) /= c(:))) then
      write(*,*) c
      write(*,*) t
      call abort()
    else
      write(*,*) 'OK get reversed'
    endif
  endif

end program
