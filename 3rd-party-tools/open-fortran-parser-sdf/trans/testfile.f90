program main
  implicit none
  real, allocatable :: f(:)[:],df_dx(:)
  sync all
  error stop "oops"
end program
