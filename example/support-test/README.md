Test Support
------------
The programs in this directory intentionally error-terminate to support the `error stop` tests in `test/caf_error_stop.f90`, which use Fortran's `execute_command_line` to run the programms in this directory and check for the expected non-zero stop codes.  Running the tests in this manner enables the Vegetable tests to continue executing in the presence of error-terminating tests.
