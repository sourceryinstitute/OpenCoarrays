add_executable(team_number team-number.f90)
target_link_libraries(team_number OpenCoarrays)
target_include_directories(team_number PRIVATE ${CMAKE_BINARY_DIR}/mod)
