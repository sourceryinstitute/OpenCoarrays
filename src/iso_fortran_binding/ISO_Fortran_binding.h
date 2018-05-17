/* ISO_Fortran_binding.h of GCC's GNU Fortran compiler.
   Copyright (C) 2013 Free Software Foundation, Inc.

This file is part of the GNU Fortran runtime library (libgfortran)
Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with libquadmath; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 51 Franklin Street
- Fifth Floor, Boston, MA 02110-1301, USA.  */


/* Definitions as defined by ISO/IEC Technical Specification TS 29113:2012
   on Further Interoperability of Fortran with C.

   Note: This header file contains some GCC-specific CFI_type macros.
   Additionally, the order of elements in CFI_cdesc_t (except of the
   first three) and of CFI_dim_t is not defined by TS29113 - and both
   are permitted to have extra fields.  */


#ifndef ISO_FORTRAN_BINDING_H
#define ISO_FORTRAN_BINDING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  /* For size_t and ptrdiff_t.  */
#include <stdint.h>  /* For int32_t etc.  */


/* Constants, defined as macros.  */

#define CFI_VERSION 1
#define CFI_MAX_RANK 15

/* Attribute values.  */

#define CFI_attribute_pointer 1
#define CFI_attribute_allocatable 2
#define CFI_attribute_other 3

/* Error status codes.  */

#define CFI_FAILURE 0
#define CFI_SUCCESS 1
#define CFI_ERROR_BASE_ADDR_NULL 2
#define CFI_ERROR_BASE_ADDR_NOT_NULL 3
#define CFI_INVALID_ELEM_LEN 4
#define CFI_INVALID_RANK 5
#define CFI_INVALID_TYPE 6
#define CFI_INVALID_ATTRIBUTE 7
#define CFI_INVALID_EXTENT 8
#define CFI_INVALID_STRIDE 9
#define CFI_INVALID_DESCRIPTOR 10
#define CFI_ERROR_MEM_ALLOCATION 11
#define CFI_ERROR_OUT_OF_BOUNDS 12


/* Types definitions.  */

typedef ptrdiff_t CFI_index_t;
typedef int8_t CFI_rank_t;
typedef int8_t CFI_attribute_t;
typedef int16_t CFI_type_t;


typedef struct CFI_dim_t
{
  CFI_index_t lower_bound;
  CFI_index_t extent;
  CFI_index_t sm;
}
CFI_dim_t;

typedef struct CFI_cdesc_t
{
  void *base_addr;
  size_t elem_len;
  int version;
  CFI_rank_t rank;
  CFI_attribute_t attribute;
  CFI_type_t type;
  size_t offset;
  CFI_dim_t dim[];
}
CFI_cdesc_t;


/* Extension: CFI_CDESC_T but with an explicit type.  */

#define CFI_CDESC_TYPE_T(r, base_type) \
struct {\
  base_type *base_addr;\
  size_t elem_len;\
  int version; \
  CFI_rank_t rank; \
  CFI_attribute_t attribute; \
  CFI_type_t type;\
  size_t offset;\
  CFI_dim_t dim[r];\
}

#define CFI_CDESC_T(r) CFI_CDESC_TYPE_T (r, void)


/* Functions. */

void *CFI_address (const CFI_cdesc_t *, const CFI_index_t []);
int CFI_allocate (CFI_cdesc_t *, const CFI_index_t [], const CFI_index_t [],
		  size_t);
int CFI_deallocate (CFI_cdesc_t *);
int CFI_establish (CFI_cdesc_t *, void *, CFI_attribute_t, CFI_type_t, size_t,
		   CFI_rank_t, const CFI_index_t []);
int CFI_is_contiguous (const CFI_cdesc_t *);
int CFI_section (CFI_cdesc_t *, const CFI_cdesc_t *, const CFI_index_t [],
		 const CFI_index_t [], const CFI_index_t []);
int CFI_select_part (CFI_cdesc_t *, const CFI_cdesc_t *, size_t, size_t);
int CFI_setpointer (CFI_cdesc_t *, CFI_cdesc_t *, const CFI_index_t []);


/* Types. Consisting of the type and the kind number. */

#define CFI_type_mask 0xFF
#define CFI_type_kind_shift 8

/* Intrinsic types - not to be used directly.  */

#define CFI_type_Integer 1
#define CFI_type_Logical 2
#define CFI_type_Real 3
#define CFI_type_Complex 4
#define CFI_type_Character 5

/* Types without kind paramter.   */

#define CFI_type_struct 6
#define CFI_type_cptr 7
#define CFI_type_cfunptr 8
#define CFI_type_other -1


/* Types with kind parameter; usually the kind is the same as the byte size.
   Exception is  REAL(10) which has a size of 64 bytes but only 80 bits
   precision.  And for complex variables, their byte size is twice the kind
   number (except for complex(10)).  The ucs4_char matches wchar_t
   if sizeof (wchar_t) == 4.  */

