!> @name OpenCoarrays Example: Coarray Hello World
!! @{

!> <BR> Get and print greetings from each image.
!> @brief This program constructs a unique greeting in each image. Image gets the greetings from other 
!! images, prints all of the greetings, and tests the greetings from images 2-9 against the expected content.

!****p* examples/coarrayHelloWorld/hello_multiverse
! NAME 
!   hello_multiverse 
! SYNOPSIS
!   Demonstrate coarray communication via a scalar character coarray.
! INPUTS
!   None. 
! OUTPUTS
!   Test pass or failure. 
!******

program main
  implicit none
  integer, parameter :: MAX_STRING=100
  character(len=MAX_STRING) :: greeting[*] ! Scalar coarray
  integer image
  write(greeting,"(2(a,i2))") "Greetings from image ",this_image()," of ",num_images()
  sync all ! Barrier
  if (this_image()==1) then 
    do concurrent (image=1:num_images())
      print *,greeting[image]
    end do
    block 
      integer, parameter :: expected_location=23,max_single_digit=9
      do image=2,min(num_images(),max_single_digit)
        ! Verify that the greetings have their image number at the expected location:
        if (scan(greeting[image],set="123456789")/=expected_location) error stop "Test failed"
      end do
    end block
    print *,"Test passed."
  end if
end program
