! OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
! OpenCoarrays -- An open-source transport layer supporting coarray Fortran compilers
! Copyright (c) 2015, Sourcery, Inc.
! Copyright (c) 2015, Sourcery Institute
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without modification, are permitted
! provided that the following conditions are met:
!
! 1. Redistributions of source code must retain the above copyright notice, this list of conditions
!    and the following disclaimer.
! 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
!    and the following disclaimer in the documentation and/or other materials provided with the distribution.
! 3. Neither the names of the copyright holders nor the names of their contributors may be used to endorse or
!    promote products derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
! WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
! PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
! ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
! TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
! HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
! NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
! POSSIBILITY OF SUCH DAMAGE.
!____________________________
!
! Test the invocation of the intrinsic functions this_image() and num_images()
program main
  ! Import the OpenCoarrays capabilities for extending non-coarray Fortran compilers
  use iso_fortran_env, only : error_unit
  use opencoarrays
  implicit none
  integer :: me,argc,i
  character(len=:), allocatable :: argv(:)

  argc=command_argument_count()
  allocate( argv(argc), source=repeat(" ",ncopies=max_argument_length()) )
  do i=1,argc
    call get_command_argument(i,argv(i))
  end do
  call caf_init(argc,c_loc(argv))

  me=this_image()
  print *,"Hello from image ",me,"of",num_images()
  if (me<0 .or. me>num_images()) error stop

  call caf_finalize(argc,c_loc(argv))

contains

    function max_argument_length()
      character(len=0) :: dummy
      integer max_argument_length, n, length_of_argument_n, error_flag

      max_argument_length=0
      do n=1,command_argument_count()
        call get_command_argument(n,dummy,status=error_flag,length=length_of_argument_n)
        max_argument_length = max(length_of_argument_n,max_argument_length)
      end do
    end function

end program
