/* Common declarations for all of GNU Fortran libcaf implementations.

Copyright (c) 2012-2014, OpenCoarray Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#ifndef LIBCAF_H
#define LIBCAF_H

#include <stdint.h>	/* For int32_t.  */
#include <stddef.h>	/* For size_t.  */
#include <stdbool.h>

#ifndef __GNUC__
#define __attribute__(x)
#define likely(x)       (x)
#define unlikely(x)     (x)
#else
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

#ifdef PREFIX_NAME
#define PREFIX3(X,Y) X ## Y
#define PREFIX2(X,Y) PREFIX3(X,Y)
#define PREFIX(X) PREFIX2(PREFIX_NAME,X)
#else
#define PREFIX(X) X
#endif


/* Definitions of the Fortran 2008 standard; need to kept in sync with
   ISO_FORTRAN_ENV, cf. libgfortran.h.  */
#define STAT_UNLOCKED		0
#define STAT_LOCKED		1
#define STAT_LOCKED_OTHER_IMAGE	2
#define STAT_STOPPED_IMAGE 	6000

/* Describes what type of array we are registerring. Keep in sync with
   gcc/fortran/trans.h.  */
typedef enum caf_register_t {
  CAF_REGTYPE_COARRAY_STATIC,
  CAF_REGTYPE_COARRAY_ALLOC,
  CAF_REGTYPE_LOCK,
  CAF_REGTYPE_LOCK_COMP
}
caf_register_t;

typedef void* caf_token_t;


/* GNU Fortran's array descriptor.  Keep in sync with libgfortran.h.  To be
   replaced by TS29113's ISO_Fortran_binding.h with CFI_cdesc_t.  */

typedef struct descriptor_dimension
{
  ptrdiff_t _stride;
  ptrdiff_t lower_bound;
  ptrdiff_t _ubound;
}
descriptor_dimension;

typedef struct gfc_descriptor_t {
  void *base_addr;
  size_t offset;
  ptrdiff_t dtype;
  descriptor_dimension dim[];
} gfc_descriptor_t;


#define GFC_MAX_DIMENSIONS 7

#define GFC_DTYPE_RANK_MASK 0x07
#define GFC_DTYPE_TYPE_SHIFT 3
#define GFC_DTYPE_TYPE_MASK 0x38
#define GFC_DTYPE_SIZE_SHIFT 6
#define GFC_DESCRIPTOR_RANK(desc) ((desc)->dtype & GFC_DTYPE_RANK_MASK)
#define GFC_DESCRIPTOR_TYPE(desc) (((desc)->dtype & GFC_DTYPE_TYPE_MASK) \
                                   >> GFC_DTYPE_TYPE_SHIFT)
#define GFC_DESCRIPTOR_SIZE(desc) ((desc)->dtype >> GFC_DTYPE_SIZE_SHIFT)


/* Linked list of static coarrays registered.  */
typedef struct caf_static_t {
  caf_token_t token;
  struct caf_static_t *prev;
}
caf_static_t;


/* Common auxiliary functions: caf_auxiliary.c.  */

bool PREFIX (is_contiguous) (gfc_descriptor_t *);


/* Header for the specific implementation.  */

void PREFIX(init) (int *, char ***);
void PREFIX(finalize) (void);

int PREFIX(this_image) (int);
int PREFIX(num_images) (int, int);

void *PREFIX(register) (size_t, caf_register_t, caf_token_t *, int *, char *,
			int);
void PREFIX(deregister) (caf_token_t *, int *, char *, int);

void PREFIX(send) (caf_token_t, size_t, int, void *, size_t, bool);
void PREFIX(send_desc) (caf_token_t, size_t, int, gfc_descriptor_t*,
			gfc_descriptor_t*, bool);
void PREFIX(send_desc_scalar) (caf_token_t, size_t, int, gfc_descriptor_t*,
			       void*, bool);

void PREFIX(sync_all) (int *, char *, int);
void PREFIX(sync_images) (int, int[], int *, char *, int);

/* FIXME: The CRITICAL functions should be removed;
   the functionality is better represented using Coarray's lock feature.  */
void PREFIX(critical) (void);
void PREFIX(end_critical) (void);

void PREFIX(error_stop_str) (const char *, int32_t)
     __attribute__ ((noreturn));
void PREFIX(error_stop) (int32_t) __attribute__ ((noreturn));

#endif  /* LIBCAF_H  */
