/* Single-Image implementation of Libcaf

Copyright (c) 2012-2014, Sourcery, Inc.
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

#include "libcaf.h"
#include <stdio.h>  /* For fputs and fprintf.  */
#include <stdlib.h> /* For exit and malloc.  */
#include <string.h> /* For memcpy and memset.  */
#include <stdarg.h> /* For variadic arguments.  */

/* Define GFC_CAF_CHECK to enable run-time checking.  */
/* #define GFC_CAF_CHECK  1  */

/* Single-image implementation of the CAF library.
   Note: For performance reasons -fcoarry=single should be used
   rather than this library.  */

typedef void* single_token_t;
#define TOKEN(X) ((single_token_t) (X))


/* Global variables.  */
caf_static_t *caf_static_list = NULL;


/* Keep in sync with mpi.c.  */
static void
caf_runtime_error (const char *message, ...)
{
  va_list ap;
  fprintf (stderr, "Fortran runtime error: ");
  va_start (ap, message);
  vfprintf (stderr, message, ap);
  va_end (ap);
  fprintf (stderr, "\n");

  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  exit (EXIT_FAILURE);
}

void
PREFIX(init) (int *argc __attribute__ ((unused)),
	      char ***argv __attribute__ ((unused)))
{
}


void
PREFIX (finalize) (void)
{
  while (caf_static_list != NULL)
    {
      caf_static_t *tmp = caf_static_list->prev;
      free (TOKEN (caf_static_list->token));
      free (caf_static_list);
      caf_static_list = tmp;
    }
}


int
PREFIX (this_image) (int distance __attribute__ ((unused)))
{
  return 1;
}


int
PREFIX (num_images) (int distance __attribute__ ((unused)),
		    int failed __attribute__ ((unused)))
{
  return 1;
}


void *
PREFIX (register) (size_t size, caf_register_t type, caf_token_t *token,
		  int *stat, char *errmsg, int errmsg_len)
{
  void *local;

  local = malloc (size);
  *token = malloc (sizeof (single_token_t));

  if (unlikely (local == NULL || TOKEN (*token) == NULL))
    {
      const char msg[] = "Failed to allocate coarray";
      if (stat)
	{
	  *stat = 1;
	  if (errmsg_len > 0)
	    {
	      int len = ((int) sizeof (msg) > errmsg_len) ? errmsg_len
							  : (int) sizeof (msg);
	      memcpy (errmsg, msg, len);
	      if (errmsg_len > len)
		memset (&errmsg[len], ' ', errmsg_len-len);
	    }
	  return NULL;
	}
      else
	  caf_runtime_error (msg);
    }

  *token = local;

  if (stat)
    *stat = 0;

  if (type == CAF_REGTYPE_COARRAY_STATIC)
    {
      caf_static_t *tmp = malloc (sizeof (caf_static_t));
      tmp->prev  = caf_static_list;
      tmp->token = *token;
      caf_static_list = tmp;
    }
  return local;
}


void
PREFIX (deregister) (caf_token_t *token, int *stat,
		     char *errmsg __attribute__ ((unused)),
		     int errmsg_len __attribute__ ((unused)))
{
  free (TOKEN (*token));

  if (stat)
    *stat = 0;
}


