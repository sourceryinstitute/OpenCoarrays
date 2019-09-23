Program test1
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Test program to CO_BROADCAST two user defined types
!  1) Allocatable, dimensioned user type
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  Implicit None
!
  Integer ,Parameter                             :: C1 = 1, C2 = 2
  Integer                                        :: i, j, k, N0, me, number_images
!
  Type L2_Data
     Integer                                     :: N2
     Integer          ,Allocatable ,Dimension(:) :: A2
  End Type L2_Data
!
  Type L1_Data
     Integer                                     :: N1
     Integer          ,Allocatable ,Dimension(:) :: B1
     Type ( L2_Data ) ,Allocatable ,Dimension(:) :: L2
  End Type L1_Data
!
  Type ( L1_Data )    ,Allocatable ,Dimension(:) :: L1
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Setting image data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  number_images = num_images()
  me            = this_image()
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Setting L1 Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  If ( me .eq. C1 ) Then
     Write (*,*) 'L1: Data on image ', me
     N0 = 2
     Allocate ( L1(N0) )
     Write (*,*) 'L1: i,j,k,N = ', -1, -1, -1, N0, ' = ', 2
     Do i = 1, N0
        L1(i)%N1 = 3
        Write (*,*) 'L1: i,j,k,N = ', i, -1, -1, L1(i)%N1, ' = ', 3
        Allocate ( L1(i)%B1(L1(i)%N1) )
        Do j = 1, L1(i)%N1
           L1(i)%B1(j) = j
           Write (*,*) 'L1: i,j,k,B = ', i, j, -1, L1(i)%B1(j), ' = ', j
        End Do
        Allocate ( L1(i)%L2(L1(i)%N1) )
        Do j = 1, L1(i)%N1
           L1(i)%L2(j)%N2 = 4
           Write (*,*) 'L1: i,j,k,N = ', i, j, -1, L1(i)%L2(j)%N2, ' = ', 4
           Allocate ( L1(i)%L2(j)%A2(L1(i)%L2(j)%N2) )
           Do k = 1, L1(i)%L2(j)%N2
              L1(i)%L2(j)%A2(k) = 10*k
              Write (*,*) 'L1: i,j,k,A = ', i, j, k, L1(i)%L2(j)%A2(k), ' = ', 10*k
           End Do
        End Do
     End Do
     Write (*,*) 'Calling CO_BROADCAST for L1 Data'
  End If
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! CO_BROADCASTing L1 Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  Sync All
!
if(me.eq.C1)print *, 'co_bro: N0                                 = ', N0
  Call CO_BROADCAST ( N0,     C1 )
  If ( me .ne. C1 ) Then
print *, 'Values: N0                                 = ', N0
     Allocate ( L1(N0) )
  End If
  Do i = 1, N0
if(me.eq.C1)print *, 'co_bro: N0, i, L1(i)%N1                    = ', N0, i, L1(i)%N1
     Call CO_BROADCAST ( L1(i)%N1,   C1 )
     If ( me .ne. C1 ) Then
print *, 'Values: N0, i, L1(i)%N1                    = ', N0, i, L1(i)%N1
        Allocate ( L1(i)%B1(L1(i)%N1) )
        Allocate ( L1(i)%L2(L1(i)%N1) )
     End If
     Do j = 1, L1(i)%N1
if(me.eq.C1)print *, 'co_bro: N0, i, L1(i)%N1, j, L1(i)%L2(j)%N2 = ', N0, i, L1(i)%N1, j, L1(i)%L2(j)%N2
        Call CO_BROADCAST ( L1(i)%L2(j)%N2, C1 )
        If ( me .ne. C1 ) Then
print *, 'Values: N0, i, L1(i)%N1, j, L1(i)%L2(j)%N2 = ', N0, i, L1(i)%N1, j, L1(i)%L2(j)%N2
           Allocate ( L1(i)%L2(j)%A2(L1(i)%L2(j)%N2) )
        End If
     End Do
  End Do
if(me.eq.C1)print *, 'co_bro: L1'
  Call CO_BROADCAST ( L1, C1 )
!
  Sync All
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Writting L1 Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  If ( me .ne. C1 ) Then
     Write (*,*) 'L1: Data on image ', me
     Write (*,*) 'L1: i,j,k,N = ', -1, -1, -1, N0, ' = ', 2
     Do i = 1, N0
        Write (*,*) 'L1: i,j,k,N = ', i, -1, -1, L1(i)%N1, ' = ', 3
        Do j = 1, L1(i)%N1
           Write (*,*) 'L1: i,j,k,B = ', i, j, -1, L1(i)%B1(j), ' = ', j
        End Do
        Do j = 1, L1(i)%N1
           Write (*,*) 'L1: i,j,k,N = ', i, j, -1, L1(i)%L2(j)%N2, ' = ', 4
           Do k = 1, L1(i)%L2(j)%N2
              Write (*,*) 'L1: i,j,k,A = ', i, j, k, L1(i)%L2(j)%A2(k), ' = ', 10*k
           End Do
        End Do
     End Do
  End If
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! End of program
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
End Program test1
