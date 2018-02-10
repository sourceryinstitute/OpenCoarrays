program slice
   type coarr
      real, allocatable :: a(:, :, :)[:, :, :]
   end type

   type(coarr) :: co
   integer :: nimg, me, z, nx, ny, nz, north, south, mex, mey, mez, coords(3)
   real, allocatable :: bufxz(:, :) ! a plane (2d) slice, normal in the y direction

   nx = 6
   ny = 4
   nz = 2

   me = this_image()
   nimg = num_images()

   if (nimg /= 8) stop

   allocate(co % a(nx, ny, nz)[1:2, 1:2, *])
   allocate(bufxz(nx, nz))

   co % a = reshape([(z, z=1, nx * ny * nz)], shape(co % a))

   coords = this_image(co % a)
   mex = coords(1)
   mey = coords(2)
   mez = coords(3)

   north = mey + 1
   south = mey - 1

   sync all
   if (north <= 2) then
      z = image_index(co % a, [mex, north, mez])
      sync images(z)
      bufxz = co % a(1:nx, 1, 1:nz)[mex, north, mez]
      co % a(1:nx, ny, 1:nz) = bufxz
   end if
   if (south >= 1) then
      z = image_index(co % a, [mex, south, mez])
      sync images(z)
      bufxz = co % a(1:nx, 1, 1:nz)[mex, south, mez]
      co % a(1:nx, ny, 1:nz) = bufxz
   end if
   sync all

   deallocate(co % a, bufxz)
   if(this_image() == 1) write(*,*) "Test passed."
   ! Regression would cause error message:
   ! Fortran runtime error on image <...>: libcaf_mpi::caf_get_by_ref(): rank out of range.
 end program
