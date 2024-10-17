program get_with_1d_vector_index
  use iso_fortran_env
  implicit none
  integer, parameter :: nloc=8, nhl=2, ivsize=nloc+2*nhl
  real    :: xv(ivsize)[*]
  real, allocatable :: expected(:)
  integer :: rmt_idx(2), loc_idx(2)
  integer, allocatable :: xchg(:)
  integer :: nrcv, me, np, nxch, i, iv
  character(len=120) :: fmt

  me = this_image()
  np = num_images()

  if (np==1) then
    xchg = [ integer :: ]

  else if (me == 1) then
    xchg = [me+1]
  else if (me == np) then
    xchg = [me-1]
  else
    xchg = [me-1, me+1]
  end if
  nxch = size(xchg)
  nrcv = nxch * nhl

  allocate(expected(nxch))
  xv(1:nloc) = [(i,i=(me-1)*nloc+1,me*nloc)]
  iv = nloc + 1
  loc_idx(1:nhl) = [ (i,i=iv,iv+nhl-1) ]
  rmt_idx(1:nhl) = [ (i,i=1,nhl) ]

  sync images(xchg)
  iv = nloc + 1

  xv(iv:iv+nhl-1) = xv(rmt_idx(1:nhl))[xchg(1)]
  print *, me, ":", xv
  iv = iv + nhl
  if (me == 1) then
    expected(:) = nloc + rmt_idx(1:nhl)
  else
    expected(:) = ((me - 2) * nloc) + rmt_idx(1:nhl)
  end if

  sync all
  if (any(xv(loc_idx(1:nhl)) /= expected(:))) then
    write(fmt,*) '( i0,a,',nhl,'(f5.0,1x),a,',nhl,'(f5.0,1x) )'
    write(*,fmt) me,': is:',xv(loc_idx(1:nhl)),', exp:',expected(1:nhl)

    error stop 'Test failed.'
  end if

  sync all
  if (me == 1) print *, 'Test passed.'
end program get_with_1d_vector_index
