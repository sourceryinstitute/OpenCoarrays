module mpi_module
!declare what other modules are being used
use kind_parameters, only: rkind, ikind
use object_interface, only : object
use ForTrilinos_assertion_utility, only : assert,error_message
use shared

implicit none
!everything in this module aside from mpi_class is private
private
public ::mpi_class
integer(ikind) :: program_status=0 !integer for keeping track of whether mpi has

!started or not
!extend object class so can make use of assertions
type, extends(object) :: mpi_class

contains
procedure :: output !mandatory extension of object
procedure, nopass :: mpi_begin ! initiate mpi
procedure, nopass :: mpi_end ! end mpi
procedure, nopass :: barrier
procedure, nopass :: oned_message !communicate with neighboors for a 1d pde
end type

contains

  subroutine output(this,unit,iotype,v_list,iostat,iomsg)
    class(mpi_class), intent(in) :: this
    integer, intent(in) :: unit ! Unit on which output happens (negative for internal file)
    character(*), intent(in) :: iotype ! Allowable values: ’LISTDIRECTED’,’NAMELIST’, or ’DT’
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(*), intent(inout) :: iomsg
    integer(ikind) i
    ! Requires
    call assert(this%user_defined(),error_message("mpi_object%output recieved unitialized object."))
      write (unit=unit,iostat=iostat,fmt="(i8,3(f12.4,2x))")
   end subroutine

  subroutine mpi_begin
  integer :: dims(1), periods(1), reorder
   if (program_status .eq. 0) then !prevent accidentally starting mpi when
   !already has been initiated
    call MPI_INIT(ierr) !initiate MPI
    call MPI_COMM_SIZE(MPI_COMM_WORLD, num_procs, ierr) !retrieve processer count
    call MPI_COMM_RANK(MPI_COMM_WORLD, my_id, ierr) !retrive processor rank
    dims = num_procs
    reorder = 1
    periods = 1
    call MPI_CART_CREATE(MPI_COMM_WORLD, 1, dims, periods, reorder, MPI_COMM_CART, ierr)
    call MPI_COMM_RANK(MPI_COMM_CART, my_id, ierr)
    call MPI_CART_SHIFT(MPI_COMM_CART, 0, 1, left_id, right_id, ierr)
    program_status=1
   endif
  end subroutine

  subroutine mpi_end
    if(program_status==1) then !prevent ending mpi if it is not running
     call MPI_FINALIZE(ierr)
     program_status=0
     endif
  end subroutine

  subroutine barrier
    call mpi_barrier(mpi_comm_world, ierr)
  end subroutine

  subroutine oned_message(periodic,local_grid_resolution,left_sub,right_sub)
  integer (ikind), intent(in)  :: local_grid_resolution
  real (rkind), intent(in), dimension(:) :: periodic ! keep track of global f
  real (rkind), intent(inout) ::left_sub,right_sub !images from nearby processors
  DOUBLE PRECISION left,right  !intermediate variable for storing messages
  !  assertions to ensure that proper input provided to subroutine
    call assert(size(periodic)>= local_grid_resolution,error_message("size of local function too small."))
    call assert(local_grid_resolution>0,error_message("invalid local grid spacing."))

    if (num_procs >1) then !no need to communicate if only 1 processor
      call MPI_SENDRECV(periodic(1),1,MPI_DOUBLE_PRECISION,left_id,0, &
                      right,1,MPI_DOUBLE_PRECISION,right_id,0,MPI_COMM_CART,status,ierr)
      call MPI_SENDRECV(periodic(local_grid_resolution),1,MPI_DOUBLE_PRECISION,right_id,0, &
                      left,1,MPI_DOUBLE_PRECISION,left_id,0,MPI_COMM_CART,status,ierr)
      left_sub = left
      right_sub = right
    else!incase only one processor
      left_sub = periodic(local_grid_resolution)
      right_sub = periodic(1)
    endif

end subroutine

end module
