module error_stop_test
    use garden, only: test_item_t, describe, result_t, it, assert_that

    implicit none
    private
    public :: test_this_image

contains
    function test_this_image() result(tests)
        type(test_item_t) :: tests

        tests = describe( &
                "A program that executes the error_stop function", &
                [ it("exits with a non-zero exitstat when stop code is an integer", check_integer_stop_code) &
                 ,it("exits with a non-zero exitstat when stop code is an character", check_character_stop_code) &
                ])
    end function

    function check_integer_stop_code() result(result_)
        type(result_t) :: result_
        integer exit_status

        call execute_command_line( &
          command = "./build/run-fpm.sh run --example error_stop_integer_code > /dev/null 2>&1", &
          wait = .true., &
          exitstat = exit_status &
        )   
        result_ = assert_that(exit_status /= 0)

    end function

    function check_character_stop_code() result(result_)
        type(result_t) :: result_
        integer exit_status

        call execute_command_line( &
          command = "./build/run-fpm.sh run --example error_stop_character_code > /dev/null 2>&1", &
          wait = .true., &
          exitstat = exit_status &
        )   
        result_ = assert_that(exit_status /= 0)

    end function

end module error_stop_test
