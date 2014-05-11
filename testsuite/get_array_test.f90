!
! This program does a correctness check for
! ... = ARRAY[idx] and ... = SCALAR[idx]
!
program main
  implicit none
  integer, parameter :: n = 3
  integer, parameter :: m = 4

  ! Allocatable coarrays
  call one(-5, 1)
  call one(0, 0)
  call one(1, -5)
  call one(0, -11)

  ! Static coarrays
!  call two()
!  call three()
  write(*,*) 'Test passed'
contains
  subroutine one(lb1, lb2)
    integer, value :: lb1, lb2

    integer :: i_sgn1, i_sgn2, i, i_e, i_s, j, j_e, j_s
    integer, allocatable :: caf(:,:)[:]
    integer, allocatable :: a(:,:), b(:,:), c(:,:)

    allocate(caf(lb1:n+lb1-1, lb2:m+lb2-1)[*], &
         a(lb1:n+lb1-1, lb2:m+lb2-1), &
         b(lb1:n+lb1-1, lb2:m+lb2-1), &
         c(lb1:n+lb1-1, lb2:m+lb2-1))

    b = reshape([(i*33, i = 1, size(b))], shape(b))

    ! Whole array: ARRAY = ARRAY
    caf = -42
    a = -42
    c = -42
    if (this_image() == num_images()) then
      caf(:,:) = b(:,:)
    endif
    sync all
    a(:,:) = b(:,:)
    c(:,:) = caf(:,:)[num_images()]
    if (any (a /= c)) then
      print *, 'RES 1:', any (a /= c)
      print *, a
      print *, c
      ! FIXME: Without the print lines above, it always fails. Why?
      call abort()
    end if

    ! Scalar assignment
    caf = -42
    a = -42
    c = -42
    if (this_image() == num_images()) then
      caf(:,:) = b(:,:)
    endif
    sync all
    do j = lb2, m+lb2-1
      do i = n+lb1-1, lb1, -2
        a(i,j) = b(i,j)
        c(i,j) = caf(i,j)[num_images()]
      end do
    end do
    do j = lb2, m+lb2-1
      do i = lb1, n+lb1-1, 2
        a(i,j) = b(i,j)
        c(i,j) = caf(i,j)[num_images()]
      end do
    end do
    if (any (a /= c)) then
      print *, 'RES 2:', any (a /= c)
      print *, this_image(), ': ', a
      print *, this_image(), ': ', c
      ! FIXME: Without the print lines above, it always fails. Why?
      call abort()
    end if

    ! Array sections with different ranges and pos/neg strides
    do i_sgn1 = -1, 1, 2
      do i_sgn2 = -1, 1, 2
        do i=lb1, n+lb1-1
          do i_e=lb1, n+lb1-1
            do i_s=1, n
              do j=lb2, m+lb2-1
                do j_e=lb2, m+lb2-1
                  do j_s=1, m
                    ! ARRAY = ARRAY
                    caf = -42
                    a = -42
                    c = -42
                    if (this_image() == num_images()) then
                      caf(:,:) = b(:,:)
                    endif
                    sync all
                    a(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) &
                         = b(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)
                    c(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) &
                         = caf(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)[num_images()]
                    if (any (c /= a)) then
                      print *, 'RES 3:', any (a /= c)
                      print *, a
                      print *, c
                      ! FIXME: Without the print lines above, it always fails. Why?
                      call abort()
                    end if
                  end do
                end do
              end do
            end do
          end do
        end do
      end do
    end do
  end subroutine one

  subroutine two()
    integer, parameter :: lb1 = -5, lb2 = 1

    integer :: i_sgn1, i_sgn2, i, i_e, i_s, j, j_e, j_s
    integer, save :: caf(lb1:n+lb1-1, lb2:m+lb2-1)[*]
    integer, save :: a(lb1:n+lb1-1, lb2:m+lb2-1)
    integer, save :: b(lb1:n+lb1-1, lb2:m+lb2-1)

    b = reshape([(i*33, i = 1, size(b))], shape(b))

    ! Whole array: ARRAY = SCALAR
    caf = -42
    a = -42
    a(:,:) = b(lb1, lb2)
    sync all
    if (this_image() == 1) then
      caf(:,:)[num_images()] = b(lb1, lb2)
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Whole array: ARRAY = ARRAY
    caf = -42
    a = -42
    a(:,:) = b(:, :)
    sync all
    if (this_image() == 1) then
      caf(:,:)[num_images()] = b(:, :)
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Scalar assignment
    caf = -42
    a = -42
    do j = lb2, m+lb2-1
      do i = n+lb1-1, 1, -2
        a(i,j) = b(i,j)
      end do
    end do
    do j = lb2, m+lb2-1
      do i = 1, n+lb1-1, 2
        a(i,j) = b(i,j)
      end do
    end do
    sync all
    if (this_image() == 1) then
      do j = lb2, m+lb2-1
        do i = n+lb1-1, 1, -2
          caf(i,j)[num_images()] = b(i, j)
        end do
      end do
      do j = lb2, m+lb2-1
        do i = 1, n+lb1-1, 2
          caf(i,j)[num_images()] = b(i, j)
        end do
      end do
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Array sections with different ranges and pos/neg strides
    do i_sgn1 = -1, 1, 2
      do i_sgn2 = -1, 1, 2
        do i=lb1, n+lb1-1
          do i_e=lb1, n+lb1-1
            do i_s=1, n
              do j=lb2, m+lb2-1
                do j_e=lb2, m+lb2-1
                  do j_s=1, m
                    ! ARRAY = SCALAR
                    caf = -42
                    a = -42
                    a(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) = b(lb1, lb2)
                    sync all
                    if (this_image() == 1) then
                      caf(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)[num_images()] &
                           = b(lb1, lb2)
                    end if
                    sync all

                    ! ARRAY = ARRAY
                    caf = -42
                    a = -42
                    a(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) &
                         = b(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)
                    sync all
                    if (this_image() == 1) then
                      caf(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)[num_images()] &
                           = b(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)
                    end if
                    sync all

                    if (this_image() == num_images()) then
                      if (any (a /= caf)) then
                        print '(*(g0))', "bounds: ", lb1,":",n+lb1-1,", ", &
                             lb2,":",m+lb2-1
                        print '(*(g0))', "section: ", i,":",i_e,":",i_s*i_sgn1, &
                             ", ", j,":",j_e,":",j_s*i_sgn2
                        print *, i
                        print *, a
                        print *, caf
                        print *, a-caf
                        call abort()
                      endif
                    end if
                  end do
                end do
              end do
            end do
          end do
        end do
      end do
    end do
  end subroutine two

  subroutine three()
    integer, parameter :: lb1 = 0, lb2 = 0

    integer :: i_sgn1, i_sgn2, i, i_e, i_s, j, j_e, j_s
    integer, save :: caf(lb1:n+lb1-1, lb2:m+lb2-1)[*]
    integer, save :: a(lb1:n+lb1-1, lb2:m+lb2-1)
    integer, save :: b(lb1:n+lb1-1, lb2:m+lb2-1)

    b = reshape([(i*33, i = 1, size(b))], shape(b))

    ! Whole array: ARRAY = SCALAR
    caf = -42
    a = -42
    a(:,:) = b(lb1, lb2)
    sync all
    if (this_image() == 1) then
      caf(:,:)[num_images()] = b(lb1, lb2)
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Whole array: ARRAY = ARRAY
    caf = -42
    a = -42
    a(:,:) = b(:, :)
    sync all
    if (this_image() == 1) then
      caf(:,:)[num_images()] = b(:, :)
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Scalar assignment
    caf = -42
    a = -42
    do j = lb2, m+lb2-1
      do i = n+lb1-1, 1, -2
        a(i,j) = b(i,j)
      end do
    end do
    do j = lb2, m+lb2-1
      do i = 1, n+lb1-1, 2
        a(i,j) = b(i,j)
      end do
    end do
    sync all
    if (this_image() == 1) then
      do j = lb2, m+lb2-1
        do i = n+lb1-1, 1, -2
          caf(i,j)[num_images()] = b(i, j)
        end do
      end do
      do j = lb2, m+lb2-1
        do i = 1, n+lb1-1, 2
          caf(i,j)[num_images()] = b(i, j)
        end do
      end do
    end if
    sync all
    if (this_image() == num_images()) then
      if (any (a /= caf)) &
           call abort()
    end if

    ! Array sections with different ranges and pos/neg strides
    do i_sgn1 = -1, 1, 2
      do i_sgn2 = -1, 1, 2
        do i=lb1, n+lb1-1
          do i_e=lb1, n+lb1-1
            do i_s=1, n
              do j=lb2, m+lb2-1
                do j_e=lb2, m+lb2-1
                  do j_s=1, m
                    ! ARRAY = SCALAR
                    caf = -42
                    a = -42
                    a(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) = b(lb1, lb2)
                    sync all
                    if (this_image() == 1) then
                      caf(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)[num_images()] &
                           = b(lb1, lb2)
                    end if
                    sync all

                    ! ARRAY = ARRAY
                    caf = -42
                    a = -42
                    a(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2) &
                         = b(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)
                    sync all
                    if (this_image() == 1) then
                      caf(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)[num_images()] &
                           = b(i:i_e:i_s*i_sgn1, j:j_e:j_s*i_sgn2)
                    end if
                    sync all

                    if (this_image() == num_images()) then
                      if (any (a /= caf)) then
                        print '(*(g0))', "bounds: ", lb1,":",n+lb1-1,", ", &
                             lb2,":",m+lb2-1
                        print '(*(g0))', "section: ", i,":",i_e,":",i_s*i_sgn1, &
                             ", ", j,":",j_e,":",j_s*i_sgn2
                        print *, i
                        print *, a
                        print *, caf
                        print *, a-caf
                        call abort()
                      endif
                    end if
                  end do
                end do
              end do
            end do
          end do
        end do
      end do
    end do
  end subroutine three
end program main
