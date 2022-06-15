module num_images_test
  use garden, only: result_t, test_item_t, assert_that, describe, it

  implicit none
  private
  public :: test_num_images

contains
  function test_num_images() result(tests)
    type(test_item_t) :: tests

    tests = &
      describe( &
        "The num_images function result", &
        [ it("is a valid number of images when invoked with no arguments", check_num_images_valid) &
      ])
  end function

  function check_num_images_valid() result(result_)
    type(result_t) :: result_
    result_ = assert_that(num_images()>0, "positive number of images")
  end function

end module num_images_test
