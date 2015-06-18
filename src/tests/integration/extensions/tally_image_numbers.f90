! Compute a parallel collective sum of all image numbers using the
! "opencoarrays" Fortran module that is intended to support non-coarray 
! Fortran compilers.  
program main
  use iso_fortran_env, only : error_unit
  use iso_c_binding, only : c_int ,c_int32_t
  use opencoarrays, only : co_sum,error_stop,sync_all,this_image,num_images
  implicit none    
  integer(c_int) :: me,image
  me=this_image()
  call co_sum(me)
  call sync_all
  ! Verify co_sum by comparing to the expected result:
  if (me/=sum([(image,image=1,num_images())])) then
    ! Report incorrect result:
    write(error_unit,"(2(a,i2))") "Incorrect sum ",me," on image ",this_image()
    ! Halt all images:
    call error_stop
  end if
  ! Wait for every image to pass the test:
  call sync_all
  ! Image 1 reports success:
  if (this_image()==1) print *, "Test passed."
end program
