program event_post_1
  use iso_fortran_env
  implicit none

  type(event_type), allocatable :: snd_copied(:)[:]
 
  associate(me => this_image(), np => num_images())
    allocate(snd_copied(np)[*])
    if (me == 2) print*,'I am  image 2, I am posting to 4'
    if (me == 2) event post(snd_copied(2)[4])
    if (me == 2) print*,' I am image 2, I have posted to 4'
    if (me == 4) then
      event wait(snd_copied(2))
      print *, 'Test passed.'
    end if
    if (allocated(snd_copied)) deallocate(snd_copied)
  end associate
end program event_post_1
! vim:ts=2:sts=2:sw=2:

