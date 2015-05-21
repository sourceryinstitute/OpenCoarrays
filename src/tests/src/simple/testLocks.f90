program locks1
  use iso_fortran_env
  implicit none
 
  type(lock_type) :: l[*]
  integer :: me,np,counter[*],stat
  character(len=100) :: err

  me = this_image()
  np = num_images()

  counter = 0

  sync all

  critical
    counter[1] = counter[1] + 1
  end critical

  sync all

  if(me == 1) write(*,*) counter

  sync all

  lock(l[1])
    if(me == 1) then 
      lock(l[1],stat=stat)
      write(*,*) stat
    endif
  unlock(l[1])

  sync all

  unlock(l[1],stat=stat,errmsg = err)

  write(*,*) stat,err

  sync all

  lock(l[np])
    counter[np] = me
    write(*,*) me,'Writing on counter[np]'
  unlock(l[np])

  sync all

  if(me == np) write(*,*) counter

end program
