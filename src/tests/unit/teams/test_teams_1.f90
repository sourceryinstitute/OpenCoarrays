! BSD 3-Clause License
!
! Copyright (c) 2018-2025, Sourcery Institute
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!
! * Redistributions of source code must retain the above copyright notice, this
!   list of conditions and the following disclaimer.
!
! * Redistributions in binary form must reproduce the above copyright notice,
!   this list of conditions and the following disclaimer in the documentation
!   and/or other materials provided with the distribution.
!
! * Neither the name of the copyright holder nor the names of its
!   contributors may be used to endorse or promote products derived from
!   this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
! DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
! SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
! CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
! OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

program test_teams_1
    use, intrinsic :: iso_fortran_env
    use oc_assertions_interface, only : assert

    integer :: caf(3,3)[*] != 42
    type(team_type) :: row_team, column_team

    caf = reshape((/(-i, i = 1, 9 )/), [3,3])
    associate(me => this_image(), np => num_images())
        call assert(np == 9, "I need exactly 9 teams.")

        ! Form a row team
        form team((me - 1) / 3 + 1, row_team, new_index=mod(me - 1, 3) + 1)
        row_t: change team(row_team, row[*] => caf(:, team_number(row_team)))
            ! Form column teams; each team has only one image
            form team (this_image(), column_team)
            col_t: change team(column_team, cell[*] => row(this_image()))
                cell = team_number(row_team)
                if (this_image(row_team) /= 1) row(this_image(row_team))[1, team=row_team] = cell
            end team col_t
            sync team(row_team)
            if (this_image() == 1) caf(:, team_number(row_team))[1, team_number = -1] = row
        end team row_t
        sync all
        if (me == 1) then
            if (all(caf == reshape([1,1,1,2,2,2,3,3,3], [3, 3]))) then
                print *, "Test passed."
            else
                print *, "Test failed."
                print *, "Expected:", reshape([1,1,1,2,2,2,3,3,3], [3, 3])
                print *, "Got     :", caf
            end if
        end if
    end associate

end program test_teams_1
