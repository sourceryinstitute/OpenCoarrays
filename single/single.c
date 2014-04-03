/* ARMCI implementation of Libcaf

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
PREFIX(finalize) (void)
{
  while (caf_static_list != NULL)
    {
      caf_static_t *tmp = caf_static_list->prev;
      free (TOKEN(caf_static_list->token));
      free (caf_static_list);
      caf_static_list = tmp;
    }
}


int
PREFIX(this_image) (int distance __attribute__ ((unused)))
{
  return 1;
}


int
PREFIX(num_images) (int distance __attribute__ ((unused)),
		    int failed __attribute__ ((unused)))
{
  return 1;
}


void *
PREFIX(register) (size_t size, caf_register_t type, caf_token_t *token,
		  int *stat, char *errmsg, int errmsg_len)
{
  void *local;

  local = malloc (size);
  *token = malloc (sizeof (single_token_t));

  if (unlikely (local == NULL || TOKEN(*token) == NULL))
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
PREFIX(deregister) (caf_token_t *token, int *stat,
		    char *errmsg __attribute__ ((unused)),
		    int errmsg_len __attribute__ ((unused)))
{
  free (TOKEN(*token));

  if (stat)
    *stat = 0;
}


/* Send scalar (or contiguous) data from buffer to a remote image.  */

void
PREFIX(send) (caf_token_t token, size_t offset,
	      int image_id __attribute__ ((unused)),
	      void *buffer, size_t size, bool asyn __attribute__ ((unused)))
{
    void *dest = (void *) ((char *) TOKEN(token) + offset);
    memmove (dest, buffer, size);
}


/* Send array data from src to dest on a remote image.  */

void
PREFIX (send_desc) (caf_token_t token, size_t offset,
		    int image_id __attribute__ ((unused)),
		    gfc_descriptor_t *dest, gfc_descriptor_t *src,
		    bool asyn __attribute__ ((unused)))
{
  fprintf (stderr, "COARRAY ERROR: Array communication "
	   "[send_desc] not yet implemented for rank /= 0");
  exit (EXIT_FAILURE);
  size_t i, j;
  size_t size = GFC_DESCRIPTOR_SIZE (dest);
  int rank = GFC_DESCRIPTOR_RANK (dest);

  if (rank != 1)
    {
      fprintf (stderr, "COARRAY ERROR: Array communication "
	       "[_gfortran_caf_send_desc] not yet implemented for rank /= 0");
      exit (EXIT_FAILURE);
    }
  
  for (j = dest->dim[0].lower_bound - dest->offset,
       i = src->dim[0].lower_bound - src->offset;
       j <= dest->dim[0]._ubound - dest->offset
       && i <= src->dim[0]._ubound - src->offset;
       j += dest->dim[0]._stride,
       i += src->dim[0]._stride)
    {
      void *dst = (void *)((char *) TOKEN(token) + offset + j*size);
      void *sr = (void *)((char *)src->base_addr + j*size);
      memmove (dst, sr, size);
    }
}


/* Send scalar data from src to array dest on a remote image.  */

void
PREFIX (send_desc_scalar) (caf_token_t token, size_t offset,
			   int image_id __attribute__ ((unused)),
			   gfc_descriptor_t *dest, void *buffer,
			   bool asyn __attribute__ ((unused)))
{
  size_t j;
  size_t size = GFC_DESCRIPTOR_SIZE (dest);
  int rank = GFC_DESCRIPTOR_RANK (dest);

  if (rank != 1)
    {
      fprintf (stderr, "COARRAY ERROR: Array communication "
	       "[send_desc_scalar] not yet implemented for "
	       "rank /= 0");
      exit (EXIT_FAILURE);
    }
  
  for (j = dest->dim[0].lower_bound - dest->offset;
       j <= dest->dim[0]._ubound - dest->offset;
       j += dest->dim[0]._stride)
    {
      void *dst = (void *)((char *) TOKEN(token) + offset + j*size);
      memmove (dst, buffer, size);
    }
}


void
PREFIX(sync_all) (int *stat,
		  char *errmsg __attribute__ ((unused)),
		  int errmsg_len __attribute__ ((unused)))
{
  if (stat)
    *stat = 0;
}


void
PREFIX(sync_images) (int count __attribute__ ((unused)),
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
PREFIX(error_stop_str) (const char *string, int32_t len)
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
