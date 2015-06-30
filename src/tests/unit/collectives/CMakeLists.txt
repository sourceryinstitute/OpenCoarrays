add_executable(co_sum co_sum_test.F90)
target_link_libraries(co_sum OpenCoarrays)

add_executable(co_broadcast co_broadcast_tests.F90)
target_link_libraries(co_broadcast OpenCoarrays)
