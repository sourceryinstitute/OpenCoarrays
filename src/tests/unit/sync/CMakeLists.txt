add_executable(syncall syncall.f90)
target_link_libraries(syncall OpenCoarrays)

add_executable(syncimages syncimages.f90)
target_link_libraries(syncimages OpenCoarrays)

add_executable(duplicate_syncimages duplicate_syncimages.f90)
target_link_libraries(duplicate_syncimages OpenCoarrays)

#add_executable(syncimages_status syncimages_status.f90)
#target_link_libraries(syncimages_status OpenCoarrays)

#add_executable(send_array send_array_test.f90)
#target_link_libraries(send_array OpenCoarrays)

#add_executable(get_with_offset_1d get_with_offset_1d.f90)
#target_link_libraries(get_with_offset_1d OpenCoarrays)

#add_executable(whole_get_array whole_get_array.f90)
#target_link_libraries(whole_get_array OpenCoarrays)

#add_executable(strided_get strided_get.f90)
#target_link_libraries(strided_get OpenCoarrays)
