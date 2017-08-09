program async_comp_alloc_2

  implicit none
  type :: nonsymmetric
     real, dimension(:), allocatable :: arr
  end type

  type(nonsymmetric), codimension[*] :: parent_obj

  sync all

  associate(me => this_image())
    if (me == 2) then
      allocate(parent_obj%arr(3))
      print *, 'Image 2: memory allocated.'
    end if

    sync all

    if (me == 1) then
      print *, 'Alloc status on [2]: ', allocated(parent_obj[2]%arr)
      print *, 'Image 2 has size ', size(parent_obj[2]%arr), ' asymmetric allocation'
      if (size(parent_obj[2]%arr) /= 3) error stop 'Test failed.'
      print *, 'Test passed.'
    end if
    sync all
  end associate
end program

