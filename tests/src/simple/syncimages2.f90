! The test passes if it terminates
! If the test fails you will get an error or a non-termination.

program syncimages2
  implicit none

  if(this_image() == 1) then
    sync images(*)
  else
    sync images(1)
  endif

  sync all

  if(this_image()==1) print *,"Test passed."

end program
