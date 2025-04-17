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

program teams_coindexed
    use, intrinsic :: iso_fortran_env

    type(team_type) :: parentteam, team, formed_team
    integer :: t_num= 42, stat = 42, lhs
    integer(kind=2) :: st_num=42
    integer :: caf(2)[*]

    parentteam = get_team()

    caf = [23, 32]
    form team(t_num, team)
    form team(t_num, formed_team)

    associate(me => this_image())
        change team(team, cell[*] => caf(2))
            ! for get_from_remote
            ! Checking against caf_single is very limitted.
            if (cell[me, team_number=t_num] /= 32) stop 1
            if (cell[me, team_number=st_num] /= 32) stop 2
            if (cell[me, team=parentteam] /= 32) stop 3

            ! Check that team_number is validated
            lhs = cell[me, team_number=5, stat=stat]
            if (stat /= 1) stop 4

            ! Check that only access to active teams is valid
            stat = 42
            lhs = cell[me, team=formed_team, stat=stat]
            if (stat /= 1) stop 5

            ! for send_to_remote
            ! Checking against caf_single is very limitted.
            cell[me, team_number=t_num] = 45
            if (cell /= 45) stop 11
            cell[me, team_number=st_num] = 46
            if (cell /= 46) stop 12
            cell[me, team=parentteam] = 47
            if (cell /= 47) stop 13

            ! Check that team_number is validated
            stat = -1
            cell[me, team_number=5, stat=stat] = 0
            if (stat /= 1) stop 14

            ! Check that only access to active teams is valid
            stat = 42
            cell[me, team=formed_team, stat=stat] = -1
            if (stat /= 1) stop 15

            ! for transfer_between_remotes
            ! Checking against caf_single is very limitted.
            cell[me, team_number=t_num] = caf(1)[me, team_number=-1]
            if (cell /= 23) stop 21
            cell[me, team_number=st_num] = caf(2)[me, team_number=-1]
            ! cell is an alias for caf(2) and has been overwritten by caf(1)!
            if (cell /= 23) stop 22
            cell[me, team=parentteam] = caf(1)[me, team= team]
            if (cell /= 23) stop 23

            ! Check that team_number is validated
            stat = -1
            cell[me, team_number=5, stat=stat] = caf(1)[me, team_number= -1]
            if (stat /= 1) stop 24
            stat = -1
            cell[me, team_number=t_num] = caf(1)[me, team_number= -2, stat=stat]
            if (stat /= 1) stop 25

            ! Check that only access to active teams is valid
            stat = 42
            cell[me, team=formed_team, stat=stat] = caf(1)[me]
            if (stat /= 1) stop 26
            stat = 42
            cell[me] = caf(1)[me, team=formed_team, stat=stat]
            if (stat /= 1) stop 27
        end team

        sync all
        if (me == 1) print *, "Test passed."
    end associate
end program teams_coindexed
