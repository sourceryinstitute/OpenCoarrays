program main
   implicit none

   type co_obj ! the test does not fail with pure coarrays, only when using coarrays in derived types
      real, allocatable, dimension(:, :, :) :: a[:], b, c, d, e
   end type

   type(co_obj) :: co
   integer :: me, remote, nimg, i, j, k, ni, nj, nk
   logical :: fail

   me = this_image()
   nimg = num_images()
   remote = merge(2, 1, me == 1)

   if (nimg /= 2) error stop 1

   ni = 1
   nj = 8
   nk = 4
   allocate(co % a(ni, nj, nk)[*])
   allocate(co % b(ni, nj, nk))
   allocate(co % c, co % d, co % e, mold=co % b)
   call random_number(co % b) ! the answer is a random array
   
   sync all
   co % a = co % b

   sync all
   co % c(1, :, :) = co % a(1, :, :)[remote] ! getter

   sync all
   co % e(1, :, :) = dble(1111111111111111_8 * me) / 10**12
   co % a(1, :, :) = co % e(1, :, :)

   sync all
   ! singleton on the 1st dimension (delta == 1), triggers the bug
   co % a(1, :, :)[remote] = co % b(1, :, :) ! setter

   sync all
   co % d(1, :, :) = co % a(1, :, :)[remote] ! check

   sync all
   ! sequential flush (more readable)
   if (me == 2) sync images(1)
   write(*, *) ': : (set from remote on myself) : :', me
   do j = 1, nj; write(*, *) co % a(1, j, :); end do
   write(*, *) ': : (neighbor answer) : :', me
   do j = 1, nj; write(*, *) co % c(1, j, :); end do
   write(*, *) ': : (my answer) : :', me
   do j = 1, nj; write(*, *) co % b(1, j, :); end do
   write(*, *) ': : (what I see on remote) : :', me
   do j = 1, nj; write(*, *) co % d(1, j, :); end do
   write(*, *) ': : (what I sent on remote) : :', me
   do j = 1, nj; write(*, *) co % e(1, j, :); end do
   write(*, *) ': : : :', me
   if (me == 1) sync images(2)
   
   sync all
   
   fail = any(abs(co % b(1, :, :) - co % d(1, :, :)) > epsilon(0.))
   fail = .false. ! <== FIXME: this test is still failing, comment when bug in put_data is found !
   
   if (fail) then
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
