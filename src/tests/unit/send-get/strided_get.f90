program strided_get
  implicit none

  integer :: i,me,np
  integer,allocatable :: a(:,:,:,:)[:],b(:,:,:,:)

  me = this_image()
  np = num_images()

  allocate(a(0:11,-10:-5,-1:0,-1:5)[*],b(6,5,2,6))

  a = me

  sync all

  if(me > 1) then
    b(1:6,:,:,:) = a(0:5,:,:,:)[me-1]
    write(*,*) b(1:2,:,:,:)
  endif

end program
