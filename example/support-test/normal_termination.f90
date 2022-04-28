program normal_termination
  implicit none

  stop 0

  stop 1 ! opencoarrays/test/zzz_finalization_test.f90 reports a failure if this line runs
end program 
