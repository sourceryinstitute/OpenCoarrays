add_executable(convert-before-put issue-292-convert-type-before-put.f90)
target_link_libraries(convert-before-put OpenCoarrays)

add_executable(event-post issue-293-silent-event-failure.F90)
target_link_libraries(event-post OpenCoarrays)

add_executable(source-alloc-sync issue-243-source-allocation-no-sync.f90)
target_link_libraries(source-alloc-sync OpenCoarrays)

