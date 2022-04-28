module co_min_test
    use garden, only: result_t, test_item_t, assert_equals, describe, it, assert_that, assert_equals

    implicit none
    private
    public :: test_co_min

contains
    function test_co_min() result(tests)
        type(test_item_t) tests
    
        tests = describe( &
          "The co_min subroutine computes the minimum", &
          [ it("default integer scalar with stat argument present", min_default_integer_scalars) &
           ,it("integer(c_int64_t) scalar with no optional arguments present", min_c_int64_scalars) &
           ,it("default integer 1D array elements with no optional arguments present", min_default_integer_1D_array) &
           ,it("default integer 7D array elements with stat argument present", min_default_integer_7D_array) &
           ,it("default real scalars with stat argument present", min_default_real_scalars) &
           ,it("double precision 2D array elements with no optional arguments present", min_double_precision_2D_array) &
           ! _______ Character type not yet supported _________
           !,it("length-5 string with no optional arguments", &
           !alphabetically_1st_scalar_string) &
           !,it("elements across images with 2D arrays of strings", min_elements_in_2D_string_arrays) &
        ])
    end function

    function min_default_integer_scalars() result(result_)
        type(result_t) result_
        integer i, status_
 
        status_ = -1
        i = -this_image()
        call co_min(i, stat=status_)
        result_ = assert_equals(-num_images(), i) .and. assert_equals(0, status_)
    end function

    function min_c_int64_scalars() result(result_)
        use iso_c_binding, only : c_int64_t 
        type(result_t) result_
        integer(c_int64_t) i
 
        i = this_image()
        call co_min(i)
        result_ = assert_equals(1, int(i))
    end function

    function min_default_integer_1D_array() result(result_)
        type(result_t) result_
        integer i
        integer, allocatable :: array(:)
 
        associate(sequence_ => this_image()*[(i, i=1, num_images())])
          array = sequence_
          call co_min(array)
          associate(min_sequence => [(i, i=1, num_images())])
            result_ = assert_that(all(min_sequence == array))
          end associate
        end associate
    end function

    function min_default_integer_7D_array() result(result_)
        type(result_t) result_
        integer array(2,1,1, 1,1,1, 2), status_
 
        status_ = -1
        array = 3 - this_image()
        call co_min(array, stat=status_)
        result_ = assert_that(all(array == 3 - num_images())) .and. assert_equals(0, status_)
    end function

    function min_default_real_scalars() result(result_)
        type(result_t) result_
        real scalar
        real, parameter :: pi = 3.141592654
        integer status_

        status_ = -1
        scalar = -pi*this_image()
        call co_min(scalar, stat=status_)
        result_ = assert_equals(-dble(pi*num_images()), dble(scalar) ) .and. assert_equals(0, status_)
    end function

    function min_double_precision_2D_array() result(result_)
        type(result_t) result_
        double precision, allocatable :: array(:,:)
        double precision, parameter :: tent(*,*) = dble(reshape(-[0,1,2,3,2,1], [3,2]))
 
        array = tent*dble(this_image())
        call co_min(array)
        result_ = assert_that(all(array==tent*num_images()))
    end function

    function min_elements_in_2D_string_arrays() result(result_)
      type(result_t) result_
      character(len=*), parameter :: script(*) = &
        [character(len=len("the question.")) :: "To be ","or not"," to ","be.","  That is ","the question."]
      character(len=len(script)), dimension(3,2) :: scramlet, co_min_scramlet
      integer i, cyclic_permutation(size(script))
      
      associate(me => this_image())
        associate(cyclic_permutation => [(1 + mod(i-1,size(script)), i=me, me+size(script) )])
          scramlet = reshape(script(cyclic_permutation), shape(scramlet))
        end associate
      end associate

      co_min_scramlet = scramlet
      call co_min(co_min_scramlet, result_image=1)

      block 
        integer j, delta_j
        character(len=len(script)) expected_script(size(script)), expected_scramlet(size(scramlet,1),size(scramlet,2))

        do j=1, size(script)
          expected_script(j) = script(j)
          do delta_j = 1, min(num_images()-1, size(script))
            associate(periodic_index => 1 + mod(j+delta_j-1, size(script)))
              expected_script(j) = min(expected_script(j), script(periodic_index))
            end associate
          end do
        end do
        expected_scramlet = reshape(expected_script, shape(scramlet))

        result_ =  assert_that(all(scramlet == co_min_scramlet),"all(scramlet == co_min_scramlet)")
      end block
      
    end function

    function alphabetically_1st_scalar_string() result(result_)
      type(result_t) result_
      character(len=*), parameter :: words(*) = [character(len=len("to party!")):: "Loddy","doddy","we","like","to party!"]
      character(len=:), allocatable :: my_word

      associate(me => this_image())
        associate(periodic_index => 1 + mod(me-1,size(words)))
          my_word = words(periodic_index)
          call co_min(my_word)
        end associate
      end associate

      associate(expected_word => minval(words(1:min(num_images(), size(words)))))
        result_ = assert_equals(expected_word, my_word)
      end associate
    end function

end module co_min_test
