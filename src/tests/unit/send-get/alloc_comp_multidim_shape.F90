! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
! DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
! FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
! DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
! SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
! CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
! OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

! Comments preceded by "!!" are formatted for the FORD docoumentation generator
program alloc_comp_multidim_shape
  !! summary: Test shape of multidimensional allocatable array coarray components of a derived type shape(object%comp(:,:)[1])
  !! author: Damian Rouson , 2018
  !! date: 2018-03-08
  !!
  !! [OpenCoarrays issue #511](https://github.com/sourceryinstitute/opencoarrays/issues/511)

  implicit none

! TODO: add tests for other types and kinds, including integer, logical, character, and derived types

  type reals
    real, allocatable :: array_2D(:,:)[*]
    real, allocatable :: array_3D(:,:,:)[*]
    real, allocatable :: array_4D(:,:,:,:)[*]
    real, allocatable :: array_5D(:,:,:,:,:)[*]
    real, allocatable :: array_6D(:,:,:,:,:,:)[*]
#ifdef GCC_GE_8
    real, allocatable :: array_7D(:,:,:,:,:,:,:)[*]
    real, allocatable :: array_8D(:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_9D(:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_10D (:,:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_11D(:,:,:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_12D(:,:,:,:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_13D(:,:,:,:,:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_14D(:,:,:,:,:,:,:,:,:,:,:,:,:,:)[*]
    real, allocatable :: array_15D(:,:,:,:,:,:,:,:,:,:,:,:,:,:,:)[*]
#endif
  end type

  type(reals), save :: object

  logical :: error_printed=.false.

  associate(me => this_image(), np => num_images())

      allocate(object%array_2D(2,1)[*])
      if ( any( shape(object%array_2D) /= shape(object%array_2D(:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 2D array component of derived type')

      allocate(object%array_3D(3,2,1)[*])
      if ( any( shape(object%array_3D) /= shape(object%array_3D(:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 3D array component of derived type')

      allocate(object%array_4D(4,3,2,1)[*])
      if ( any( shape(object%array_4D) /= shape(object%array_4D(:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 4D array component of derived type')

      allocate(object%array_5D(5,4,3,2,1)[*])
      if ( any( shape(object%array_5D) /= shape(object%array_5D(:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 5D array component of derived type')

      allocate(object%array_6D(6,5,4,3,2,1)[*])
      if ( any( shape(object%array_6D) /= shape(object%array_6D(:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 6D array component of derived type')

#ifdef GCC_GE_8
      allocate(object%array_7D(7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_7D) /= shape(object%array_7D(:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 7D array component of derived type')

      allocate(object%array_8D(6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_8D) /= shape(object%array_8D(:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 8D array component of derived type')

      allocate(object%array_9D(5,6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_9D) /= shape(object%array_9D(:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 9D array component of derived type')

      allocate(object%array_10D(4,5,6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_10D) /= shape(object%array_10D(:,:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 10D array component of derived type')

      allocate(object%array_11D(3,4,5,6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_11D) /= shape(object%array_11D(:,:,:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 11D array component of derived type')

      allocate(object%array_12D(2,3,4,5,6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_12D) /= shape(object%array_12D(:,:,:,:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 12D array component of derived type')

      allocate(object%array_13D(1,2,3,4,5,6,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_13D) /= shape(object%array_13D(:,:,:,:,:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 13D array component of derived type')

      allocate(object%array_14D(2,3,4,5,6,7,8,7,6,5,4,3,2,1)[*])
      if ( any( shape(object%array_14D) /= shape(object%array_14D(:,:,:,:,:,:,:,:,:,:,:,:,:)[1]) ) )  &
        call print_and_register('incorrect shape for coindexed 14D array component of derived type')

#endif

    check_global_success: block

      logical :: no_error_printed

      no_error_printed = .not. error_printed
      call co_all(no_error_printed,result_image=1)

      if (me==1 .and. no_error_printed) print *,"Test passed."

    end block check_global_success

  end associate

contains

  subroutine print_and_register(error_message)
    use iso_fortran_env, only : error_unit
    character(len=*), intent(in) :: error_message
    write(error_unit,*) error_message
    error_printed=.true.
  end subroutine

  pure function both(lhs,rhs) RESULT(lhs_and_rhs)
    logical, intent(in) :: lhs,rhs
    logical :: lhs_and_rhs
    lhs_and_rhs = lhs .and. rhs
  end function

  subroutine co_all(boolean,result_image)
    logical, intent(inout) :: boolean
    integer, intent(in) :: result_image
    call co_reduce(boolean,both,result_image=result_image)
  end subroutine

end program alloc_comp_multidim_shape

