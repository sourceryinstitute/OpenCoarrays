program main
  use global_field_module, only : global_field
  implicit none
  type(global_field) :: T,laplacian_T,T_half
  real, parameter :: alpha=1.,dt=0.0001,final_time=1.,tolerance=1.E-3
  real :: time=0.
  call T%global_field_(internal_values=0.,boundary_values=[1.,0.],domain=[0.,1.],num_global_points=16384)
  call T_half%global_field_()
  do while(time<final_time)
    T_half = T + (.laplacian.T)*(alpha*dt/2.)
    T      = T + (.laplacian.T_half)*(alpha*dt)
    time = time + dt
  end do
  call laplacian_T%global_field_()
  laplacian_T = .laplacian.T
  if (any(laplacian_T%state()>tolerance)) error stop "Test failed."
  if (this_image()==1) print *,"Test passed."
end program
