module shared
  !module for sharing mpi functionality/variables with other modules/main
  use mpi !non-native mpi functionality
  integer :: tag, status(MPI_STATUS_SIZE)
  integer :: MPI_COMM_CART
  integer :: my_id, num_procs, ierr, local_grid_resolution, left_id, right_id
  integer, parameter :: send_data_tag = 2001, return_data_tag = 2002 !tags for sending information
  integer, parameter :: root_process = 0
  integer, parameter :: max_local_resolution = 10000
end module
