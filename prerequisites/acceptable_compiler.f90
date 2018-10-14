!
! acceptable_compiler
!
! -- Report whether the compiler version equals or exceeds the first
!    OpenCoarrays-aware version
!
! OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
! Copyright (c) 2015, 2016, Sourcery, Inc.
! Copyright (c) 2015, 2016, Sourcery Institute
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without modification,
! are permitted provided that the following conditions are met:
!
! 1. Redistributions of source code must retain the above copyright notice, this
!    list of conditions and the following disclaimer.
! 2. Redistributions in binary form must reproduce the above copyright notice, this
!    list of conditions and the following disclaimer in the documentation and/or
!    other materials provided with the distribution.
! 3. Neither the names of the copyright holders nor the names of their contributors
!    may be used to endorse or promote products derived from this software without
!    specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
! ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
! IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
! INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
! NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
! PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
! WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
! POSSIBILITY OF SUCH DAMAGE.

program main
  use iso_fortran_env, only : compiler_version
  implicit none
  integer, parameter :: first_argument=1
  integer stat
  character(len=9) version_number
  call get_command_argument(first_argument,version_number,status=stat)
  select case(stat)
    case(-1)
       error stop "acceptable_compiler.f90: insufficient string length in attempt to read command argument"
    case(0)
      ! successful command argument read
    case(1:)
       error stop "acceptable_compiler.f90: no version-number supplied"
    case default
       error stop "invalid status"
  end select
  print *,(compiler_version() >= "GCC version "//adjustl(trim(version_number))//" ")
end program
