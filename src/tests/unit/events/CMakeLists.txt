# Fortran tests

add_executable(event_post_1 event_post_1.f90)
target_link_libraries(event_post_1 OpenCoarrays)

add_executable(event_post_2 event_post_2.f90)
target_link_libraries(event_post_2 OpenCoarrays)

