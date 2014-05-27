! This code tests the transpose routines used in Fourier-spectral simulations of homogeneous turbulence.
! The data is presented to the physics routines as groups of y-z or x-z planes distributed among the images.
! The (out-of-place) transpose routines do the x <--> y transposes required and consist of transposes within
! data blocks (intra-image) and a transpose of the distribution of these blocks among the images (inter-image).
! Two methods are tested here:
!   RECEIVE: receive block from other image and transpose it
!   SEND:    transpose block and send it to other image

!==================  test transposes with integer x,y,z values  ===============================

module run_size
    implicit none
        integer(8) :: nx, ny, nz
        integer(8) :: my, mx, first_y, last_y, first_x, last_x
        integer(8) :: my_node, num_nodes
        real(8) :: tran_time

interface
   function WALLTIME() bind(C, name = "WALLTIME")
       real(8) :: WALLTIME
   end function WALLTIME
end interface

contains

subroutine copy3( A,B, n1, sA1, sB1, n2, sA2, sB2, n3, sA3, sB3 )
  implicit none
  complex, intent(in)  :: A(0:*)
  complex, intent(out) :: B(0:*)
  integer(8), intent(in) :: n1, sA1, sB1
  integer(8), intent(in) :: n2, sA2, sB2
  integer(8), intent(in) :: n3, sA3, sB3
  integer(8) i,j,k

  do k=0,n3-1
     do j=0,n2-1
        do i=0,n1-1
           B(i*sB1+j*sB2+k*sB3) = A(i*sA1+j*sA2+k*sA3)
        end do
     end do
  end do
end subroutine copy3

end module run_size

