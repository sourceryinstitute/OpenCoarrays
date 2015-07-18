add_executable(co_sum_test co_sum.F90)
target_link_libraries(co_sum_test OpenCoarrays)

add_executable(co_broadcast_test co_broadcast.F90)
target_link_libraries(co_broadcast_test OpenCoarrays)

add_executable(co_min_test co_min.F90)
target_link_libraries(co_min_test OpenCoarrays)

add_executable(co_max_test co_max.F90)
target_link_libraries(co_max_test OpenCoarrays)

add_executable(co_reduce_test co_reduce.F90)
target_link_libraries(co_reduce_test OpenCoarrays)
