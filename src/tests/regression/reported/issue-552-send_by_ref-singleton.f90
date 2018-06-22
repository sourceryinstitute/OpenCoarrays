program main
   implicit none

   type co_obj ! the test does not fail with pure coarrays, only when using derived types
      real, allocatable :: a(:, :, :)[:], b(:, :, :)
   end type

   integer :: me, nimg
   type(co_obj) :: co

   me = this_image()
   nimg = num_images()

   if (nimg /= 2) error stop 1

   allocate(co % a(1, 10, 20)[*], co % b(1, 10, 20))
   call random_number(co % b)

   ! singleton on 1st dimension (delta == 1)
   co % a(1, :, :)[merge(2, 1, me == 1)] = co % b(1, :, :)

   ! icar output
   ! 3/4: Entering send_by_ref(may_require_tmp = 1, dst_type = 3).
   ! 3/4: _gfortran_caf_send_by_ref() remote_image = 3, offset = 0, remote_mem = 0x7f4a29422060
   ! 3/4: _gfortran_caf_send_by_ref() remote desc rank: 3 (ref_rank: 7297736)
   ! 3/4: _gfortran_caf_send_by_ref() remote desc dim[0] = (lb = 1, ub = 1, stride = 1)
   ! 3/4: _gfortran_caf_send_by_ref() remote desc dim[1] = (lb = 1, ub = 20, stride = 1)
   ! 3/4: _gfortran_caf_send_by_ref() remote desc dim[2] = (lb = 1, ub = 103, stride = 20)
   ! 3/4: _gfortran_caf_send_by_ref() extent(dst, 0): 1 != delta: 20.  ! <====== mistmatch src_cur_dim not incremented
   ! 3/4: _gfortran_caf_send_by_ref() extent(dst, 1): 20 != delta: 101.

   write(*, *) 'Test passed.'
end program
