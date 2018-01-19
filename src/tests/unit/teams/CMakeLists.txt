add_executable(team_number team-number.f90)
target_link_libraries(team_number OpenCoarrays)
#TODO: Remove when gfortran supports team_number()
target_include_directories(team_number PRIVATE ${CMAKE_BINARY_DIR}/include)

#add_executable(get_communicator get-communicator.f90)
#target_link_libraries(get_communicator OpenCoarrays)
#TODO: Remove when gfortran supports team_number()
#target_include_directories(get_communicator PRIVATE ${CMAKE_BINARY_DIR}/include)
