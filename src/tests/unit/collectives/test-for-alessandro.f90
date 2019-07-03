program main
  !! This test seg faults for if num_images() >  1
  implicit none

  type dynamic
    integer, allocatable :: vector(:)
  end type
  type(dynamic) alloc_message, alloc_content
   
  integer, parameter :: sender=1 !! co_broadcast source_image

  alloc_content%vector = [1,2,3,4]

  if (this_image()==sender) then
     alloc_message = alloc_content
  else
     allocate(alloc_message%vector(4))
  end if

  sync all

  call co_broadcast(alloc_message,source_image=sender)
  if ( any(alloc_message%vector /= alloc_content%vector) ) error stop "Test failed."

  sync all  ! Wait for each image to pass the test
  if (this_image()==sender) print *,"Test passed."

end program main
