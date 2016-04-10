program main
  implicit none
  real, allocatable :: f(:)[:],df_dx(:)
  sync images(neighbors)
  sync all
  error stop "oops"
end program