program zzz
  !(***********************************************************************************************************
  !                   m a i n   p r o g r a m
  !***********************************************************************************************************)
      use run_size
      implicit none
      include 'mpif.h'
      
      complex, allocatable ::  u(:,:,:,:)    ! u(nz,4,first_x:last_x,ny)    !(*-- ny = my * num_nodes --*)
      complex, allocatable ::  ur(:,:,:,:)   !ur(nz,4,first_y:last_y,nx/2)  !(*-- nx/2 = mx * num_nodes --*)
      complex, allocatable :: bufr(:)

      integer(8) :: x, y, z, msg_size, iter
      integer(8) :: ierror

  call MPI_INIT(ierror)
  call MPI_COMM_RANK(MPI_COMM_WORLD, my_node, ierror)
  call MPI_COMM_SIZE(MPI_COMM_WORLD, num_nodes, ierror)

      if( my_node == 0 ) then
        write(6,fmt="(A)") "nx,ny,nz : "
        read(5,*) nx, ny, nz
      end if

        call MPI_BARRIER(MPI_COMM_WORLD, ierror)  !-- other nodes wait for broadcast!
        call MPI_BCAST( nx, 1, MPI_INT, 0,MPI_COMM_WORLD, ierror )
        call MPI_BCAST( ny, 1, MPI_INT, 0,MPI_COMM_WORLD, ierror )
        call MPI_BCAST( nz, 1, MPI_INT, 0,MPI_COMM_WORLD, ierror )


 	  if ( mod(ny,num_nodes) == 0)  then;   my = ny / num_nodes
                                    else;   write(6,*) "node ", my_node, " ny not multiple of num_nodes";     stop
      end if

      if ( mod(nx/2,num_nodes) == 0)  then;   mx = nx/2 / num_nodes
                                    else;   write(6,*) "node ", my_node, "nx/2 not multiple of num_nodes";    stop
      end if

      first_y = my_node*my + 1;   last_y  = my_node*my + my
      first_x = my_node*mx + 1;   last_x  = my_node*mx + mx

      msg_size = nz*4*mx*my     !-- message size (complex data items

      allocate (  u(nz , 4 , first_x:last_x , ny)   )   !(*-- y-z planes --*)
      allocate ( ur(nz , 4 , first_y:last_y , nx/2) )   !(*-- x-z planes --*)
      allocate ( bufr(msg_size) )


!---------  initialize data u (mx y-z planes per image) ----------

        do x = first_x, last_x
            do y = 1, ny
                do z = 1, nz
                    u(z,1,x,y) = x
                    u(z,2,x,y) = y
                    u(z,3,x,y) = z
                end do
            end do
        end do

    tran_time = 0
    do iter = 1, 2  !--- 2 transform pairs per second-order time step

!---------  transpose data u -> ur (mx y-z planes to my x-z planes per image)  --------

      ur = 0
      call transpose_X_Y

!--------- test data ur (my x-z planes per image) ----------

         do x = 1, nx/2
            do y = first_y, last_y
                do z = 1, nz
                    if ( real(ur(z,1,y,x)) /= x .or. real(ur(z,2,y,x)) /= y .or. real(ur(z,3,y,x)) /= z )then
                         write(6,fmt="(A,i3,3(6X,A,f7.3,i4))") "transpose_X_Y failed:  image ", my_node &
                            , " X ",real(ur(z,1,y,x)),x, "  Y ",real(ur(z,2,y,x)),y, "  Z ", real(ur(z,3,y,x)),z
                        stop
                    end if
                end do
            end do
        end do

!---------  transpose data ur -> u (my x-z planes to mx y-z planes per image)  --------

      u = 0
      call transpose_Y_X

!--------- test data u (mx y-z planes per image) ----------

         do x = first_x, last_x
            do y = 1, ny
                do z = 1, nz
                    if ( real(u(z,1,x,y)) /= x .or. real(u(z,2,x,y)) /= y .or. real(u(z,3,x,y)) /= z )then
                         write(6,fmt="(A,i3,3(6X,A,f7.3,i4))") "transpose_Y_X failed:  image ", my_node &
                            , " X ",real(u(z,1,y,x)),x, "  Y ",real(u(z,2,y,x)),y, "  Z ", real(u(z,3,y,x)),z
                        stop
                    end if
                end do
            end do
        end do
    end do

call MPI_BARRIER(MPI_COMM_WORLD, ierror)

        if( my_node == 0 )  write(6,fmt="(A,f8.3)")  "test passed:  tran_time ", tran_time

    deallocate ( bufr, ur, u)

!=========================   end of main executable  =============================

contains 

!-------------   out-of-place transpose data_s --> data_r  ----------------------------

 subroutine transpose_X_Y

    use run_size
    implicit none

    integer(8) :: to, from, send_tag, recv_tag
    integer :: stage, idr(0:num_nodes-1), ids(0:num_nodes-1)
    integer(8) :: send_status(MPI_STATUS_SIZE), recv_status(MPI_STATUS_SIZE)
    character*(MPI_MAX_ERROR_STRING) errs

    call MPI_BARRIER(MPI_COMM_WORLD, ierror)   !--  wait for other nodes to finish compute
    tran_time = tran_time - WALLTIME()

!--------------   transpose my image's block (no communication needed)  ------------------

    call copy3 (    u(1,1,first_x,1+my_node*my) &                   !-- intra-node transpose
                ,  ur(1,1,first_y,1+my_node*mx) &                   !-- no inter-node transpose needed
                ,   nz*3, 1_8, 1_8        &                                 !-- note: only 3 of 4 words needed
                ,   mx, nz*4, nz*4*my &
                ,   my, nz*4*mx, nz*4 )

#define RECEIVE
#ifdef RECEIVE

!--------------   issue all block sends ... tags = 256*dst_image+src_image  ------------------

    do stage = 1, num_nodes-1
        to = mod( my_node+stage, num_nodes )
        send_tag = 256*to + my_node
        call MPI_ISSEND ( u(1,1,first_x,1+to*my) &
                        ,   msg_size*2,  MPI_REAL,  to, send_tag,  MPI_COMM_WORLD,  ids(stage),  ierror)
    end do

!--------------   receive and transpose other image's block  ------------------

    do stage = 1, num_nodes-1   !-- process receives in order
        from = mod( my_node+stage, num_nodes )
        recv_tag = 256*my_node + from
        call MPI_RECV  ( bufr &
                        ,   msg_size*2,  MPI_REAL,  from, recv_tag,  MPI_COMM_WORLD,  recv_status,  ierror)

        call copy3 ( bufr, ur(1,1,first_y,1+from*mx)  &                !-- intra-node transpose from buffer
                        ,   nz*3, 1_8, 1_8        &                             !-- note: only 3 of 4 words needed
                        ,   mx, nz*4, nz*4*my &
                        ,   my, nz*4*mx, nz*4 )
    end do

#else

!--------------   issue all block receives ... tags = 256*dst_image+src_image  ------------------

    do stage = 1, num_nodes-1
        from = mod( my_node+stage, num_nodes )
        recv_tag = 256*my_node + from
        call MPI_IRECV  ( ur(1,1,first_y,1+from*mx) &
                        ,   msg_size*2,  MPI_REAL,  from, recv_tag,  MPI_COMM_WORLD,  idr(stage),  ierror)
    end do

!--------------   issue all block sends ... tags = 256*dst_image+src_image  ------------------


    do stage = 1, num_nodes-1   !-- process sends in order
        to = mod( my_node+stage, num_nodes )
        send_tag = 256*to + my_node
        call copy3 ( u(1,1,first_x,1+to*my), bufr &                !-- intra-node transpose from buffer
                ,   nz*3, 1_8, 1_8        &                             !-- note: only 3 of 4 words needed
                ,   mx, nz*4, nz*4*my &
                ,   my, nz*4*mx, nz*4 )

        call MPI_SEND ( bufr &
                        ,   msg_size*2,  MPI_REAL,  to, send_tag,  MPI_COMM_WORLD,  ierror)
    end do

!--------------   wait on receives   ------------------

    do stage = 1, num_nodes-1
        call MPI_WAIT( idr(stage),  recv_status,  ierror )
    end do

#endif

call MPI_BARRIER(MPI_COMM_WORLD, ierror)     !--  wait for other nodes to finish transpose
    tran_time = tran_time + WALLTIME()

! deallocate(ids,idr)

 end  subroutine transpose_X_Y

!-------------   out-of-place transpose data_r --> data_s  ----------------------------

 subroutine transpose_Y_X

    use run_size
    implicit none

    integer(8) :: to, from, send_tag, recv_tag
    integer :: stage, idr(0:num_nodes-1), ids(0:num_nodes-1)
    character*(MPI_MAX_ERROR_STRING) errs
    integer(8) :: send_status(MPI_STATUS_SIZE), recv_status(MPI_STATUS_SIZE)

    call MPI_BARRIER(MPI_COMM_WORLD, ierror)   !--  wait for other nodes to finish compute
    tran_time = tran_time - WALLTIME()

!--------------   transpose my image's block (no communication needed)  ------------------

    call copy3 (   ur(1,1,first_y,1+my_node*mx) &                   !-- intra-node transpose
                ,   u(1,1,first_x,1+my_node*my) &                   !-- no inter-node transpose needed
                ,   nz*4, 1_8, 1_8        &                                 !-- note: all 4 words needed
                ,   my, nz*4, nz*4*mx &
                ,   mx, nz*4*my, nz*4 )

#define RECEIVE
#ifdef RECEIVE

!--------------   issue all block sends ... tags = 256*dst_image+src_image  ------------------

    do stage = 1, num_nodes-1
        to = mod( my_node+stage, num_nodes )
        send_tag = 256*to + my_node
        call MPI_ISSEND ( ur(1,1,first_y,1+to*mx)    &
                        ,   msg_size*2,  MPI_REAL,  to, send_tag,  MPI_COMM_WORLD,  ids(stage),  ierror)

    end do

!--------------   transpose other image's block (get block then transpose it)  ------------------

    do stage = 1, num_nodes-1   !-- process receives in order
        from = mod( my_node+stage, num_nodes )
        recv_tag = 256*my_node + from
        call MPI_RECV  ( bufr  &
                        ,   msg_size*2,  MPI_REAL,  from, recv_tag,  MPI_COMM_WORLD,  recv_status,  ierror)

        call copy3 ( bufr, u(1,1,first_x,1+from*my)  &                 !-- intra-node transpose from buffer
                    ,   nz*4, 1_8, 1_8        &
                    ,   my, nz*4, nz*4*mx &
                    ,   mx, nz*4*my, nz*4 )
    end do

#else

!--------------   issue all block receives ... tags = 256*dst_image+src_image  ------------------

    do stage = 1, num_nodes-1
        from = mod( my_node+stage, num_nodes )
        recv_tag = 256*my_node + from
        call MPI_IRECV  ( u(1,1,first_x,1+from*my) &
                        ,   msg_size*2,  MPI_REAL,  from, recv_tag,  MPI_COMM_WORLD,  idr(stage),  ierror)
    end do

!--------------   issue all block sends ... tags = 256*dst_image+src_image  ------------------


    do stage = 1, num_nodes-1   !-- process sends in order
        to = mod( my_node+stage, num_nodes )
        send_tag = 256*to + my_node
        call copy3 ( ur(1,1,first_y,1+to*mx), bufr &                !-- intra-node transpose from buffer
                ,   nz*4, 1_8, 1_8        &                                 !-- note: all 4 words needed
                ,   my, nz*4, nz*4*mx &
                ,   mx, nz*4*my, nz*4 )

        call MPI_SEND ( bufr &
                        ,   msg_size*2,  MPI_REAL,  to, send_tag,  MPI_COMM_WORLD,  ierror)
    end do

!--------------   wait on receives   ------------------

    do stage = 1, num_nodes-1
        call MPI_WAIT( idr(stage),  recv_status,  ierror )
    end do

#endif

    call MPI_BARRIER(MPI_COMM_WORLD, ierror)     !--  wait for other nodes to finish transpose
    tran_time = tran_time + WALLTIME()
 !   deallocate(ids,idr)
 end  subroutine transpose_Y_X

end program zzz