static void
convert_type (void *dst, int dst_type, int dst_kind, void *src,
		       int src_type, int src_kind)
{
#ifdef HAVE_GFC_INTEGER_16
   typedef __int128 int128t;
#else
   typedef int64_t int128t;
#endif

#if defined(GFC_REAL_16_IS_LONG_DOUBLE)
   typedef long double real128t;
   typedef _Complex long double complex128t;
#elif defined(HAVE_GFC_REAL_16)
   typedef _Complex float __attribute__((mode(TC))) __complex128;
   typedef __float128 real128t;
   typedef __complex128 complex128t;
#elif defined(HAVE_GFC_REAL_10)
   typedef long double real128t;
   typedef long double complex128t;
#else
   typedef double real128t;
   typedef _Complex double complex128t;
#endif

   int128t int_val = 0;
   real128t real_val = 0;
   complex128t cmpx_val = 0;

   switch (src_type)
     {
     case BT_INTEGER:
	if (src_kind == 1)
	  int_val = *(int8_t*) src;
	else if (src_kind == 2)
	  int_val = *(int16_t*) src;
	else if (src_kind == 4)
	  int_val = *(int32_t*) src;
	else if (src_kind == 8)
	  int_val = *(int64_t*) src;
#ifdef HAVE_GFC_INTEGER_16
	else if (src_kind == 16)
	  int_val = *(int128t*) src;
#endif
	else
	  goto error;
	break;
     case BT_REAL:
	if (src_kind == 4)
	  real_val = *(float*) src;
	else if (src_kind == 8)
	  real_val = *(double*) src;
#ifdef HAVE_GFC_REAL_10
	else if (src_kind == 10)
	  real_val = *(long double*) src;
#endif
#ifdef HAVE_GFC_REAL_16
	else if (src_kind == 16)
	  real_val = *(real128t*) src;
#endif
	else
	  goto error;
	break;
     case BT_COMPLEX:
	if (src_kind == 4)
	  cmpx_val = *(_Complex float*) src;
	else if (src_kind == 8)
	  cmpx_val = *(_Complex double*) src;
#ifdef HAVE_GFC_REAL_10
	else if (src_kind == 10)
	  cmpx_val = *(_Complex long double*) src;
#endif
#ifdef HAVE_GFC_REAL_16
	else if (src_kind == 16)
	  cmpx_val = *(complex128t*) src;
#endif
	else
	  goto error;
	break;
     default:
	goto error;
     }

   switch (dst_type)
     {
     case BT_INTEGER:
	if (src_type == BT_INTEGER)
	  {
	    if (dst_kind == 1)
	      *(int8_t*) dst = (int8_t) int_val;
	    else if (dst_kind == 2)
	      *(int16_t*) dst = (int16_t) int_val;
	    else if (dst_kind == 4)
	      *(int32_t*) dst = (int32_t) int_val;
	    else if (dst_kind == 8)
	      *(int64_t*) dst = (int64_t) int_val;
#ifdef HAVE_GFC_INTEGER_16
	    else if (dst_kind == 16)
	      *(int128t*) dst = (int128t) int_val;
#endif
	    else
	      goto error;
	  }
        else if (src_type == BT_REAL)
	  {
	    if (dst_kind == 1)
	      *(int8_t*) dst = (int8_t) real_val;
	    else if (dst_kind == 2)
	      *(int16_t*) dst = (int16_t) real_val;
	    else if (dst_kind == 4)
	      *(int32_t*) dst = (int32_t) real_val;
	    else if (dst_kind == 8)
	      *(int64_t*) dst = (int64_t) real_val;
#ifdef HAVE_GFC_INTEGER_16
	    else if (dst_kind == 16)
	      *(int128t*) dst = (int128t) real_val;
#endif
	    else
	      goto error;
	  }
        else if (src_type == BT_COMPLEX)
	  {
	    if (dst_kind == 1)
	      *(int8_t*) dst = (int8_t) cmpx_val;
	    else if (dst_kind == 2)
	      *(int16_t*) dst = (int16_t) cmpx_val;
	    else if (dst_kind == 4)
	      *(int32_t*) dst = (int32_t) cmpx_val;
	    else if (dst_kind == 8)
	      *(int64_t*) dst = (int64_t) cmpx_val;
#ifdef HAVE_GFC_INTEGER_16
	    else if (dst_kind == 16)
	      *(int128t*) dst = (int128t) cmpx_val;
#endif
	    else
	      goto error;
	  }
        else
          goto error;
	break;
     case BT_REAL:
	if (src_type == BT_INTEGER)
	  {
	    if (dst_kind == 4)
	      *(float*) dst = (float) int_val;
	    else if (dst_kind == 8)
	      *(double*) dst = (double) int_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(long double*) dst = (long double) int_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(real128t*) dst = (real128t) int_val;
#endif
	    else
	      goto error;
	  }
	else if (src_type == BT_REAL)
	  {
	    if (dst_kind == 4)
	      *(float*) dst = (float) real_val;
	    else if (dst_kind == 8)
	      *(double*) dst = (double) real_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(long double*) dst = (long double) real_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(real128t*) dst = (real128t) real_val;
#endif
	    else
	      goto error;
	  }
	else if (src_type == BT_COMPLEX)
	  {
	    if (dst_kind == 4)
	      *(float*) dst = (float) cmpx_val;
	    else if (dst_kind == 8)
	      *(double*) dst = (double) cmpx_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(long double*) dst = (long double) cmpx_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(real128t*) dst = (real128t) cmpx_val;
#endif
	    else
	      goto error;
	  }
	break;
     case BT_COMPLEX:
	if (src_type == BT_INTEGER)
	  {
	    if (dst_kind == 4)
	      *(_Complex float*) dst = (_Complex float) int_val;
	    else if (dst_kind == 8)
	      *(_Complex double*) dst = (_Complex double) int_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(_Complex long double*) dst = (_Complex long double) int_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(complex128t*) dst = (complex128t) int_val;
#endif
	    else
	      goto error;
	  }
	else if (src_type == BT_REAL)
	  {
	    if (dst_kind == 4)
	      *(_Complex float*) dst = (_Complex float) real_val;
	    else if (dst_kind == 8)
	      *(_Complex double*) dst = (_Complex double) real_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(_Complex long double*) dst = (_Complex long double) real_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(complex128t*) dst = (complex128t) real_val;
#endif
	    else
	      goto error;
	  }
	else if (src_type == BT_COMPLEX)
	  {
	    if (dst_kind == 4)
	      *(_Complex float*) dst = (_Complex float) cmpx_val;
	    else if (dst_kind == 8)
	      *(_Complex double*) dst = (_Complex double) cmpx_val;
#ifdef HAVE_GFC_REAL_10
	    else if (dst_kind == 10)
	      *(_Complex long double*) dst = (_Complex long double) cmpx_val;
#endif
#ifdef HAVE_GFC_REAL_16
	    else if (dst_kind == 16)
	      *(complex128t*) dst = (complex128t) cmpx_val;
#endif
	    else
	      goto error;
	  }
	else
	  goto error;
	break;
     default:
        goto error;
     }

error:
   fprintf (stderr, "RUNTIME ERROR: Cannot convert type %d kind "
            "%d to type %d kind %d\n", src_type, src_kind, dst_type, dst_kind);
   PREFIX (error_stop) (1);
}/* Get a scalar (or contiguous) data from remote image into a buffer.  */


void
PREFIX (get) (caf_token_t token, size_t offset,
	      int image_index __attribute__ ((unused)),
	      gfc_descriptor_t *src ,
	      caf_vector_t *src_vector __attribute__ ((unused)),
	      gfc_descriptor_t *dest, int src_kind, int dst_kind)
{
  /* FIXME: Handle vector subscripts; check whether strings of different
     kinds are permitted.  */
  size_t i, k, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);

  if (rank == 0)
    {
      void *sr = (void *) ((char *) TOKEN (token) + offset);
      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	memmove (dest->base_addr, sr,
		 dst_size > src_size ? src_size : dst_size);
      else
	convert_type (dest->base_addr, GFC_DESCRIPTOR_TYPE (dest),
		      dst_kind, sr, GFC_DESCRIPTOR_TYPE (src), src_kind);
      if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
	{
	  if (dst_kind == 1)
	    memset ((void*)(char*) dest->base_addr + src_size, ' ',
		    dst_size-src_size);
	  else /* dst_kind == 4.  */
	    for (i = src_size/4; i < dst_size/4; i++)
	      ((int32_t*) dest->base_addr)[i] = (int32_t) ' ';
	}
      return;
    }

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (size == 0)
    return;

  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset_dst = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      for (j = 0; j < rank-1; j++)
	{
	  array_offset_dst += ((i / (extent*stride))
			       % (dest->dim[j]._ubound
				  - dest->dim[j].lower_bound + 1))
			      * dest->dim[j]._stride;
	  extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
          stride = dest->dim[j]._stride;
	}
      array_offset_dst += (i / extent) * dest->dim[rank-1]._stride;
      void *dst = dest->base_addr + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);

      ptrdiff_t array_offset_sr = 0;
      stride = 1;
      extent = 1;
      for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
	{
	  array_offset_sr += ((i / (extent*stride))
			       % (src->dim[j]._ubound
				  - src->dim[j].lower_bound + 1))
			     * src->dim[j]._stride;
	  extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
	  stride = src->dim[j]._stride;
	}
      array_offset_sr += (i / extent) * src->dim[rank-1]._stride;
      void *sr = (void *)((char *) TOKEN (token) + offset
			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src));

      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	memmove (dst, sr, dst_size > src_size ? src_size : dst_size);
      else
	convert_type (dst, GFC_DESCRIPTOR_TYPE (dest), dst_kind,
		      sr, GFC_DESCRIPTOR_TYPE (src), src_kind);
      if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
	{
	  if (dst_kind == 1)
	    memset ((void*)(char*) dst + src_size, ' ', dst_size-src_size);
	  else /* dst_kind == 4.  */
	    for (k = src_size/4; k < dst_size/4; i++)
	      ((int32_t*) dst)[i] = (int32_t) ' ';
	}
    }
}


void
PREFIX (send) (caf_token_t token, size_t offset,
	       int image_index __attribute__ ((unused)),
	       gfc_descriptor_t *dest,
	       caf_vector_t *dst_vector __attribute__ ((unused)),
	       gfc_descriptor_t *src, int dst_kind,
	       int src_kind)
{
  /* FIXME: Handle vector subscripts; check whether strings of different
     kinds are permitted.  */
  size_t i, k, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);

  if (rank == 0)
    {
      void *dst = (void *) ((char *) TOKEN (token) + offset);
      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	memmove (dst, src->base_addr,
		 dst_size > src_size ? src_size : dst_size);
      else
	convert_type (dst, GFC_DESCRIPTOR_TYPE (dest), dst_kind, src->base_addr,
		      GFC_DESCRIPTOR_TYPE (src), src_kind);
      if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
	{
	  if (dst_kind == 1)
	    memset ((void*)(char*) dst + src_size, ' ', dst_size-src_size);
	  else /* dst_kind == 4.  */
	    for (i = src_size/4; i < dst_size/4; i++)
	      ((int32_t*) dst)[i] = (int32_t) ' ';
	}
      return;
    }

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (size == 0)
    return;

  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset_dst = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      for (j = 0; j < rank-1; j++)
	{
	  array_offset_dst += ((i / (extent*stride))
			       % (dest->dim[j]._ubound
				  - dest->dim[j].lower_bound + 1))
			      * dest->dim[j]._stride;
	  extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
          stride = dest->dim[j]._stride;
	}
      array_offset_dst += (i / extent) * dest->dim[rank-1]._stride;
      void *dst = (void *)((char *) TOKEN (token) + offset
			   + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      void *sr;
      if (GFC_DESCRIPTOR_RANK (src) != 0)
	{
	  ptrdiff_t array_offset_sr = 0;
	  stride = 1;
	  extent = 1;
	  for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
	    {
	      array_offset_sr += ((i / (extent*stride))
				  % (src->dim[j]._ubound
				     - src->dim[j].lower_bound + 1))
				 * src->dim[j]._stride;
	      extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
	      stride = src->dim[j]._stride;
	    }
	  array_offset_sr += (i / extent) * src->dim[rank-1]._stride;
	  sr = (void *)((char *) src->base_addr
			+ array_offset_sr*GFC_DESCRIPTOR_SIZE (src));
	}
      else
	sr = src->base_addr;

      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	memmove (dst, sr, dst_size > src_size ? src_size : dst_size);
      else
	convert_type (dst, GFC_DESCRIPTOR_TYPE (dest), dst_kind, sr,
		      GFC_DESCRIPTOR_TYPE (src), src_kind);
      if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
	{
	  if (dst_kind == 1)
	    memset ((void*)(char*) dst + src_size, ' ', dst_size-src_size);
	  else /* dst_kind == 4.  */
	    for (k = src_size/4; k < dst_size/4; i++)
	      ((int32_t*) dst)[i] = (int32_t)' ';
	}
    }
}


