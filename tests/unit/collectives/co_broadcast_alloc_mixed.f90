program co_broadcast_derived_with_allocs_test
!! author: Brad Richardson & Andre Vehreschild
!! category: regression
!!
!! [issue #727](https://github.com/sourceryinstitute/opencoarrays/issues/727)
!! broadcasting derived types with a mixture of scalar and allocatable
!! scalars or arrays lead to unexpected results

implicit none

type nsas_t
  integer :: i
  integer, allocatable :: j
end type

type asas_t
  integer, allocatable :: i
  integer, allocatable :: j
end type

type nsaa_t
  integer :: i
  integer, allocatable :: j(:)
end type

type naaa_t
  integer :: i(3)
  integer, allocatable :: j(:)
end type

type(nsas_t) nsas
type(asas_t) asas
type(nsaa_t) nsaa
type(naaa_t) naaa

integer, parameter :: source_image = 1

if (this_image() == source_image)  then
  nsas = nsas_t(2, 3)

  asas = asas_t(4, 5)

  nsaa = nsaa_t(6, (/ 7, 8 /))

  naaa = naaa_t((/ 9,10,11 /), (/ 12,13,14,15 /))
else
  allocate(nsas%j)

  allocate(asas%i); allocate(asas%j)

  allocate(nsaa%j(2))

  allocate(naaa%j(4))
end if

print *, "nsas"
call co_broadcast(nsas, source_image)
if (nsas%i /= 2 .or. nsas%j /= 3)  error stop "Test failed at 1."

print *, "asas"
call co_broadcast(asas, source_image)
if (asas%i /= 4 .or. asas%j /= 5)  error stop "Test failed at 2."

print *, "nsaa"
call co_broadcast(nsaa, source_image)
if (nsaa%i /= 6 .or. any(nsaa%j(:) /= (/ 7, 8 /)))  error stop "Test failed at 3."

print *, "naaa"
call co_broadcast(naaa, source_image)
if (any(naaa%i(:) /= (/ 9,10,11 /)) .or. any(naaa%j(:) /= (/ 12,13,14,15 /))) then
  error stop "Test failed at 3."
end if

sync all

print *, "Test passed."

end
