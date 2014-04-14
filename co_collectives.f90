!
! Provides access to CO_SUM, CO_MIN and CO_MAX of TS 18508.
!
! Use as you would use the official intrinsics, but:
! Always append an errmsg_len=0 or errmsg_len=len(errmsg)
! Always include result_image=...; use "0" is all images shall get the
! result
!
module co_collectives
  implicit none
  interface
    subroutine co_sum(source, result, result_image, stat, errmsg, errmsg_len) &
        bind(C, name="_gfortran_caf_co_sum")
      type(*), dimension(..) :: source
      type(*), dimension(..), optional :: result
      integer, value :: result_image
      integer, optional :: stat
      character, optional :: errmsg(*)
      integer, value :: errmsg_len
    end subroutine co_sum
  end interface
  procedure(co_sum), bind(C, name="_gfortran_caf_co_min") :: co_min
  procedure(co_sum), bind(C, name="_gfortran_caf_co_max") :: co_max
end module co_collectives
