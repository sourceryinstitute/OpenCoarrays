program sameloc
  implicit none

  integer,codimension[*] :: a
  integer,dimension(10),codimension[*] :: b,c
  integer,dimension(10) :: t
  
  a = 10
  b(1:5) = 1
  b(6:10) = -1
  c(1:5) = 1
  c(6:10) = -1

  t(:) = b(:)
  t(1:5) = b(2:6)

  sync all

  a = a[1]
  if (this_image() == 1) write(*,*) 'OK'

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
