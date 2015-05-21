program cobroadcast
  implicit none

  integer :: np, me
  integer,codimension[*] :: a
  integer, dimension(10), codimension[*] :: aa
  integer, dimension(10) :: bb

  np = num_images()
  me = this_image()

  a = 0

  sync all

  if (me == 1) a = me

  call co_broadcast(a,1)

  if(a /= 1) call abort()

  write(*,*) 'OK for scalars'

  bb = 1

  if(me == 1) aa = me

  sync all

  call co_broadcast(aa,1)

  if(any(aa /= bb)) call abort()

  write(*,*) 'OK for contiguous arrays'

  bb = 1

  bb(1:10:2) = -1

  if(me == 1) aa(1:10:2) = -1

  sync all

  call co_broadcast(aa,1)

  if(any(aa /= bb)) call abort()

  write(*,*) 'OK for strided'

end program
