program main
  implicit none
  integer, parameter :: MAX_STRING=100
  character(len=MAX_STRING) :: greeting[*] ! Scalar coarray
  integer image
  write(greeting,"(2(a,i2))") " Greetings from image ",this_image()," of ",num_images()
  sync all ! Barrier
  if (this_image()==1) then 
    do concurrent (image=1:num_images())
      print *,greeting[image]
    end do
    print *,"Test passed."
  end if
end program
