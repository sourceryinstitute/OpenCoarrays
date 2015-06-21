program strided_get
  implicit none

  integer :: i,me,np
  integer,allocatable :: a(:,:,:,:)[:],b(:,:,:,:)

  me = this_image()
  np = num_images()

  allocate(a(0:11,-10:-5,-1:0,-1:5)[*],b(6,6,2,7))

  a = me
  b = me

  sync all

  if(me == 2) then
    b(1:2,:,:,:) = a(0:1,:,:,:)[me-1]
    if(any(b(1:2,:,:,:) /= 1)) call abort()
    write(*,*) 'Test passed.'
  endif

end program
