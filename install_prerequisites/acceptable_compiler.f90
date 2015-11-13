program main
  use iso_fortran_env, only : compiler_version
  implicit none
  print *,compiler_version() >= " GCC version 5.1.0"
end program
