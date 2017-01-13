! SYNC IMAGES([this_image - 1, this_image + 1]) with the STAT=STAT_STOPPED_IMAGE
! specifier and wrap around of image numbers.  The test is intended to check
! that syncing in a ring with a stopped image still terminates all images.

program sync_images_ring
  use, intrinsic:: iso_fortran_env
  implicit none

  integer :: stat_var = 0

  if (num_images() .lt. 2) error stop "Need at least two images to test."

  associate (me => this_image())
    if (me /= 1) then
	  associate (lhs => merge(me - 1, num_images(), me /= 1), &
				 rhs => merge(me + 1, 1, me /= num_images()))
		sync images([lhs, rhs], STAT=stat_var)
		! Only on image 2 and num_images() testing whether a stopped image is
		! present can be done reliably. All other images could be up ahead.
		if (stat_var /= STAT_STOPPED_IMAGE .and. me == 2) error stop "Error: stat_var /= STAT_STOPPED_IMAGE: "
		if(me == 2) print *, 'Test passed.'
	  end associate
	end if
  end associate
end program sync_images_ring
