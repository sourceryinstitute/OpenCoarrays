program main
   implicit none

   type co_obj ! the test does not fail with pure coarrays, only when using coarrays in derived types
      real, allocatable :: a(:, :, :)[:], b(:, :, :)
   end type

   integer :: me, remote, nimg
   type(co_obj) :: co

   me = this_image()
   nimg = num_images()
   remote = merge(2, 1, me == 1)

   if (nimg /= 2) error stop 1

   allocate(co % a(1, 4, 8)[*], source=99.)
   allocate(co % b(1, 4, 8))
   call random_number(co % b)
   sync all

   ! singleton on the 1st dimension (delta == 1), triggers the bug
   co % a(1, :, :)[remote] = co % b(1, :, :)

   sync all
   if (any(abs(co % b(1, :, :) - co % a(1, :, :)[remote]) > epsilon(0.))) then
      write(*, *) 'Test failed!'
      error stop 5
   else
      write(*, *) 'Test passed.'
   end if

end program

! icar output
! 3/4: Entering send_by_ref(may_require_tmp = 1, dst_type = 3).
! 3/4: _gfortran_caf_send_by_ref() remote_image = 3, offset = 0, remote_mem = 0x7f4a29422060
! 3/4: _gfortran_caf_send_by_ref() remote desc rank: 3 (ref_rank: 7297736)
! 3/4: _gfortran_caf_send_by_ref() remote desc dim[0] = (lb = 1, ub = 1, stride = 1)
! 3/4: _gfortran_caf_send_by_ref() remote desc dim[1] = (lb = 1, ub = 20, stride = 1)
! 3/4: _gfortran_caf_send_by_ref() remote desc dim[2] = (lb = 1, ub = 103, stride = 20)
! 3/4: _gfortran_caf_send_by_ref() extent(dst, 0): 1 != delta: 20.  ! <====== mistmatch src_cur_dim not incremented
! 3/4: _gfortran_caf_send_by_ref() extent(dst, 1): 20 != delta: 101.
