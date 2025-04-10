program main
  use, intrinsic :: iso_fortran_env, only: team_type
  implicit none
  integer, parameter :: PARENT_TEAM = 1, CURRENT_TEAM = 2, CHILD_TEAM = 3
  type(team_type) :: team(3)

  if (num_images() < 8) error stop "I need at least 8 images to function."
  associate(ni => num_images())

  form team (1, team(PARENT_TEAM))
  change team (team(PARENT_TEAM))
    form team (ni + mod(this_image(),2)+1, team(CURRENT_TEAM))
    change team (team(CURRENT_TEAM))
      form team(2 * ni + mod(this_image(),2)+1, team(CHILD_TEAM))
      sync team(team(PARENT_TEAM))
      ! change order / number of syncs between teams to try to expose deadlocks
      if (team_number() == 1) then
         sync team(team(CURRENT_TEAM))
         sync team(team(CHILD_TEAM))
      else
         sync team(team(CHILD_TEAM))
         sync team(team(CURRENT_TEAM))
         sync team(team(CHILD_TEAM))
         sync team(team(CURRENT_TEAM))
      end if
    end team
  end team
  end associate

  sync all

  if (this_image() == 1) write(*,*) "Test passed."

end program
