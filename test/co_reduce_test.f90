module co_reduce_test
  use garden, only : result_t, test_item_t, assert_equals, describe, it, assert_that, assert_equals
  use assert_m, only : assert
  use iso_c_binding, only : c_bool, c_char, c_double, c_int64_t

  implicit none
  private
  public :: test_co_reduce

contains

  function test_co_reduce() result(tests)
    type(test_item_t) tests
  
    tests = describe( &
      "The co_reduce subroutine", &
      [ it("sums default integer scalars with no optional arguments present", sum_default_integer_scalars) & 
       ,it("sums integer(c_int64_t) scalars with no optional arguments present", sum_c_int64_t_scalars) &
       ,it("multiplies default real scalars with all optional arguments present", multiply_default_real_scalars) &
       ,it("multiplies real(c_double) scalars with all optional arguments present", multiply_c_double_scalars) &
       ,it("performs a collective .and. operation across logical scalars", reports_on_consensus) &
       ,it("sums default integer elements of a 2D array across images", sum_integer_array_elements) &
      ! _______ Complex type not yet supported _________
      !,it("sums default complex scalars with a stat-variable present", sum_default_complex_scalars) &
      !,it("sums complex(c_double) scalars with a stat-variable present", sum_complex_c_double_scalars) &
      ! _______ Character type not yet supported _________
      !,it("finds the alphabetically first length-5 string with result_image present", alphabetically_1st_size1_string_array) & 
    ])
  end function

  function alphabetically_1st_size1_string_array() result(result_)
    type(result_t) result_
    character(len=*, kind=c_char), parameter :: names(*) = ["larry","harry","carey","betty","tommy","billy"]
    character(len=:, kind=c_char), allocatable :: my_name(:)

    associate(me => this_image())
      associate(periodic_index => 1 + mod(me-1,size(names)))
        my_name = [names(periodic_index)]
        call co_reduce(my_name, alphabetize)
      end associate
    end associate

    associate(expected_name => minval(names(1:min(num_images(), size(names)))))
      result_ = assert_that(all(expected_name == my_name))
    end associate

  contains

    pure function alphabetize(lhs, rhs) result(first_alphabetically)
      character(len=*), intent(in) :: lhs, rhs
      character(len=len(lhs)) first_alphabetically
      call assert(len(lhs)==len(rhs), "co_reduce_s alphabetize: LHS/RHS length match", lhs//" , "//rhs)
      first_alphabetically = min(lhs,rhs)
    end function

  end function

  function sum_integer_array_elements() result(result_)
    type(result_t) result_
    integer status_
    integer, parameter :: input_array(*,*) = reshape([1, 2, 3, 4], [2, 2])
    integer array(2,2)

    array = input_array
    call co_reduce(array, add)
    result_ = assert_that(all(num_images()*input_array==array))

  contains

    pure function add(lhs, rhs) result(total)
      integer, intent(in) :: lhs, rhs
      integer total
      total = lhs + rhs 
    end function

  end function

  function sum_complex_c_double_scalars() result(result_)
    type(result_t) result_
    integer status_
    complex(c_double) z
    complex(c_double), parameter :: z_input=(1._c_double, 1._c_double)

    z = z_input
    call co_reduce(z, add_complex, stat=status_)
    result_ = assert_equals(real(num_images()*z_input, c_double), real(z, c_double)) .and. assert_equals(0, status_)

  contains

    pure function add_complex(lhs, rhs) result(total)
      complex(c_double), intent(in) :: lhs, rhs
      complex(c_double) total
      total = lhs + rhs 
    end function

  end function

  function sum_default_complex_scalars() result(result_)
    type(result_t) result_
    integer status_
    complex z
    complex, parameter :: z_input=(1.,1.)

    z = z_input
    call co_reduce(z, add_complex, stat=status_)
    result_ = assert_equals(dble(num_images()*z_input), dble(z)) .and. assert_equals(0, status_)

  contains

    pure function add_complex(lhs, rhs) result(total)
      complex, intent(in) :: lhs, rhs
      complex total
      total = lhs + rhs 
    end function

  end function

  function sum_default_integer_scalars() result(result_)
    type(result_t) result_
    integer i

    i = 1
    call co_reduce(i, add)
    result_ = assert_equals(num_images(), i)

  contains

    pure function add(lhs, rhs) result(total)
      integer, intent(in) :: lhs, rhs
      integer total
      total = lhs + rhs 
    end function

  end function

  function sum_c_int64_t_scalars() result(result_)
    type(result_t) result_
    integer(c_int64_t) i

    i = 1_c_int64_t
    call co_reduce(i, add_integers)
    result_ = assert_that(int(num_images(), c_int64_t)==i)

  contains

    pure function add_integers(lhs, rhs) result(total)
      integer(c_int64_t), intent(in) :: lhs, rhs
      integer(c_int64_t) total
      total = lhs + rhs 
    end function

  end function

  function reports_on_consensus() result(result_)
    type(result_t) result_
    logical(c_bool) one_false, one_true, all_true

    one_false = merge(.false., .true., this_image()==1)
    call co_reduce(one_false, and)

    one_true = merge(.true., .false., this_image()==1)
    call co_reduce(one_true, and)
 
    all_true = .true.
    call co_reduce(all_true, and)

    result_ = assert_that(one_false .eqv. .false.) .and. &
              assert_that(one_true .eqv. merge(.true.,.false.,num_images()==1)) .and. &
              assert_that(all_true .eqv. .true.)
  contains

    pure function and(lhs, rhs) result(lhs_and_rhs)
      logical(c_bool), intent(in) :: lhs, rhs
      logical(c_bool) lhs_and_rhs
      lhs_and_rhs = lhs .and. rhs 
    end function

  end function

  function multiply_c_double_scalars() result(result_)
    type(result_t) result_
    real(c_double) p
    integer j, status_
    character(len=:), allocatable :: error_message

    error_message = "unused"
    associate(me => this_image())
      p = real(me,c_double)
      call co_reduce(p, multiply_doubles, result_image=1, stat=status_, errmsg=error_message)
      associate(expected_result => merge( product([(real(j,c_double), j = 1, num_images())]), real(me,c_double), me==1 ))
        result_ = &
          assert_equals(expected_result, real(p,c_double)) .and. &
          assert_equals(0, status_) .and. &
          assert_equals("unused", error_message)
      end associate
    end associate

  contains

    pure function multiply_doubles(lhs, rhs) result(product_)
      real(c_double), intent(in) :: lhs, rhs
      real(c_double) product_
      product_ = lhs * rhs 
    end function

  end function

  function multiply_default_real_scalars() result(result_)
    type(result_t) result_
    real p
    integer j, status_
    character(len=:), allocatable :: error_message

    error_message = "unused"
    associate(me => this_image())
      p = real(me)
      call co_reduce(p, multiply, result_image=1, stat=status_, errmsg=error_message)
      associate(expected_result => merge( product([(dble(j), j = 1, num_images())]), dble(me), me==1 ))
        result_ = &
          assert_equals(expected_result, dble(p)) .and. &
          assert_equals(0, status_) .and. &
          assert_equals("unused", error_message)
      end associate
    end associate

  contains

    pure function multiply(lhs, rhs) result(product_)
      real, intent(in) :: lhs, rhs
      real product_
      product_ = lhs * rhs 
    end function

  end function
end module co_reduce_test
