module mpi_share
  !module exclusively for initiating and sharing an mpi_object 
  use mpi_module, only :mpi_class
  type (mpi_class) :: mpi_object
end module
