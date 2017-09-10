program send_convert_char_array

  implicit none

  character(kind=1, len=:), allocatable, codimension[:] :: co_str_k1_1
  character(kind=1, len=:), allocatable :: str_k1_1
  character(kind=4, len=:), allocatable, codimension[:] :: co_str_k4_1
  character(kind=4, len=:), allocatable :: str_k4_1

  associate(me => this_image(), np => num_images())
    if (np < 2) error stop 'Can not run with less than 2 images.'

    allocate(str_k1_1, SOURCE='abcdefghij')
    allocate(str_k4_1, SOURCE=4_'abcdefghij')
    allocate(character(len=20)::co_str_k1_1[*]) ! allocate syncs here
    allocate(character(kind=4, len=20)::co_str_k4_1[*]) ! allocate syncs here
    ! First check send/copy to self
    if (me == 1) then
      co_str_k1_1[1] = str_k1_1
      print *, '#' // co_str_k1_1 // '#, len:', len(co_str_k1_1)
      if (co_str_k1_1 /= str_k1_1 // '          ') error stop 'send kind=1 to kind=1 self failed.'

      co_str_k4_1[1] = str_k4_1
      print *, 4_'#' // co_str_k4_1 // 4_'#, len:', len(co_str_k4_1)
      if (co_str_k4_1 /= str_k4_1 // 4_'          ') error stop 'send kind=4 to kind=4 self failed.'

      co_str_k4_1[1] = str_k1_1
      print *, 4_'#' // co_str_k4_1 // 4_'#, len:', len(co_str_k4_1)
      if (co_str_k4_1 /= str_k4_1 // 4_'          ') error stop 'send kind=1 to kind=4 self failed.'

      co_str_k1_1[1] = str_k4_1
      print *, '#' // co_str_k1_1 // '#, len:', len(co_str_k1_1)
      if (co_str_k1_1 /= str_k1_1 // '          ') error stop 'send kind=4 to kind=1 self failed.'
    end if

    sync all
    if (me == 1) then
      co_str_k1_1[2] = str_k1_1

      co_str_k4_1[2] = str_k4_1
    end if

    sync all
    if (me == 2) then
      print *, '#' // co_str_k1_1 // '#, len:', len(co_str_k1_1)
      if (co_str_k1_1 /= str_k1_1 // '          ') error stop 'send kind=1 to kind=1 image 2 failed.'

      print *, 4_'#' // co_str_k4_1 // 4_'#, len:', len(co_str_k4_1)
      if (co_str_k4_1 /= str_k4_1 // 4_'          ') error stop 'send kind=4 to kind=4 image 2 failed.'
    end if

    sync all
    if (me == 1) then
      co_str_k4_1[2] = str_k1_1

      co_str_k1_1[2] = str_k4_1
    end if

    sync all
    if (me == 2) then
      print *, 4_'#' // co_str_k4_1 // 4_'#, len:', len(co_str_k4_1)
      if (co_str_k4_1 /= str_k4_1 // 4_'          ') error stop 'send kind=1 to kind=4 to image 2 failed.'

      print *, '#' // co_str_k1_1 // '#, len:', len(co_str_k1_1)
      if (co_str_k1_1 /= str_k1_1 // '          ') error stop 'send kind=4 to kind=1 to image 2 failed.'
    end if

    sync all
    if (me == 1) print *, 'Test passed.'
  end associate
end program send_convert_char_array

! vim:ts=2:sts=2:sw=2:
