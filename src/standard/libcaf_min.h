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

#ifndef LIBCAF_MIN_H
#define LIBCAF_MIN_H

#include "cfi-descriptor.h"
#include "../libcaf-version-def.h"
#include "../libcaf-gfortran-descriptor.h"


 //#define __attribute__(x)
#define likely(x)       (x)
#define unlikely(x)     (x)
#define PREFIX(X) X

#define GCC_GE_8 1

/* Definitions of the Fortran 2008 standard; need to kept in sync with
   ISO_FORTRAN_ENV, cf. libgfortran.h.  */
#define STAT_UNLOCKED           0
#define STAT_LOCKED             1
#define STAT_LOCKED_OTHER_IMAGE 2
#define STAT_DUP_SYNC_IMAGES    3
#define STAT_STOPPED_IMAGE      6000
#define STAT_FAILED_IMAGE       6001

#define GFC_CAF_ARG_VALUE  (1<<2)



/* Describes what type of array we are registerring. Keep in sync with
   gcc/fortran/trans.h.  */
typedef enum caf_register_t {
  CAF_REGTYPE_COARRAY_STATIC,
  CAF_REGTYPE_COARRAY_ALLOC,
  CAF_REGTYPE_LOCK_STATIC,
  CAF_REGTYPE_LOCK_ALLOC,
  CAF_REGTYPE_CRITICAL,
  CAF_REGTYPE_EVENT_STATIC,
  CAF_REGTYPE_EVENT_ALLOC,
  CAF_REGTYPE_COARRAY_ALLOC_REGISTER_ONLY,
  CAF_REGTYPE_COARRAY_ALLOC_ALLOCATE_ONLY
}
caf_register_t;

/* Describes the action to take on _caf_deregister. Keep in sync with
   gcc/fortran/trans.h.  */
typedef enum caf_deregister_t {
  CAF_DEREGTYPE_COARRAY_DEREGISTER,
  CAF_DEREGTYPE_COARRAY_DEALLOCATE_ONLY
}
caf_deregister_t;

typedef void* caf_token_t;
/** Add a dummy type representing teams in coarrays. */

typedef void * caf_team_t;

typedef struct caf_teams_list {
  caf_team_t team;
  int team_id;
  struct caf_teams_list *prev;
} caf_teams_list;

typedef struct caf_used_teams_list {
  struct caf_teams_list *team_list_elem;
  struct caf_used_teams_list *prev;
} caf_used_teams_list;


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
} caf_vector_t;


/* The type to use for string lengths.  */
#ifdef GCC_GE_8
typedef size_t charlen_t;
#else
#error "This code should not compile"
#endif


void PREFIX (sync_all) (int *, char *, charlen_t);
bool PREFIX (is_contiguous) (CFI_cdesc_t *);


#endif  /* LIBCAF_MIN_H  */
