program test_sendget_by_ref
  implicit none
  type :: rank1_type
    integer, allocatable :: A(:)
  end type
  type :: rank2_type
    integer, allocatable :: A(:,:)
  end type
  type(rank1_type) :: R_get[*]
  type(rank2_type) :: R_send[*]
  integer :: i, j
  logical :: res = .True.

  allocate(R_get%A(this_image()), source=-1)
  R_get%A(this_image()) = this_image()

  allocate(R_send%A(num_images(),num_images()), source=-2)

  sync all

  do i = 1, num_images()
    do j = 1, num_images()
      R_send[i]%A(j,this_image()) = R_get[j]%A(j)
    end do
  end do

  sync all

  do i = 1, num_images()
    if (any(R_send%A(:,i) /= (/(j, j = 1, num_images())/))) res = .False.
  end do

  call co_reduce(res, both)
  write(*,*) this_image(), ':', R_get%A, '|', R_send%A

  if (this_image() == 1) then
    if (res) then
      write(*,*) "Test passed."
    else
      write(*,*) "Test failed."
    end if
  end if
contains

  pure function both(lhs, rhs) result(res)
    logical, intent(in) :: lhs, rhs
    logical :: res
    res = lhs .AND. rhs
  end function
end program test_sendget_by_ref
