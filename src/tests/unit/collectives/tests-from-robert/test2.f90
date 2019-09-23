Program test2
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Test program to CO_BROADCAST two user defined types
!  1) Allocatable, dimensioned user type
!  2) Scalre user type
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  Implicit None
!
  Integer ,Parameter                             :: C1 = 1, C2 = 2
  Integer                                        :: i, j, k, me, number_images
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
  Type L_Data
     Integer                                     :: N0
     Type ( L1_Data ) ,Allocatable ,Dimension(:) :: L1
  End type L_Data
!
  Type ( L_Data )                                :: L
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Setting image data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  number_images = num_images()
  me            = this_image()
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Setting L Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  If ( me .eq. C2 ) Then
     Write (*,*) 'L: Data on image ', me
     L%N0 = 3
     Allocate ( L%L1(L%N0) )
     Write (*,*) 'L:  i,j,k,N = ', -1, -1, -1, L%N0, ' = ', 3
     Do i = 1, L%N0
        L%L1(i)%N1 = 4
        Write (*,*) 'L:  i,j,k,N = ', i, -1, -1, L%L1(i)%N1, ' = ', 4
        Allocate ( L%L1(i)%B1(L%L1(i)%N1) )
        Do j = 1, L%L1(i)%N1
           L%L1(i)%B1(j) = 10*j
           Write (*,*) 'L:  i,j,k,B = ', i, j, -1, L%L1(i)%B1(j), ' = ', 10*j
        End Do
        Allocate ( L%L1(i)%L2(L%L1(i)%N1) )
        Do j = 1, L%L1(i)%N1
           L%L1(i)%L2(j)%N2 = 5
           Write (*,*) 'L:  i,j,k,N = ', i, j, -1, L%L1(i)%L2(j)%N2, ' = ', 5
           Allocate ( L%L1(i)%L2(j)%A2(L%L1(i)%L2(j)%N2) )
           Do k = 1, L%L1(i)%L2(j)%N2
              L%L1(i)%L2(j)%A2(k) = 100*k
              Write (*,*) 'L:  i,j,k,A = ', i, j, k, L%L1(i)%L2(j)%A2(k), ' = ', 100*k
           End Do
        End Do
     End Do
     Write (*,*) 'Calling CO_BROADCAST for L Data'
  End If
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! CO_BROADCASTing L Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  Sync All
!
if(me.eq.C2)print *, 'co_bro: L%N0                                     = ', L%N0
  Call CO_BROADCAST ( L%N0,     C2 )
  If ( me .ne. C2 ) Then
print *, 'Values: L%N0                                     = ', L%N0
     Allocate ( L%L1(L%N0) )
  End If
  Do i = 1, L%N0
if(me.eq.C2)print *, 'co_bro: L%N0, i, L%L1(i)%N1                      = ', L%N0, i, L%L1(i)%N1
     Call CO_BROADCAST ( L%L1(i)%N1,   C2 )
     If ( me .ne. C2 ) Then
print *, 'Values: L%N0, i, L%L1(i)%N1                      = ', L%N0, i, L%L1(i)%N1
        Allocate ( L%L1(i)%B1(L%L1(i)%N1) )
        Allocate ( L%L1(i)%L2(L%L1(i)%N1) )
     End If
     Do j = 1, L%L1(i)%N1
if(me.eq.C2)print *, 'co_bro: L%N0, i, L%L1(i)%N1, j, L%L1(i)%L2(j)%N2 = ', L%N0, i, L%L1(i)%N1, j, L%L1(i)%L2(j)%N2
        Call CO_BROADCAST ( L%L1(i)%L2(j)%N2, C2 )
        If ( me .ne. C2 ) Then
print *, 'Values: L%N0, i, L%L1(i)%N1, j, L%L1(i)%L2(j)%N2 = ', L%N0, i, L%L1(i)%N1, j, L%L1(i)%L2(j)%N2
           Allocate ( L%L1(i)%L2(j)%A2(L%L1(i)%L2(j)%N2) )
        End If
     End Do
  End Do
if(me.eq.C2)print *, 'co_bro: L'
  Call CO_BROADCAST ( L, C2 )
!
  Sync All
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Writting L Data
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
  If ( me .ne. C2 ) Then
     Write (*,*) 'L: Data on image ', me
     Write (*,*) 'L:  i,j,k,N = ', -1, -1, -1, L%N0, ' = ', 3
     Do i = 1, L%N0
        Write (*,*) 'L:  i,j,k,N = ', i, -1, -1, L%L1(i)%N1, ' = ', 4
        Do j = 1, L%L1(i)%N1
           Write (*,*) 'L:  i,j,k,B = ', i, j, -1, L%L1(i)%B1(j), ' = ', 10*j
        End Do
        Do j = 1, L%L1(i)%N1
           Write (*,*) 'L:  i,j,k,N = ', i, j, -1, L%L1(i)%L2(j)%N2, ' = ', 5
           Do k = 1, L%L1(i)%L2(j)%N2
              Write (*,*) 'L:  i,j,k,A = ', i, j, k, L%L1(i)%L2(j)%A2(k), ' = ', 100*k
           End Do
        End Do
     End Do
  End If
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! End of program
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
End Program test2
