program error_stop_integer_code
  implicit none

  error stop

  stop 0 ! caffeine/test/error_stop_test.f90 reports a failure if this line runs
end program 
