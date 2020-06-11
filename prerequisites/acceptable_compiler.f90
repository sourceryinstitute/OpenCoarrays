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
  !! input: acceptable compiler version in the form major.minor.patch
  !! output:
  !!   .true. if compiler version >= acceptable version
  !!   .false. otherwise
  use iso_fortran_env, only : compiler_version
  implicit none

  integer, parameter :: first_argument=1, max_version_length=len('999.999.999')
  integer stat
  character(len=max_version_length) acceptable_version

  call get_command_argument(first_argument,acceptable_version,status=stat)
  call validate_command_line( stat )

  print *, meets_minimum( acceptable_version )

contains

  pure function meets_minimum( required_version ) result( acceptable)
    character(len=*), intent(in) :: required_version
    logical acceptable

    acceptable = .false. ! default result

    associate( actual_version => compiler_version() )
      associate(major_version=>major(actual_version), acceptable_major=>major(required_version))
        if (major_version < acceptable_major) return
        if (major_version > acceptable_major) then
          acceptable = .true.
          return
        end if
        associate(minor_version=>minor(actual_version), acceptable_minor=>minor(required_version))
          if (minor_version < acceptable_minor) return
          if (minor_version > acceptable_minor) then
            acceptable = .true.
            return
          end if
          associate(patch_version=>patch(actual_version), acceptable_patch=>patch(required_version))
            if (patch_version < acceptable_patch) return
            acceptable = .true.
          end associate
        end associate
      end associate
    end associate

  end function

  pure subroutine validate_command_line( command_line_status )
    integer, intent(in) :: command_line_status
    select case(command_line_status)
      case(-1)
         error stop "acceptable_compiler.f90: insufficient string length in attempt to read command argument"
      case(0)
        ! successful command argument read
      case(1:)
         error stop "acceptable_compiler.f90: no version-number supplied"
      case default
         error stop "invalid status"
    end select
   end subroutine

  pure function major(version_string) result(major_value)
    character(len=*), intent(in) :: version_string
    integer major_value
    character(len=:), allocatable :: leading_digits

    associate( first_dot => scan(version_string, '.') )
      associate( first_digit => scan( version_string(1:first_dot-1), '0123456789' ) )
        leading_digits = version_string( first_digit : first_dot-1  )
        read(leading_digits,*) major_value
      end associate
    end associate

  end function

  pure function minor(version_string) result(minor_value)
    character(len=*), intent(in) :: version_string
    integer minor_value
    character(len=:), allocatable :: middle_digits

    associate( first_dot => scan(version_string, '.') )
      associate( second_dot => first_dot + scan(version_string(first_dot+1:), '.') )
        middle_digits = version_string( first_dot+1 : second_dot-1 )
        read(middle_digits,*) minor_value
      end associate
    end associate

  end function

  pure function patch(version_string) result(patch_value)
    use iso_fortran_env, only : iostat_end
    character(len=*), intent(in) :: version_string
    integer patch_value, io_stat
    character(len=:), allocatable :: trailing_digits

    associate( first_dot => scan(version_string, '.') )
      associate( second_dot => first_dot + scan(version_string(first_dot+1:), '.') )
        associate( next_non_digit=> second_dot + next_printable_non_digit(version_string(second_dot+1:)) )
          trailing_digits = version_string( second_dot+1 : next_non_digit-1 )
          read(trailing_digits, * , iostat=io_stat) patch_value
        end associate
      end associate
    end associate

  end function

  pure function next_printable_non_digit( string ) result(location)
    character(len=*), intent(in) :: string
    integer i, location
    integer, parameter :: ASCII_non_digit(*)=[(i,i=32,47),(i,i=58,126)]
    character(len=1), parameter :: non_digit(*)=[( char(ASCII_non_digit(i)) , i=1, size(ASCII_non_digit) )]
    character(len=size(non_digit)) non_digit_string
    write(non_digit_string,'(85a)') non_digit
    location = scan(string,non_digit_string)
    if (location==0) location=len(string)+1
  end function

end program