#define CFI_type_char (CFI_type_Character + (1 << CFI_type_kind_shift))
#define CFI_type_ucs4_char (CFI_type_Character + (4 << CFI_type_kind_shift))

/* C-Fortran Interoperability types. */

#define CFI_type_signed_char (CFI_type_Integer + (1 << CFI_type_kind_shift))
#define CFI_type_short (CFI_type_Integer + (2 << CFI_type_kind_shift))
#define CFI_type_int (CFI_type_Integer + (4 << CFI_type_kind_shift))
#define CFI_type_long (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_long_long (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_size_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_int8_t (CFI_type_Integer + (1 << CFI_type_kind_shift))
#define CFI_type_int16_t (CFI_type_Integer + (2 << CFI_type_kind_shift))
#define CFI_type_int32_t (CFI_type_Integer + (4 << CFI_type_kind_shift))
#define CFI_type_int64_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_int_least8_t (CFI_type_Integer + (1 << CFI_type_kind_shift))
#define CFI_type_int_least16_t (CFI_type_Integer + (2 << CFI_type_kind_shift))
#define CFI_type_int_least32_t (CFI_type_Integer + (4 << CFI_type_kind_shift))
#define CFI_type_int_least64_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_int_fast8_t (CFI_type_Integer + (1 << CFI_type_kind_shift))
#define CFI_type_int_fast16_t (CFI_type_Integer + (2 << CFI_type_kind_shift))
#define CFI_type_int_fast32_t (CFI_type_Integer + (4 << CFI_type_kind_shift))
#define CFI_type_int_fast64_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_intmax_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_intptr_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_ptrdiff_t (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_int128_t (CFI_type_Integer + (16 << CFI_type_kind_shift))
#define CFI_type_int_least128_t (CFI_type_Integer + (16 << CFI_type_kind_shift))
#define CFI_type_int_fast128_t (CFI_type_Integer + (16 << CFI_type_kind_shift))
#define CFI_type_Bool (CFI_type_Logical + (1 << CFI_type_kind_shift))
#define CFI_type_float (CFI_type_Real + (4 << CFI_type_kind_shift))
#define CFI_type_double (CFI_type_Real + (8 << CFI_type_kind_shift))
#define CFI_type_long_double (CFI_type_Real + (10 << CFI_type_kind_shift))
#define CFI_type_float128 (CFI_type_Real + (16 << CFI_type_kind_shift))
#define CFI_type_float_Complex (CFI_type_Complex + (4 << CFI_type_kind_shift))
#define CFI_type_double_Complex (CFI_type_Complex + (8 << CFI_type_kind_shift))
#define CFI_type_long_double_Complex (CFI_type_Complex + (10 << CFI_type_kind_shift))
#define CFI_type_float128_Complex (CFI_type_Complex + (16 << CFI_type_kind_shift))

/* gfortran intrinsic non-character types.  */

#define CFI_type_Integer1 (CFI_type_Integer + (1 << CFI_type_kind_shift))
#define CFI_type_Integer2 (CFI_type_Integer + (2 << CFI_type_kind_shift))
#define CFI_type_Integer4 (CFI_type_Integer + (4 << CFI_type_kind_shift))
#define CFI_type_Integer8 (CFI_type_Integer + (8 << CFI_type_kind_shift))
#define CFI_type_Integer16 (CFI_type_Integer + (16 << CFI_type_kind_shift))
#define CFI_type_Logical1 (CFI_type_Logical + (1 << CFI_type_kind_shift))
#define CFI_type_Logical2 (CFI_type_Logical + (2 << CFI_type_kind_shift))
#define CFI_type_Logical4 (CFI_type_Logical + (4 << CFI_type_kind_shift))
#define CFI_type_Logical8 (CFI_type_Logical + (8 << CFI_type_kind_shift))
#define CFI_type_Logical16 (CFI_type_Logical + (16 << CFI_type_kind_shift))
#define CFI_type_Real4 (CFI_type_Real + (4 << CFI_type_kind_shift))
#define CFI_type_Real8 (CFI_type_Real + (8 << CFI_type_kind_shift))
#define CFI_type_Real10 (CFI_type_Real + (10 << CFI_type_kind_shift))
#define CFI_type_Real16 (CFI_type_Real + (16 << CFI_type_kind_shift))
#define CFI_type_Complex4 (CFI_type_Complex + (4 << CFI_type_kind_shift))
#define CFI_type_Complex8 (CFI_type_Complex + (8 << CFI_type_kind_shift))
#define CFI_type_Complex10 (CFI_type_Complex + (10 << CFI_type_kind_shift))
#define CFI_type_Complex16 (CFI_type_Complex + (16 << CFI_type_kind_shift))

#ifdef __cplusplus
}
#endif

#endif  /* ISO_FORTRAN_BINDING_H */
