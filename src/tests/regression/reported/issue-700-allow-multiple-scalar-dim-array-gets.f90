  implicit none

  type package
    integer, allocatable :: surface_fluxes(:)
  end type

  type outbox
    type(package), allocatable :: block_surfaces(:)
  end type

  type(outbox), save :: halo[*]

  integer, parameter :: source_image = 1
  integer, parameter :: message(1)=[99]

  associate (me => this_image())
    if (me == source_image) then
      allocate(halo%block_surfaces(1))
      allocate(halo%block_surfaces(1)%surface_fluxes, source = message)
    end if

    sync all

    if (me /= source_image) then
      allocate(halo%block_surfaces(1))
      allocate(halo%block_surfaces(1)%surface_fluxes, mold = halo[source_image]%block_surfaces(1)%surface_fluxes)
      halo%block_surfaces(1)%surface_fluxes(1) = halo[source_image]%block_surfaces(1)%surface_fluxes(1)

      if (me == source_image + 1) then
        if (halo%block_surfaces(1)%surface_fluxes(1) == 99) then
          write(*,*) 'Test passed.'
        end if
      end if
    end if

  end associate
end
