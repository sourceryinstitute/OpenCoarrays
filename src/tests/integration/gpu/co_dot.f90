module accelerated_module
  use iso_c_binding, only : c_double,c_float,c_int
  implicit none
  
  private
  public :: co_dot_accelerated
  public :: co_dot_unaccelerated
  public :: co_dot_manually_accelerated
  public :: co_dot_mapped_manually_accelerated
  public :: CUDA,OpenACC,OpenMP
  public :: walltime

  ! Explicit interfaces for procedures that wrap accelerated kernels
  interface  

     ! This is the wrapper a programmer would have to write today to manually accelerate calculations
     subroutine manual_cudaDot(a,b,partial_dot,n,img) bind(C, name="manual_cudaDot")
       use iso_c_binding, only : c_float,c_int
       real(c_float) :: a(*),b(*)
       real(c_float) :: partial_dot
       integer(c_int),value :: n
       integer(c_int),value :: img
     end subroutine

     subroutine manual_mapped_cudaDot(a,b,partial_dot,n,img) bind(C, name="manual_mapped_cudaDot")
       use iso_c_binding, only : c_float,c_int
       real(c_float) :: a(*),b(*)
       real(c_float) :: partial_dot
       integer(c_int),value :: n
       integer(c_int),value :: img
     end subroutine

     ! This wrapper exploits the OpenCoarrays acceleration support and is therefore simpler
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

  ! This parallel collective dot product uses manual acceleration
  subroutine co_dot_manually_accelerated(x,y,x_dot_y)
     real(c_float), intent(in) :: x(:),y(:)
     real(c_float), intent(out) :: x_dot_y
     call manual_cudaDot(x,y,x_dot_y,size(x),this_image()-1) 
     call co_sum(x_dot_y) ! Call Fortarn 2015 collective sum
  end subroutine 

  subroutine co_dot_mapped_manually_accelerated(x,y,x_dot_y)
     real(c_float), intent(in) :: x(:),y(:)
     real(c_float), intent(out) :: x_dot_y
     call manual_mapped_cudaDot(x,y,x_dot_y,size(x),this_image()-1)
     call co_sum(x_dot_y) ! Call Fortarn 2015 collective sum
  end subroutine

  ! Exploit the OpenCoarrays support for a accelerated dot products 
  ! using any one of several acceleration APIs: OpenACC, CUDA, OpenMP 4.0, etc.
  ! On heterogeneous platforms, the API choice can vary in space (e.g., from one image/node to the 
  ! next) or in time (e.g., based on dynamic detection of the hardware or network behavior).
  subroutine co_dot_accelerated(x,y,x_dot_y,API)
     real, intent(in) :: x(:),y(:) 
     real, intent(out) :: x_dot_y     
     integer(c_int), intent(in) :: API
     select case(API)
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
  real(c_float) :: dot
  real(c_double) :: t_start, t_end

  ! Compiler/library-accelerated variables
  real(c_float), allocatable :: a_acc(:)[:], b_acc(:)[:]
  real(c_float) :: dot_acc[*]

  ! Manually accelerated variables
  real(c_float), allocatable :: a_man(:)[:], b_man(:)[:]
  real(c_float) :: dot_man[*]

  integer(c_int),parameter :: n = 99900000
  integer(c_int) :: n_local,np,me

  np = num_images()
  me = this_image()

  if (mod(n,np)/=0) error stop "n is not evenly divisible by num_images()"
  n_local = n/np

  call initialize_all_variables
  sync all

  block 
!    use accelerated_module, only : co_dot_accelerated,co_dot_unaccelerated,co_dot_manually_accelerated,CUDA,walltime,co_dot_mapped_manually_accelerated
    use accelerated_module

    !Parallel execution
    t_start = walltime()
    call co_dot_accelerated(a_acc,b_acc,dot_acc,CUDA)
    t_end = walltime()
    if(me==1) print *, 'Accelerated dot_prod',dot_acc,'time:',t_end-t_start
  
    sync all

    t_start = walltime()
    call co_dot_manually_accelerated(a_man,b_man,dot_man)
    t_end = walltime()
    if(me==1) print *, 'Manually accelerated dot_prod',dot_man,'time:',t_end-t_start

    sync all

    !Serial execution
    t_start = walltime()
    call co_dot_unaccelerated(a_man,b_man,dot)  
    t_end = walltime()
    if(me==1) print *, 'Serial result',dot,'time:',t_end-t_start

    sync all

    t_start = walltime()
    call co_dot_mapped_manually_accelerated(a_man,b_man,dot)
    t_end = walltime()
    if(me==1) print *, 'Manually mapped',dot,'time:',t_end-t_start
  end block

contains

  subroutine initialize_all_variables()
    integer(c_int) :: i
    call accelerated_allocate(a_acc(n_local)[*],b_acc(n_local)[*])
    call accelerated_allocate(a_man(n_local)[*],b_man(n_local)[*])
 
    if(me == 1) then
      ! Initialize the local unaccelerated data on every image 
      b = [(1.,i=1,n)]
      ! For even n, a is orthogonal to b
      a = [((-1.)**i,i=1,n)] 
      ! Scatter a and b to a_cc and b_cc
      do i=1,np
        a_acc(1:n_local)[i] = a(n_local*(i-1)+1:n_local*i)
        a_man(1:n_local)[i] = a(n_local*(i-1)+1:n_local*i)
        b_acc(1:n_local)[i] = b(n_local*(i-1)+1:n_local*i)
        b_man(1:n_local)[i] = b(n_local*(i-1)+1:n_local*i)
      enddo
    endif
  end subroutine

end program
