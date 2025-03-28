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
use, intrinsic :: iso_fortran_env, only: team_type

integer :: caf(2,2)[3,*], sub(2)
integer, allocatable :: res(:)
type(team_type) :: team

form team(1, team, new_index=MOD(this_image() + 43, num_images()) + 1)

associate(me => this_image(), num => num_images())
    associate(n_ind => MOD(me + 43, num) + 1)

        j1 = this_image()
        if (j1 /= me) stop 1

        sub = c_t_i(lcobound(caf), ucobound(caf), me)
        res = this_image(caf)
        if (any (res /= sub)) stop 2

        j2 = this_image(caf, 1)
        if (j2 /= sub(1)) stop 3

        j3 = this_image(team)
        if (j3 /= n_ind) stop 4

        sub = c_t_i(lcobound(caf), ucobound(caf), n_ind)
        res = this_image(caf, team)
        if (any(res /= sub)) stop 5

        j4 = this_image(caf, 1, team)
        if (j4 /= sub(1)) stop 6

        change team(team)
            j5 = this_image()
            if (j5 /= n_ind) stop 11

            sub = c_t_i(lcobound(caf), ucobound(caf), n_ind)
            res = this_image(caf)
            if (any (res /= sub)) stop 12

            j6 = this_image(caf, 1)
            if (j6 /= sub(1)) stop 13

            j7 = this_image(team)
            if (j7 /= n_ind) stop 14

            sub = c_t_i(lcobound(caf), ucobound(caf), n_ind)
            res = this_image(caf, team)
            if (any(res /= sub)) stop 15

            j8 = this_image(caf, 1, team)
            if (j8 /= sub(1)) stop 16

        end team
        sync all

        if (me == 1) print *,"Test passed."
    end associate
end associate

contains

function c_t_i(lco, uco, me) result(sub)
  integer, intent(in) :: lco(:), uco(:)
  integer, intent(in) :: me
  integer :: i, n, m, ml, extend
  integer :: sub(size(lco))

  n = size(lco)
  m = me - 1
  do i = 1, n - 1
    extend = uco(i) - lco(i) + 1
    ml = m
    m = m / extend
    sub(i) = ml - m * extend + lco(i)
  end do
  sub(n) = m + lco(n)

end function
end
