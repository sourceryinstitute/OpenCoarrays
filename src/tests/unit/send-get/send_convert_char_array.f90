program send_convert_char_array

  implicit none

  character(kind=1, len=:), allocatable, codimension[:] :: co_str_k1_scal
  character(kind=1, len=:), allocatable :: str_k1_scal
  character(kind=4, len=:), allocatable, codimension[:] :: co_str_k4_scal
  character(kind=4, len=:), allocatable :: str_k4_scal

  character(kind=1, len=:), allocatable, codimension[:] :: co_str_k1_arr(:)
  character(kind=1, len=:), allocatable :: str_k1_arr(:)
  character(kind=4, len=:), allocatable, codimension[:] :: co_str_k4_arr(:)
  character(kind=4, len=:), allocatable :: str_k4_arr(:)

  associate(me => this_image(), np => num_images())
    if (np < 2) error stop 'Can not run with less than 2 images.'

    allocate(str_k1_scal, SOURCE='abcdefghij')
    allocate(str_k4_scal, SOURCE=4_'abcdefghij')
    allocate(character(len=20)::co_str_k1_scal[*]) ! allocate syncs here
    allocate(character(kind=4, len=20)::co_str_k4_scal[*]) ! allocate syncs here

    allocate(str_k1_arr(1:4), SOURCE=['abc', 'EFG', 'klm', 'NOP'])
    allocate(str_k4_arr(1:4), SOURCE=[4_'abc', 4_'EFG', 4_'klm', 4_'NOP'])
    allocate(character(len=5)::co_str_k1_arr(4)[*])
    allocate(character(kind=4, len=5)::co_str_k4_arr(4)[*])

    ! First check send/copy to self
    if (me == 1) then
      co_str_k1_scal[1] = str_k1_scal
      print *, '#' // co_str_k1_scal // '#, len:', len(co_str_k1_scal)
      if (co_str_k1_scal /= str_k1_scal // '          ') error stop 'send scalar kind=1 to kind=1 self failed.'

      co_str_k4_scal[1] = str_k4_scal
      print *, 4_'#' // co_str_k4_scal // 4_'#, len:', len(co_str_k4_scal)
      if (co_str_k4_scal /= str_k4_scal // 4_'          ') error stop 'send scalar kind=4 to kind=4 self failed.'

      co_str_k4_scal[1] = str_k1_scal
      print *, 4_'#' // co_str_k4_scal // 4_'#, len:', len(co_str_k4_scal)
      if (co_str_k4_scal /= str_k4_scal // 4_'          ') error stop 'send scalar kind=1 to kind=4 self failed.'

      co_str_k1_scal[1] = str_k4_scal
      print *, '#' // co_str_k1_scal // '#, len:', len(co_str_k1_scal)
      if (co_str_k1_scal /= str_k1_scal // '          ') error stop 'send scalar kind=4 to kind=1 self failed.'
    end if

    ! Do the same for arrays but on image 2
    if (me == 2) then
      co_str_k1_arr(:)[2] = str_k1_arr
      print *, '#' // co_str_k1_arr(:) // '#, len:', len(co_str_k1_arr(1))
      if (any(co_str_k1_arr /= ['abc  ', 'EFG  ', 'klm  ', 'NOP  '])) error stop 'send array kind=1 to kind=1 self failed.'
     
      print *, str_k4_arr 
      co_str_k4_arr(:)[2] = [4_'abc', 4_'EFG', 4_'klm', 4_'NOP']! str_k4_arr
      print *, 4_'#' // co_str_k4_arr(:) // 4_'#, len:', len(co_str_k4_arr(1))
      if (any(co_str_k4_arr /= [4_'abc  ', 4_'EFG  ', 4_'klm  ', 4_'NOP  '])) error stop 'send array kind=4 to kind=4 self failed.'

      co_str_k4_arr(:)[2] = str_k1_arr
      print *, 4_'#' // co_str_k4_arr(:) // 4_'#, len:', len(co_str_k4_arr(1))
      if (any(co_str_k4_arr /= [ 4_'abc  ', 4_'EFG  ', 4_'klm  ', 4_'NOP  '])) error stop 'send array kind=1 to kind=4 self failed.'

      co_str_k1_arr(:)[2] = str_k4_arr
      print *, '#' // co_str_k1_arr(:) // '#, len:', len(co_str_k1_arr(1))
      if (any(co_str_k1_arr /= ['abc  ', 'EFG  ', 'klm  ', 'NOP  '])) error stop 'send array kind=4 to kind=1 self failed.'
    end if

    sync all
    if (me == 1) then
      co_str_k1_scal[2] = str_k1_scal

      co_str_k4_scal[2] = str_k4_scal
    end if

    sync all
    if (me == 2) then
      print *, '#' // co_str_k1_scal // '#, len:', len(co_str_k1_scal)
      if (co_str_k1_scal /= str_k1_scal // '          ') error stop 'send kind=1 to kind=1 image 2 failed.'

      print *, 4_'#' // co_str_k4_scal // 4_'#, len:', len(co_str_k4_scal)
      if (co_str_k4_scal /= str_k4_scal // 4_'          ') error stop 'send kind=4 to kind=4 image 2 failed.'
    end if

    sync all
    if (me == 1) then
      co_str_k4_scal[2] = str_k1_scal

      co_str_k1_scal[2] = str_k4_scal
    end if

    sync all
    if (me == 2) then
      print *, 4_'#' // co_str_k4_scal // 4_'#, len:', len(co_str_k4_scal)
      if (co_str_k4_scal /= str_k4_scal // 4_'          ') error stop 'send kind=1 to kind=4 to image 2 failed.'

      print *, '#' // co_str_k1_scal // '#, len:', len(co_str_k1_scal)
      if (co_str_k1_scal /= str_k1_scal // '          ') error stop 'send kind=4 to kind=1 to image 2 failed.'
    end if

    co_str_k1_arr(:) = '#####'
    co_str_k4_arr(:) = 4_'#####'
    
    sync all

    if (me == 1) then
      co_str_k1_arr(::2)[2] = 'foo'
      co_str_k4_arr(::2)[2] = ['bar', 'baz']
    end if

    sync all
    if (me == 2) then
      print *, co_str_k1_arr
      if (any(co_str_k1_arr /= ['foo  ', '#####', 'foo  ', '#####'])) &
        & error stop "strided send char arr kind 1 to kind 1 failed."
      print *, co_str_k4_arr
      if (any(co_str_k4_arr /= [4_'bar  ', 4_'#####', 4_'baz  ', 4_'#####'] )) &
        & error stop "strided send char arr kind 1 to kind 4 failed."
    end if

    sync all
    if (me == 1) print *, 'Test passed.'
  end associate
end program send_convert_char_array

! vim:ts=2:sts=2:sw=2:
