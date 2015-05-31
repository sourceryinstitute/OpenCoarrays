add_executable(initmpi1 initmpi1.f90)
target_link_libraries(initmpi1 OpenCoarrays)

add_executable(register1 register1.f90)
target_link_libraries(register1 OpenCoarrays)

add_executable(register2 register2.f90)
target_link_libraries(register2 OpenCoarrays)

add_executable(register3 register3.f90)
target_link_libraries(register3 OpenCoarrays)
