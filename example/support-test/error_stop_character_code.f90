program error_stop_character_code
  implicit none

  error stop ""

  stop 0 ! ../../test/error_stop_test.f90 will report a test failure if this line runs
end program 
