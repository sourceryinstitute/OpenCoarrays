/* One-sided MPI implementation of Libcaf

Copyright (c) 2012-2016, Sourcery, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Sourcery, Inc., nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE FOR ANY
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

#include "libcaf-gfortran-descriptor.h"

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
#define STAT_DUP_SYNC_IMAGES    3
#define STAT_STOPPED_IMAGE 	6000

/* Describes what type of array we are registerring. Keep in sync with
   gcc/fortran/trans.h.  */
typedef enum caf_register_t {
  CAF_REGTYPE_COARRAY_STATIC,
  CAF_REGTYPE_COARRAY_ALLOC,
  CAF_REGTYPE_LOCK_STATIC,
  CAF_REGTYPE_LOCK_ALLOC,
  CAF_REGTYPE_CRITICAL,
  CAF_REGTYPE_EVENT_STATIC,
  CAF_REGTYPE_EVENT_ALLOC
}
caf_register_t;

typedef void* caf_token_t;


/* Linked list of static coarrays registered.  */
typedef struct caf_static_t {
  caf_token_t token;
  struct caf_static_t *prev;
}
caf_static_t;

/* When there is a vector subscript in this dimension, nvec == 0, otherwise,
   lower_bound, upper_bound, stride contains the bounds relative to the declared
   bounds; kind denotes the integer kind of the elements of vector[].  */
typedef struct caf_vector_t {
  size_t nvec;
  union {
    struct {
      void *vector;
      int kind;
    } v;
    struct {
      ptrdiff_t lower_bound, upper_bound, stride;
    } triplet;
  } u;
}
caf_vector_t;



/* Common auxiliary functions: caf_auxiliary.c.  */

bool PREFIX (is_contiguous) (gfc_descriptor_t *);


/* Header for the specific implementation.  */

void PREFIX (init) (int *, char ***);
void PREFIX (finalize) (void);

int PREFIX (this_image) (int);
int PREFIX (num_images) (int, int);

void *PREFIX (register) (size_t, caf_register_t, caf_token_t *, int *, char *,
			int);
void PREFIX (deregister) (caf_token_t *, int *, char *, int);

void PREFIX (caf_get) (caf_token_t, size_t, int, gfc_descriptor_t *,
		       caf_vector_t *, gfc_descriptor_t *, int, int);
void PREFIX (caf_send) (caf_token_t, size_t, int, gfc_descriptor_t *,
                        caf_vector_t *, gfc_descriptor_t *, int, int);

void PREFIX (caf_sendget) (caf_token_t, size_t, int, gfc_descriptor_t *,
			   caf_vector_t *, caf_token_t, size_t, int,
			   gfc_descriptor_t *, caf_vector_t *, int, int);

void PREFIX (co_max) (gfc_descriptor_t *, int, int *, char *, int, int);
void PREFIX (co_min) (gfc_descriptor_t *, int, int *, char *, int, int);
void PREFIX (co_sum) (gfc_descriptor_t *, int, int *, char *, int);

void PREFIX (sync_all) (int *, char *, int);
void PREFIX (sync_images) (int, int[], int *, char *, int);

void PREFIX (error_stop_str) (const char *, int32_t)
     __attribute__ ((noreturn));
void PREFIX (error_stop) (int32_t) __attribute__ ((noreturn));

void PREFIX (atomic_define) (caf_token_t, size_t, int, void *, int *, int, int);
void PREFIX (atomic_ref) (caf_token_t, size_t, int, void *, int *, int, int);
void PREFIX (atomic_cas) (caf_token_t, size_t, int, void *, void *,
			  void *, int *, int, int);
void PREFIX (atomic_op) (int, caf_token_t, size_t, int, void *, void *,
			 int *, int, int);

void PREFIX (lock) (caf_token_t, size_t, int, int *, int *, char *, int);
void PREFIX (unlock) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_post) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_wait) (caf_token_t, size_t, int, int *, char *, int);
void PREFIX (event_query) (caf_token_t, size_t, int, int *, int *);
#endif  /* LIBCAF_H  */
