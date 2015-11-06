module accelerated_module
  use iso_c_binding, only : c_double,c_float,c_int
  implicit none
  
  private
  public :: co_dot_accelerated
  public :: co_dot_unaccelerated
  public :: CUDA,OpenACC,OpenMP
  public :: walltime

  ! Explicit interfaces for procedures that wrap accelerated kernels
  interface  

     ! This wrapper exploits the OpenCoarrays acceleration support
     subroutine cudaDot(a,b,partial_dot,n) bind(C, name="cudaDot")
       use iso_c_binding, only : c_float,c_int
       real(c_float) :: a(*),b(*)
       real(c_float) :: partial_dot
       integer(c_int),value :: n
     end subroutine

    function WALLTIME() bind(C, name = "WALLTIME")
       use iso_fortran_env
       real(real64) :: WALLTIME
    end function WALLTIME

  end interface

  enum, bind(C) 
    enumerator CUDA,OpenACC,OpenMP
  end enum

contains

  ! This parallel collective dot product uses no acceleration
  subroutine co_dot_unaccelerated(x,y,x_dot_y)
     real(c_float), intent(in) :: x(:),y(:)
     real(c_float), intent(out) :: x_dot_y
     x_dot_y = dot_product(x,y) ! Call Fortran intrinsic dot product on the local data
     call co_sum(x_dot_y) ! Call Fortarn 2015 collective sum
  end subroutine 

  ! Exploit the OpenCoarrays support for a accelerated dot products 
  ! using any one of several acceleration APIs: OpenACC, CUDA, OpenMP 4.0, etc.
  ! On heterogeneous platforms, the API choice can vary in space (e.g., from one image/node to the 
  ! next) or in time (e.g., based on dynamic detection of the hardware or network behavior).
  subroutine co_dot_accelerated(x,y,x_dot_y,API)
     real, intent(in) :: x(:),y(:) 
     real, intent(out) :: x_dot_y     
     integer(c_int), intent(in), optional :: API
     integer(c_int) :: chosen_API

     if (present(API)) then
       chosen_API = API
     else
       chosen_API = CUDA
     end if

     select case(chosen_API)
       case(CUDA)
         call cudaDot(x,y,x_dot_y,size(x)) ! Accelerated reduction on local data
       case(OpenMP)
         error stop "OpenMP acceleration not yet implemented."
       case(OpenACC)
         error stop "OpenACC acceleration not yet implemented."
       case default
         error stop "Invalid acceleration API choice."
     end select
     call co_sum(x_dot_y) ! Fortran 2015 coarray collective
  end subroutine 

end module

program cu_dot_test
  use iso_c_binding, only : c_double,c_float,c_int
  implicit none

  ! Unaccelerated variables 
  real(c_float), allocatable :: a(:),b(:)
  real(c_float), allocatable :: a_unacc(:),b_unacc(:)
  real(c_float) :: dot
  real(c_double) :: t_start, t_end

  ! Library-accelerated variables (these are corarrays to facilitate a scatter operation)
  real(c_float), allocatable :: a_acc(:)[:], b_acc(:)[:]
  real(c_float) :: dot_acc[*]

  integer(c_int) :: n = 99999904
  integer(c_int) :: n_local,np,me

  np = num_images()
  me = this_image()

  if (mod(n,np)/=0) error stop "n is not evenly divisible by num_images()"
  n_local = n/np

  call initialize_all_variables
  sync all

  block 
!    use accelerated_module, only : co_dot_accelerated,co_dot_unaccelerated,CUDA,walltime
    use accelerated_module

    !Parallel execution
    t_start = walltime()
    call co_dot_accelerated(a_acc(1:n_local),b_acc(1:n_local),dot_acc,CUDA)
    t_end = walltime()
    if(me==1) print *, 'Accelerated dot_prod',dot_acc,'time:',t_end-t_start
  
    sync all

    !Serial execution
    t_start = walltime()
    call co_dot_unaccelerated(a_unacc(1:n_local),b_unacc(1:n_local),dot)  
    t_end = walltime()
    if(me==1) print *, 'Serial result',dot,'time:',t_end-t_start

    sync all

  end block

contains

  subroutine initialize_all_variables()
    use opencoarrays,  only : accelerate
    integer(c_int) :: i
    ! These allocation arguments must be coarrays to support the scatter operation below
    allocate(a_acc(n_local)[*],b_acc(n_local)[*])
    ! These allocation arguments will be defined locally and therefore need not be coarrays
    allocate(a_unacc(n_local),b_unacc(n_local))

    ! Register the desired variables for acceleration
    call accelerate(a_acc)
    call accelerate(b_acc)
 
    if(me == 1) then
      ! Initialize the local unaccelerated data on every image 
      b = [(1.,i=1,n)]
      ! For even n, a is orthogonal to b
      a = [((-1.)**i,i=1,n)] 
      ! Scatter a and b to a_cc and b_cc
      do i=1,np
        a_acc(1:n_local)[i] = a(n_local*(i-1)+1:n_local*i)
        b_acc(1:n_local)[i] = b(n_local*(i-1)+1:n_local*i)
      end do
    endif
    sync all
    a_unacc=a_acc
    b_unacc=b_acc
  end subroutine

end program
