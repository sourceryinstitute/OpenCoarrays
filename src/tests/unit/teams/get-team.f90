! BSD 3-Clause License
!
! Copyright (c) 2016, Sourcery Institute
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
program main
  !! summary: Test get_team function, an OpenCoarrays-specific language extension
  use opencoarrays, only : get_team, &
                           team_number !! TODO: remove team_number once gfortran supports it
  use iso_c_binding, only : c_int

  implicit none

  integer(c_int)  :: comm
    !! MPI communicator
  comm = get_team()

  check_communicator: block
    use mpi, only : MPI_COMM_SIZE, MPI_COMM_RANK
    integer(c_int) :: isize,ierror,irank

    call MPI_COMM_SIZE(comm, isize, ierror)
    call assert( ierror==0 , "successful call MPI_COMM_SIZE" )

    call MPI_COMM_RANK(comm, irank, ierror)
    call assert( ierror==0 , "successful call MPI_COMM_RANK" )

    call assert( isize==num_images() , "num MPI ranks = num CAF images " )
    call assert( irank==this_image()-1 , "correct rank/image-number correspondence" )

  end block check_communicator

  sync all
  if (this_image()==1) print *,"Test passed."

contains

  elemental subroutine assert(assertion,description)
    !! TODO: move this to a common place for all tests to use
    logical, intent(in) :: assertion
    character(len=*), intent(in) :: description
    integer, parameter :: max_digits=12
    character(len=max_digits) :: image_number
    if (.not.assertion) then
      write(image_number,*) this_image()
      error stop "Assertion " // description // "failed on image " // trim(image_number)
    end if
  end subroutine

end program