void
PREFIX (sendget) (caf_token_t dst_token, size_t dst_offset, int dst_image_index,
		  gfc_descriptor_t *dest, caf_vector_t *dst_vector,
		  caf_token_t src_token, size_t src_offset,
		  int src_image_index __attribute__ ((unused)),
		  gfc_descriptor_t *src,
		  caf_vector_t *src_vector __attribute__ ((unused)),
		  int dst_len, int src_len)
{
  /* FIXME: Handle vector subscript of 'src_vector'.  */
  /* For a single image, src->base_addr should be the same as src_token + offset
     but to play save, we do it properly.  */
  void *src_base = src->base_addr;
  src->base_addr = (void *) ((char *) TOKEN (src_token) + src_offset);
  PREFIX (send) (dst_token, dst_offset, dst_image_index, dest, dst_vector,
		 src, dst_len, src_len);
  src->base_addr = src_base;
}


void
PREFIX (co_sum) (gfc_descriptor_t *a __attribute__ ((unused)),
		 int result_image __attribute__ ((unused)), int *stat,
		 char *errmsg __attribute__ ((unused)),
		 int errmsg_len __attribute__ ((unused)))
{
  if (stat)
    *stat = 0;
}


void
PREFIX (co_min) (gfc_descriptor_t *a __attribute__ ((unused)),
		 int result_image __attribute__ ((unused)), int *stat,
		 char *errmsg __attribute__ ((unused)),
		 int src_len __attribute__ ((unused)),
		 int errmsg_len __attribute__ ((unused)))
{
  if (stat)
    *stat = 0;
}


void
PREFIX (co_max) (gfc_descriptor_t *a __attribute__ ((unused)),
		 int result_image __attribute__ ((unused)), int *stat,
		 char *errmsg __attribute__ ((unused)),
		 int src_len __attribute__ ((unused)),
		 int errmsg_len __attribute__ ((unused)))
{
  if (stat)
    *stat = 0;
}


void
PREFIX (sync_all) (int *stat,
		   char *errmsg __attribute__ ((unused)),
		   int errmsg_len __attribute__ ((unused)))
{
  if (stat)
    *stat = 0;
}


void
PREFIX (sync_images) (int count __attribute__ ((unused)),
		      int images[] __attribute__ ((unused)),
		      int *stat,
		      char *errmsg __attribute__ ((unused)),
		      int errmsg_len __attribute__ ((unused)))
{
#ifdef GFC_CAF_CHECK
  int i;

  for (i = 0; i < count; i++)
    if (images[i] != 1)
      {
	fprintf (stderr, "COARRAY ERROR: Invalid image index %d to SYNC "
		 "IMAGES", images[i]);
	exit (EXIT_FAILURE);
      }
#endif

  if (stat)
    *stat = 0;
}


void
PREFIX (error_stop_str) (const char *string, int32_t len)
{
  fputs ("ERROR STOP ", stderr);
  while (len--)
    fputc (*(string++), stderr);
  fputs ("\n", stderr);

  exit (1);
}


void
PREFIX(error_stop) (int32_t error)
{
  fprintf (stderr, "ERROR STOP %d\n", error);
  exit (error);
}
