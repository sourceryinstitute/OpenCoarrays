add_executable(get_array get_array_test.f90)
target_link_libraries(get_array OpenCoarrays)

#add_executable(get old_get_array_test.f90)
#target_link_libraries(is_this_still_needed OpenCoarrays)

add_executable(get_self sameloc.f90)
target_link_libraries(get_self OpenCoarrays)

add_executable(send_array send_array_test.f90)
target_link_libraries(send_array OpenCoarrays)

add_executable(get_with_offset_1d get_with_offset_1d.f90)
target_link_libraries(get_with_offset_1d OpenCoarrays)

add_executable(whole_get_array whole_get_array.f90)
target_link_libraries(whole_get_array OpenCoarrays)

add_executable(strided_get strided_get.f90)
target_link_libraries(strided_get OpenCoarrays)
