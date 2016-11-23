program stridedsendgettest

  implicit none

  integer, save, dimension(4,6) :: srcmat[*], dstmat[*]
  integer, save, dimension(6) :: srcvec[*], dstvec[*]
  integer :: i
  logical :: test_passed = .true.

  if (this_image() == 2) then
    srcvec = [(2 * i, i = 1, 6)]
    srcmat = reshape([(i * 2, i = 1, 4*6)], [4,6])
  elseif (this_image() == 3) then
    dstmat = -1
    dstvec = -2
  end if

  sync all
  if (this_image() == 1) then
    dstvec(:)[3] = srcvec(:)[2]
    dstmat(3,:)[3] = srcvec(:)[2]
  end if

  sync all
  if (this_image() == 3) then
    if (any(dstvec(:) /= [2, 4, 6, 8, 10, 12])) then
      print *, dstvec(:)
      print *, "SendGet vec/vec does not match."
!      error stop "SendGet vec/vec does not match."
      test_passed = .false.
    end if

    if (any(dstmat(3,:) /= [2, 4, 6, 8, 10, 12])) then
      print *, dstmat(3,:)
      print *, "SendGet matrow/vec does not match."
!      error stop "SendGet matrow/vec does not match."
      test_passed = .false.
    end if
  end if

  sync all
  if (this_image() == 1) then
    dstvec(:)[3] = srcmat(2,:)[2]
    dstmat(3,:)[3] = srcmat(2,:)[2]
  end if

  sync all
  if (this_image() == 3) then
    if (any(dstvec(:) /= [4, 12, 20, 28, 36, 44])) then
      print *, dstvec(:)
      print *, "SendGet vec/matrow does not match."
!      error stop "SendGet vec/matrow does not match."
      test_passed = .false.
    end if

    if (any(dstmat(3,:) /= [4, 12, 20, 28, 36, 44])) then
      print *, dstmat(3,:)
      print *, "SendGet matrow/matrow does not match."
!      error stop "SendGet matrow/matrow does not match."
      test_passed = .false.
    end if

    if (test_passed) print *, "Test passed"
  end if

end program
