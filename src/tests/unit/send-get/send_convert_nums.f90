program send_convert_int_array

  implicit none

  real(kind=4), parameter :: tolerance4 = 1.0e-4
  real(kind=8), parameter :: tolerance4to8 = 1.0E-4
  real(kind=8), parameter :: tolerance8 = 1.0E-6

  integer(kind=1), allocatable, codimension[:] :: co_int_scal_k1
  integer(kind=1), allocatable                 :: int_scal_k1
  integer(kind=4), allocatable, codimension[:] :: co_int_scal_k4
  integer(kind=4), allocatable                 :: int_scal_k4
  real(kind=4)   , allocatable, codimension[:] :: co_real_scal_k4
  real(kind=4)   , allocatable                 :: real_scal_k4
  real(kind=8)   , allocatable, codimension[:] :: co_real_scal_k8
  real(kind=8)   , allocatable                 :: real_scal_k8

  integer(kind=1), allocatable, dimension(:), codimension[:] :: co_int_k1
  integer(kind=1), allocatable, dimension(:)                 :: int_k1
  integer(kind=4), allocatable, dimension(:), codimension[:] :: co_int_k4
  integer(kind=4), allocatable, dimension(:)                 :: int_k4
  real(kind=4)   , allocatable, dimension(:), codimension[:] :: co_real_k4
  real(kind=4)   , allocatable, dimension(:)                 :: real_k4
  real(kind=8)   , allocatable, dimension(:), codimension[:] :: co_real_k8
  real(kind=8)   , allocatable, dimension(:)                 :: real_k8

  associate(me => this_image(), np => num_images())
    if (np < 2) error stop 'Can not run with less than 2 images.'

    allocate(int_scal_k1, SOURCE=INT(42, 1))
    allocate(int_scal_k4, SOURCE=42)
    allocate(co_int_scal_k1[*]) ! allocate syncs here
    allocate(co_int_scal_k4[*]) ! allocate syncs here
    allocate(int_k1, SOURCE=INT([ 5, 4, 3, 2, 1], 1))
    allocate(int_k4, SOURCE=[ 5, 4, 3, 2, 1])
    allocate(co_int_k1(5)[*]) ! allocate syncs here
    allocate(co_int_k4(5)[*]) ! allocate syncs here

    allocate(real_scal_k4, SOURCE=37.042)
    allocate(real_scal_k8, SOURCE=REAL(37.042, 8))
    allocate(co_real_scal_k4[*]) ! allocate syncs here
    allocate(co_real_scal_k8[*]) ! allocate syncs here
    allocate(real_k4, SOURCE=[ 5.1, 4.2, 3.3, 2.4, 1.5])
    allocate(real_k8, SOURCE=REAL([ 5.1, 4.2, 3.3, 2.4, 1.5], 8))
    allocate(co_real_k4(5)[*]) ! allocate syncs here
    allocate(co_real_k8(5)[*]) ! allocate syncs here
    ! First check send/copy to self
    if (me == 1) then
      co_int_scal_k1[1] = int_scal_k1
      print *, co_int_scal_k1
      if (co_int_scal_k1 /= int_scal_k1) error stop 'send scalar int kind=1 to kind=1 self failed.'

      co_int_scal_k4[1] = int_scal_k4
      print *, co_int_scal_k4
      if (co_int_scal_k4 /= int_scal_k4) error stop 'send scalar int kind=4 to kind=4 self failed.'

      co_int_scal_k4[1] = int_scal_k1
      print *, co_int_scal_k4
      if (co_int_scal_k4 /= int_scal_k4) error stop 'send scalar int kind=1 to kind=4 self failed.'

      co_int_scal_k1[1] = int_scal_k4
      print *, co_int_scal_k1
      if (co_int_scal_k1 /= int_scal_k1) error stop 'send scalar int kind=4 to kind=1 self failed.'

      co_int_k1(:)[1] = int_k1
      print *, co_int_k1
      if (any(co_int_k1 /= int_k1)) error stop 'send int kind=1 to kind=1 self failed.'

      co_int_k4(:)[1] = int_k4
      print *, co_int_k4
      if (any(co_int_k4 /= int_k4)) error stop 'send int kind=4 to kind=4 self failed.'

      co_int_k4(:)[1] = int_k1
      print *, co_int_k4
      if (any(co_int_k4 /= int_k4)) error stop 'send int kind=1 to kind=4 self failed.'

      co_int_k1(:)[1] = int_k4
      print *, co_int_k1
      if (any(co_int_k1 /= int_k1)) error stop 'send int kind=4 to kind=1 self failed.'
    else if (me == 2) then ! Do the real copy to self checks on image 2
      co_real_scal_k4[2] = real_scal_k4
      print *, co_real_scal_k4
      if (abs(co_real_scal_k4 - real_scal_k4) > tolerance4) error stop 'send scalar real kind=4 to kind=4 self failed.'

      co_real_scal_k8[2] = real_scal_k8
      print *, co_real_scal_k8
      if (abs(co_real_scal_k8 - real_scal_k8) > tolerance8) error stop 'send scalar real kind=8 to kind=8 self failed.'

      co_real_scal_k8[2] = real_scal_k4
      print *, co_real_scal_k8
      if (abs(co_real_scal_k8 - real_scal_k8) > tolerance4to8) error stop 'send scalar real kind=4 to kind=8 self failed.'

      co_real_scal_k4[2] = real_scal_k8
      print *, co_real_scal_k4
      if (abs(co_real_scal_k4 - real_scal_k4) > tolerance4) error stop 'send scalar real kind=8 to kind=4 self failed.'

      co_real_k4(:)[2] = real_k4
      print *, co_real_k4
      if (any(abs(co_real_k4 - real_k4) > tolerance4)) error stop 'send real kind=4 to kind=4 self failed.'

      co_real_k8(:)[2] = real_k8
      print *, co_real_k8
      if (any(abs(co_real_k8 - real_k8) > tolerance8)) error stop 'send real kind=8 to kind=8 self failed.'

      co_real_k8(:)[2] = real_k4
      print *, co_real_k8
      if (any(abs(co_real_k8 - real_k8) > tolerance4to8)) error stop 'send real kind=4 to kind=8 self failed.'

      co_real_k4(:)[2] = real_k8
      print *, co_real_k4
      if (any(abs(co_real_k4 - real_k4) > tolerance4)) error stop 'send real kind=8 to kind=4 self failed.'
    end if

    sync all
    if (me == 1) then
      co_int_scal_k1[2] = int_scal_k1

      co_int_scal_k4[2] = int_scal_k4

      co_int_k1(:)[2] = int_k1

      co_int_k4(:)[2] = int_k4

      co_real_scal_k4[2] = real_scal_k4

      co_real_scal_k8[2] = real_scal_k8

      co_real_k4(:)[2] = real_k4

      co_real_k8(:)[2] = real_k8
    end if

    sync all
    if (me == 2) then
      print *, co_int_scal_k1
      if (co_int_scal_k1 /= int_scal_k1) error stop 'send scalar int kind=1 to kind=1 to image 2 failed.'

      print *, co_int_scal_k4
      if (co_int_scal_k4 /= int_scal_k4) error stop 'send scalar int kind=4 to kind=4 to image 2 failed.'

      print *, co_int_k1
      if (any(co_int_k1 /= int_k1)) error stop 'send int kind=1 to kind=1 to image 2 failed.'

      print *, co_int_k4
      if (any(co_int_k4 /= int_k4)) error stop 'send int kind=4 to kind=4 to image 2 failed.'

      print *, co_real_scal_k4
      if (abs(co_real_scal_k4 - real_scal_k4) > tolerance4) error stop 'send scalar real kind=4 to kind=4 to image 2 failed.'

      print *, co_real_scal_k8
      if (abs(co_real_scal_k8 - real_scal_k8) > tolerance8) error stop 'send scalar real kind=8 to kind=8 to image 2 failed.'

      print *, co_real_k4
      if (any(abs(co_real_k4 - real_k4) > tolerance4)) error stop 'send real kind=4 to kind=4 to image 2 failed.'

      print *, co_real_k8
      if (any(abs(co_real_k8 - real_k8) > tolerance8)) error stop 'send real kind=8 to kind=8 to image 2 failed.'
    end if

    sync all
    if (me == 1) then
      co_int_scal_k4[2] = int_scal_k1

      co_int_scal_k1[2] = int_scal_k4

      co_int_k4(:)[2] = int_k1

      co_int_k1(:)[2] = int_k4

      co_real_scal_k8[2] = real_scal_k4

      co_real_scal_k4[2] = real_scal_k8

      co_real_k8(:)[2] = real_k4

      co_real_k4(:)[2] = real_k8
    end if

    sync all
    if (me == 2) then
      print *, co_int_scal_k4
      if (co_int_scal_k4 /= int_scal_k4) error stop 'send scalar int kind=1 to kind=4 to image 2 failed.'

      print *, co_int_scal_k1
      if (co_int_scal_k1 /= int_scal_k1) error stop 'send scalar int kind=4 to kind=1 to image 2 failed.'

      print *, co_int_k4
      if (any(co_int_k4 /= int_k4)) error stop 'send int kind=1 to kind=4 to image 2 failed.'

      print *, co_int_k1
      if (any(co_int_k1 /= int_k1)) error stop 'send int kind=4 to kind=1 to image 2 failed.'

      print *, co_real_scal_k8
      if (abs(co_real_scal_k8 - real_scal_k8) > tolerance4to8) error stop 'send scalar real kind=4 to kind=8 to image 2 failed.'

      print *, co_real_scal_k4
      if (abs(co_real_scal_k4 - real_scal_k4) > tolerance4) error stop 'send scalar real kind=8 to kind=4 to image 2 failed.'

      print *, co_real_k8
      if (any(abs(co_real_k8 - real_k8) > tolerance4to8)) error stop 'send real kind=4 to kind=8 to image 2 failed.'

      print *, co_real_k4
      if (any(abs(co_real_k4 - real_k4) > tolerance4)) error stop 'send real kind=8 to kind=4 to image 2 failed.'
    end if

    ! Scalar to array replication
    sync all
    if (me == 1) then
      co_int_k4(:)[2] = int_scal_k4

      co_int_k1(:)[2] = int_scal_k1

      co_real_k8(:)[2] = real_scal_k8

      co_real_k4(:)[2] = real_scal_k4
    end if

    sync all
    if (me == 2) then
      print *, co_int_k4
      if (any(co_int_k4 /= int_scal_k4)) error stop 'send int scal kind=4 to array kind=4 to image 2 failed.'

      print *, co_int_k1
      if (any(co_int_k1 /= int_scal_k1)) error stop 'send int scal kind=1 to array kind=1 to image 2 failed.'

      print *, co_real_k8
      if (any(abs(co_real_k8 - real_scal_k8) > tolerance8)) error stop 'send real kind=8 to array kind=8 to image 2 failed.'

      print *, co_real_k4
      if (any(abs(co_real_k4 - real_scal_k4) > tolerance4)) error stop 'send real kind=4 to array kind=4 to image 2 failed.'
    end if

    ! and with kind conversion
    sync all
    if (me == 1) then
      co_int_k4(:)[2] = int_scal_k1

      co_int_k1(:)[2] = int_scal_k4

      co_real_k8(:)[2] = real_scal_k4

      co_real_k4(:)[2] = real_scal_k8
    end if

    sync all
    if (me == 2) then
      print *, co_int_k4
      if (any(co_int_k4 /= int_scal_k4)) error stop 'send int scal kind=1 to array kind=4 to image 2 failed.'

      print *, co_int_k1
      if (any(co_int_k1 /= int_scal_k1)) error stop 'send int scal kind=4 to array kind=1 to image 2 failed.'

      print *, co_real_k8
      if (any(abs(co_real_k8 - real_scal_k8) > tolerance8)) error stop 'send real kind=4 to array kind=8 to image 2 failed.'

      print *, co_real_k4
      if (any(abs(co_real_k4 - real_scal_k4) > tolerance4)) error stop 'send real kind=8 to array kind=4 to image 2 failed.'
    end if
    ! and with type conversion
    sync all
    if (me == 1) then
      co_int_k4(:)[2] = real_scal_k4

      co_int_k1(:)[2] = real_scal_k4

      co_real_k8(:)[2] = int_scal_k4

      co_real_k4(:)[2] = int_scal_k4
    end if

    sync all
    if (me == 2) then
      print *, co_int_k4
      if (any(co_int_k4 /= INT(real_scal_k4, 4))) error stop 'send real scal kind=4 to int array kind=4 to image 2 failed.'

      print *, co_int_k1
      if (any(co_int_k1 /= INT(real_scal_k4, 1))) error stop 'send real scal kind=1 to int array kind=1 to image 2 failed.'

      print *, co_real_k8
      if (any(abs(co_real_k8 - int_scal_k4) > tolerance4to8)) error stop 'send int kind=4 to real array kind=8 to image 2 failed.'

      print *, co_real_k4
      if (any(abs(co_real_k4 - int_scal_k4) > tolerance4)) error stop 'send int kind=4 to real array kind=4 to image 2 failed.'
    end if
    sync all
    if (me == 1) print *, "Test passed."
  end associate
end program send_convert_int_array

! vim:ts=2:sts=2:sw=2:
