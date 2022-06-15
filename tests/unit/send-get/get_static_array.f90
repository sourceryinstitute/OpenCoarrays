program get_static_array
    type :: container
        integer, allocatable :: stuff(:)
    end type

    type(container) :: co_containers(10)[*]

    if (this_image() == 1) then
        allocate(co_containers(2)%stuff(4))
        co_containers(2)%stuff = [1,2,3,4]
    end if

    sync all

    if (this_image() == 2) then
        if (any(co_containers(2)[1]%stuff /= [1,2,3,4])) then
          error stop "Test failed."
        else
          print *, "Test passed."
        end if
    end if
end program

