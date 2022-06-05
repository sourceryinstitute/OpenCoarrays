! Fortran 2015 feature support for Fortran 2008 compilers
!
! Copyright (c) 2015-2022, Sourcery Institute
! Copyright (c) 2015-2022, Archaeologic Inc.
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!     * Redistributions of source code must retain the above copyright
!       notice, this list of conditions and the following disclaimer.
!     * Redistributions in binary form must reproduce the above copyright
!       notice, this list of conditions and the following disclaimer in the
!       documentation and/or other materials provided with the distribution.
!     * Neither the name of the Sourcery, Inc., nor the
!       names of its contributors may be used to endorse or promote products
!       derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE FOR ANY
! DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
! (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
! LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
! ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
! SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!
module opencoarrays
  implicit none

  private
  public :: team_number
  public :: get_communicator

  interface

    function team_number(team_type_ptr) result(my_team_number) bind(C,name="_gfortran_caf_team_number")
       use iso_c_binding, only : c_int,c_ptr
       implicit none
       type(c_ptr), optional :: team_type_ptr
       integer(c_int) :: my_team_number
    end function

    function get_communicator(team_type_ptr) result(my_team) bind(C,name="_gfortran_caf_get_communicator")
       use iso_c_binding, only : c_int,c_ptr
       implicit none
       type(c_ptr), optional :: team_type_ptr
       integer(c_int) :: my_team
    end function

  end interface

end module
