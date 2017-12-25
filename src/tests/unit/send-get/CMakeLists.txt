caf_compile_executable(get_array get_array_test.f90)

#caf_compile_executable(get old_get_array_test.f90)
#target_link_libraries(is_this_still_needed OpenCoarrays)

## Pure get() tests
caf_compile_executable(get_self sameloc.f90)
caf_compile_executable(get_convert_nums get_convert_nums.f90)
caf_compile_executable(get_convert_char_array get_convert_char_array.f90)
caf_compile_executable(get_with_offset_1d get_with_offset_1d.f90)
caf_compile_executable(whole_get_array whole_get_array.f90)
caf_compile_executable(strided_get strided_get.f90)
caf_compile_executable(get_with_vector_index get_with_vector_index.f90)
caf_compile_executable(alloc_comp_get_convert_nums alloc_comp_get_convert_nums.f90)

## Pure send() tests
caf_compile_executable(send_array send_array_test.f90)
caf_compile_executable(send_with_vector_index send_with_vector_index.f90)

# Pure sendget() tests
caf_compile_executable(sendget_convert_char_array sendget_convert_char_array.f90)
caf_compile_executable(send_convert_char_array send_convert_char_array.f90)
#caf_compile_executable(sendget_with_vector_index sendget_with_vector_index.f90)
caf_compile_executable(strided_sendget strided_sendget.f90)
set_target_properties(build_strided_sendget
  PROPERTIES MIN_IMAGES 3)

if (gfortran_compiler AND (NOT CMAKE_Fortran_COMPILER_VERSION VERSION_LESS 7.3.0))
  caf_compile_executable(sendget_convert_nums sendget_convert_nums.f90)
  caf_compile_executable(send_convert_nums send_convert_nums.f90)
elseif((CAF_RUN_DEVELOPER_TESTS OR $ENV{OPENCOARRAYS_DEVELOPER}))
  message( AUTHOR_WARNING
    "Skipping building the following tests due to GFortran < 7.3.0 lack of compatibility:
    sendget_convert_nums.f90
    send_convert_nums.f90"
  )

endif()

