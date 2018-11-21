program teams_subset
  use iso_fortran_env, only : team_type
  implicit none

  type(team_type) :: team
  integer :: initial_team_image, max_image(2), min_image(2), myteam

  initial_team_image = this_image()

  if (initial_team_image == 1 .or. initial_team_image == num_images()) then
    myteam = 1
  else
    myteam = 2
  end if

  form team (myteam, team)

  if (myteam == 1) then
    change team(team)
      max_image = [initial_team_image, this_image()]
      call co_max(max_image)
      min_image = [initial_team_image, this_image()]
      call co_min(min_image)
    end team
    if (any(min_image /= [1, 1]) .or. any(max_image /= [num_images(), 2])) then
      write(*,*) "Test failed."
      error stop
    end if
  end if

  sync all

  if (initial_team_image == 1) write(*,*) "Test passed."

end program
