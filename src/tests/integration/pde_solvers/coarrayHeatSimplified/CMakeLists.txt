add_executable(co_heat
  main.f90
  global_field.f90
  local_field.f90
)
target_link_libraries(co_heat OpenCoarrays)
