/* ARMCI implementation of Libcaf

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

/****l* armci/armci_caf.c
 * NAME
 *   armci_caf
 * SYNOPSIS
 *   This program implements the LIBCAF_ARMCI transport layer.  This
 *   library is incomplete and unsupported. It exists to serve as a
 *   starting point for potential future development.
******
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* For memcpy.  */
#include <stdarg.h>	/* For variadic arguments.  */
#include <sched.h>	/* For sched_yield.  */
#include <message.h>    /* ARMCI and armci_msg_*.  */
#include <complex.h>

#include "libcaf.h"


/* Define GFC_CAF_CHECK to enable run-time checking.  */
/* #define GFC_CAF_CHECK  1  */

typedef void ** armci_token_t;
#define TOKEN(X) ((armci_token_t) (X))


static void error_stop (int error) __attribute__ ((noreturn));

/* Global variables.  */
static int caf_this_image;
static int caf_num_images;
static int caf_is_finalized;

caf_static_t *caf_static_list = NULL;

static int **arrived;
static int *orders;
static int sizeOrders = 0;
static int *images_full;

/* Keep in sync with single.c.  */
static void
caf_runtime_error (const char *message, ...)
{
  va_list ap;
  fprintf (stderr, "Fortran runtime error on image %d: ", caf_this_image);
  va_start (ap, message);
  vfprintf (stderr, message, ap);
  va_end (ap);
  fprintf (stderr, "\n");

  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  /* FIXME: Do some more effort than just to abort.  */
  armci_msg_abort (EXIT_FAILURE);

  /* Should be unreachable, but to make sure also call exit.  */
  exit (EXIT_FAILURE);
}


/* Initialize coarray program.  This routine assumes that no other
   ARMCI initialization happened before. */

void
PREFIX (init) (int *argc, char ***argv)
{
  int ierr=0, i = 0, j = 0;

  if (caf_num_images != 0)
    return;  /* Already initialized.  */

  armci_msg_init (argc, argv);
  if (unlikely ((ierr = ARMCI_Init()) != 0))
    caf_runtime_error ("Failure when initializing ARMCI: %d", ierr);

  caf_num_images = armci_msg_nproc ();
  caf_this_image = armci_msg_me ();
  caf_this_image++;
  caf_is_finalized = 0;

  images_full = (int *) calloc (caf_num_images-1, sizeof (int));

  ierr = ARMCI_Create_mutexes (1);

  for (i = 0; i < caf_num_images; i++)
    if (i + 1 != caf_this_image)
      {
	images_full[j] = i + 1;
	j++;
      }

  orders = calloc (caf_num_images, sizeof (int));

  arrived = malloc(sizeof (int *) * caf_num_images);

  ierr = ARMCI_Malloc ((void **) arrived, sizeof (int) * caf_num_images);

  for (i = 0; i < caf_num_images; i++)
    arrived[caf_this_image-1][i] = 0;
}


/* Finalize coarray program.   */

void
PREFIX (finalize) (void)
{
  while (caf_static_list != NULL)
    {
      caf_static_t *tmp = caf_static_list->prev;

      (void) ARMCI_Free (TOKEN (caf_static_list->token)[caf_this_image-1]);
      free (TOKEN (caf_static_list->token));
      free (caf_static_list);
      caf_static_list = tmp;
    }

  (void) ARMCI_Finalize ();
  armci_msg_finalize ();

  caf_is_finalized = 1;
}


int
PREFIX (this_image) (int distance __attribute__ ((unused)))
{
  return caf_this_image;
}


int
PREFIX (num_images) (int distance __attribute__ ((unused)),
		     int failed __attribute__ ((unused)))
{
  return caf_num_images;
}


void *
PREFIX (register) (size_t size, caf_register_t type, caf_token_t *token,
		   int *stat, char *errmsg, int errmsg_len)
{
  int ierr = 0;

  if (unlikely (caf_is_finalized))
    goto error;

  /* Start ARMCI if not already started.  */
  if (caf_num_images == 0)
    PREFIX (init) (NULL, NULL);

  *token = malloc (sizeof (armci_token_t));

  if (*token == NULL)
    goto error;

  *token = malloc (sizeof (void*) * caf_num_images);
  if (TOKEN (*token) == NULL)
    goto error;

  ierr = ARMCI_Malloc (TOKEN (*token), size);

  if (unlikely (ierr))
    {
      free (TOKEN (*token));
      goto error;
    }

  if (type == CAF_REGTYPE_COARRAY_STATIC)
    {
      caf_static_t *tmp = malloc (sizeof (caf_static_t));
      tmp->prev  = caf_static_list;
      tmp->token = *token;
      caf_static_list = tmp;
    }

  if (stat)
    *stat = 0;

  return TOKEN (*token)[caf_this_image-1];

error:
  {
    char *msg;

    if (caf_is_finalized)
      msg = "Failed to allocate coarray - there are stopped images";
    else
      msg = "Failed to allocate coarray";

    if (stat)
      {
	*stat = caf_is_finalized ? STAT_STOPPED_IMAGE : 1;
	if (errmsg_len > 0)
	  {
	    int len = ((int) strlen (msg) > errmsg_len) ? errmsg_len
							: (int) strlen (msg);
	    memcpy (errmsg, msg, len);
	    if (errmsg_len > len)
	      memset (&errmsg[len], ' ', errmsg_len-len);
	  }
      }
    else
      caf_runtime_error (msg);
  }

  return NULL;
}


void
PREFIX (deregister) (caf_token_t *token, int *stat, char *errmsg, int errmsg_len)
{
  int ierr=0;

  if (unlikely (caf_is_finalized))
    {
      const char msg[] = "Failed to deallocate coarray - "
			  "there are stopped images";
      if (stat)
	{
	  *stat = STAT_STOPPED_IMAGE;

	  if (errmsg_len > 0)
	    {
	      int len = ((int) sizeof (msg) - 1 > errmsg_len)
			? errmsg_len : (int) sizeof (msg) - 1;
	      memcpy (errmsg, msg, len);
	      if (errmsg_len > len)
		memset (&errmsg[len], ' ', errmsg_len-len);
	    }
	  return;
	}
      caf_runtime_error (msg);
    }

  PREFIX (sync_all) (NULL, NULL, 0);

  if (stat)
    *stat = 0;

  if (unlikely (ierr = ARMCI_Free (TOKEN (*token)[caf_this_image-1])))
    caf_runtime_error ("ARMCI memory freeing failed: Error code %d", ierr);

  free (TOKEN (*token));
}


void
PREFIX (sync_all) (int *stat, char *errmsg, int errmsg_len)
{
  int ierr=0;

  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
      ARMCI_AllFence ();
      armci_msg_barrier ();
      ierr = 0;
    }

  if (stat)
    *stat = ierr;

  if (ierr)
    {
      char *msg;
      if (caf_is_finalized)
	msg = "SYNC ALL failed - there are stopped images";
      else
	msg = "SYNC ALL failed";

      if (errmsg_len > 0)
	{
	  int len = ((int) strlen (msg) > errmsg_len) ? errmsg_len
						      : (int) strlen (msg);
	  memcpy (errmsg, msg, len);
	  if (errmsg_len > len)
	    memset (&errmsg[len], ' ', errmsg_len-len);
	}
      else
	caf_runtime_error (msg);
    }
}


/* Send scalar (or contiguous) data from buffer to a remote image.  */

/* token: The token of the array to be written to. */
/* offset: Difference between the coarray base address and the actual data, used for caf(3)[2] = 8 or caf[4]%a(4)%b = 7. */
/* image_index: Index of the coarray (typically remote, though it can also be on this_image). */
/* data: Pointer to the to-be-transferred data. */
/* size: The number of bytes to be transferred. */
/* asynchronous: Return before the data transfer has been complete  */

void
PREFIX (send) (caf_token_t token, size_t offset, int image_index, void *data,
	       size_t size, bool async)
{
  int ierr=0;

  if (unlikely (size == 0))
    return;  /* Zero-sized array.  */

  if (image_index == caf_this_image)
    {
       void *dest = (void *) ((char *) TOKEN (token)[image_index-1] + offset);
       memmove (dest, data, size);
       return;
    }

  if (!async)
    ierr = ARMCI_Put (data, TOKEN (token)[image_index-1] + offset, size,
		      image_index - 1);
  else
    ierr = ARMCI_NbPut (data, TOKEN (token)[image_index-1] + offset, size,
			image_index-1, NULL);

  if (ierr != 0)
    error_stop (ierr);
}


/* Send array data from src to dest on a remote image.  */

void
PREFIX (send_desc) (caf_token_t token, size_t offset, int image_index,
		    gfc_descriptor_t *dest, gfc_descriptor_t *src, bool async)
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);

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

  if (PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src))
    {
      void *dst = (void *)((char *) TOKEN (token)[image_index-1] + offset);

      if (image_index == caf_this_image)
	memmove (dst, src->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size);
      else if (!async)
	ierr = ARMCI_Put (src->base_addr, dst, GFC_DESCRIPTOR_SIZE (dest)*size,
			  image_index - 1);
      else
	ierr = ARMCI_NbPut (src->base_addr, dst,
			    GFC_DESCRIPTOR_SIZE (dest)*size,
			    image_index-1, NULL);
      if (ierr != 0)
	error_stop (ierr);
      return;
    }

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

      void *dst = (void *)((char *) TOKEN (token)[image_index-1] + offset
			   + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      void *sr = (void *)((char *) src->base_addr
			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src));
      if (image_index == caf_this_image)
	memmove (dst, sr, GFC_DESCRIPTOR_SIZE (dest));
      else if (!async)
        ierr = ARMCI_Put (sr, dst, GFC_DESCRIPTOR_SIZE (dest), image_index - 1);
      else
        ierr = ARMCI_NbPut (sr, dst, GFC_DESCRIPTOR_SIZE (dest),
			    image_index - 1, NULL);
      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}
    }
}


/* Send scalar data from src to array dest on a remote image.  */

void
PREFIX (send_desc_scalar) (caf_token_t token, size_t offset, int image_index,
			   gfc_descriptor_t *dest, void *buffer, bool async)
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      for (j = 0; j < rank-1; j++)
	{
	  array_offset += ((i / (extent*stride))
			   % (dest->dim[j]._ubound
			      - dest->dim[j].lower_bound + 1))
			  * dest->dim[j]._stride;
	  extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
          stride = dest->dim[j]._stride;
	}
      array_offset += (i / extent) * dest->dim[rank-1]._stride;
      void *dst = (void *)((char *) TOKEN (token)[image_index-1] + offset
			   + array_offset*GFC_DESCRIPTOR_SIZE (dest));
      if (image_index == caf_this_image)
	memmove (dst, buffer, GFC_DESCRIPTOR_SIZE (dest));
      else if (!async)
        ierr = ARMCI_Put (buffer, dst, GFC_DESCRIPTOR_SIZE (dest),
			  image_index - 1);
      else
        ierr = ARMCI_NbPut (buffer, dst, GFC_DESCRIPTOR_SIZE (dest),
			    image_index-1, NULL);
      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}
    }
}


void
PREFIX (get) (caf_token_t token, size_t offset, int image_index, void *data,
	      size_t size, bool async)
{
  int ierr = 0;

  if (unlikely (size == 0))
    return;  /* Zero-sized array.  */

  if (image_index == caf_this_image)
    memmove (data, TOKEN (token)[image_index-1] + offset, size);
  else if (async == false)
    ierr = ARMCI_Get (TOKEN (token)[image_index-1] + offset, data, size,
		      image_index - 1);
  else
    ierr = ARMCI_NbGet (TOKEN (token)[image_index-1] + offset, data, size,
			image_index - 1, NULL);

  if (ierr != 0)
    error_stop (ierr);
}


/* Get array data from a remote src to a local dest.  */

void
PREFIX (get_desc) (caf_token_t token, size_t offset, int image_index,
		   gfc_descriptor_t *src, gfc_descriptor_t *dest,
		   bool async __attribute__ ((unused)))
{
  size_t i, size;
  int ierr = 0;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);

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

  if (PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src))
    {
      void *sr = (void *) ((char *) TOKEN(token)[image_index-1] + offset);
      if (image_index == caf_this_image)
	memmove (dest->base_addr, sr, GFC_DESCRIPTOR_SIZE (dest)*size);
      else if (async == false)
	ierr = ARMCI_Get (sr, dest->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size,
			  image_index - 1);
      else
	ierr = ARMCI_NbGet (sr, dest->base_addr,
			    GFC_DESCRIPTOR_SIZE (dest)*size, image_index - 1,
			    NULL);
      if (ierr != 0)
	error_stop (ierr);
      return;
    }

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

      void *sr = (void *)((char *) TOKEN (token)[image_index-1] + offset
			   + array_offset_sr*GFC_DESCRIPTOR_SIZE (src));
      void *dst = (void *)((char *) dest->base_addr
			  + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      if (image_index == caf_this_image)
	memmove (dst, sr, GFC_DESCRIPTOR_SIZE (dest));
      else if (async == false)
	ierr = ARMCI_Get (sr, dst, GFC_DESCRIPTOR_SIZE (dest), image_index - 1);
      else
	ierr = ARMCI_NbGet (sr, dst, GFC_DESCRIPTOR_SIZE (dest),
			    image_index - 1, NULL);
      if (ierr != 0)
	error_stop (ierr);
    }
}


/* SYNC IMAGES. Note: SYNC IMAGES(*) is passed as count == -1 while
   SYNC IMAGES([]) has count == 0. Note further that SYNC IMAGES(*)
   is not equivalent to SYNC ALL. */

void
PREFIX (sync_images) (int count, int images[], int *stat, char *errmsg,
		      int errmsg_len)
{
  int i, ierr=0;
  bool freeToGo = false;

  if (count == 0 || (count == 1 && images[0] == caf_this_image))
    {
      if (stat)
	*stat = 0;
      return;
    }

#ifdef GFC_CAF_CHECK
  {
    for (i = 0; i < count; i++)
      if (images[i] < 1 || images[i] > caf_num_images)
	{
	  fprintf (stderr, "COARRAY ERROR: Invalid image index %d to SYNC "
		   "IMAGES", images[i]);
	  error_stop (1);
	}
  }
#endif

  /* FIXME: SYNC IMAGES with a nontrivial argument cannot easily be
     mapped to ARMCI communicators. Thus, exist early with an error message.  */

  /* Handle SYNC IMAGES(*).  */
  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
      /* Insert orders.  */
      if(count == -1)
	{
	  for (i = 0; i < caf_num_images - 1; i++)
	    orders[images_full[i]-1]++;
	  count = caf_num_images-1;
	  images = images_full;
	}
      else
	{
	  for (i = 0; i < count; i++)
	    orders[images[i]-1]++;
	}

      /* Sending ack.  */

      int val;

      for (i = 0; i < count; i++)
	{
	  ARMCI_Lock (0, images[i]-1);

	  val = ARMCI_GetValueInt (
			(void *) &arrived[images[i]-1][caf_this_image-1],
			images[i]-1);
	  val++;
	  ierr = ARMCI_PutValueInt (
			val, (void *) &arrived[images[i]-1][caf_this_image-1],
			images[i]-1);
	  ARMCI_Unlock (0, images[i]-1);
	}

      while (!freeToGo)
	{
	  ARMCI_Lock (0, caf_this_image-1);

	  sizeOrders = 0;

	  for (i = 0; i < caf_num_images; i++)
	    {
	      if (orders[i] != 0)
		{
		  sizeOrders++;
		  val = ARMCI_GetValueInt (
				(void *) &arrived[caf_this_image-1][i],
				caf_this_image-1);
		  /* val = arrived[caf_this_image-1][i]; */
		  if (val != 0)
		    {
		      orders[i]--;
		      sizeOrders--;
		      val--;
		      ierr = ARMCI_PutValueInt (
				val, (void *) &arrived[caf_this_image-1][i],
				caf_this_image-1);
		    }
		}
	    }

	  if (sizeOrders == 0)
	    freeToGo=true;

	  ARMCI_Unlock (0, caf_this_image-1);
	  sched_yield ();
	}

      freeToGo = false;
    }

  if (stat)
    *stat = ierr;

  if (ierr)
    {
      char *msg;
      if (caf_is_finalized)
	msg = "SYNC IMAGES failed - there are stopped images";
      else
	msg = "SYNC IMAGES failed";

      if (errmsg_len > 0)
	{
	  int len = ((int) strlen (msg) > errmsg_len) ? errmsg_len
						      : (int) strlen (msg);
	  memcpy (errmsg, msg, len);
	  if (errmsg_len > len)
	    memset (&errmsg[len], ' ', errmsg_len-len);
	}
      else
	caf_runtime_error (msg);
    }
}

#if 0

/* FIXME: The following needs to be fixed - in particular for result_image > 0;
   It is unclear what's the difference between armci_msg_igop and
   armci_msg_reduce; in particular, how to state that the result is only saved
   on a certain image?  */
static void
co_reduce_2 (char *op, int result_image, gfc_descriptor_t *source,
             gfc_descriptor_t *result, void *sr, void *dst,
	     size_t size)
{
  void *arg;
  if (result && GFC_DESCRIPTOR_TYPE (source) != BT_COMPLEX)
    memmove (dst, sr, GFC_DESCRIPTOR_SIZE (source)*size);
  else
    arg = sr;

  if (result_image == 0)
    switch (GFC_DESCRIPTOR_TYPE (source))
      {
      BT_INTEGER:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (int))
	  armci_msg_igop (arg, size, op);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (long))
	  armci_msg_lgop (arg, size, op);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (long long))
	  armci_msg_llgop (arg, size, op);
        else
	  goto error;
	break;
      BT_REAL:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (float))
	  armci_msg_fgop (arg, size, op);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (double))
	  armci_msg_dgop (arg, size, op);
        else
	  goto error;
	break;
      BT_COMPLEX:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (float) && size == 1)
	  {
	    float re = __real__ *(_Complex float*) sr;
	    float im = __imag__ *(_Complex float*) sr;
	    armci_msg_fgop (&re, 1, op);
	    armci_msg_fgop (&im, 1, op);
	    if (result)
	      *(_Complex float*) dst = re + im * _Complex_I;
	    else
	      *(_Complex float*) sr = re + im * _Complex_I;
	  }
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (double) && size == 1)
       	  {
	    double re = __real__ *(_Complex double*) sr;
	    double im = __imag__ *(_Complex double*) sr;
	    armci_msg_dgop (&re, 1, op);
	    armci_msg_dgop (&im, 1, op);
	    if (result)
	      *(_Complex double*) dst = re + im * _Complex_I;
	    else
	      *(_Complex double*) sr = re + im * _Complex_I;
	  }
	else
	  goto error;
	break;
      default:
	goto error;
      }
  else
    switch (GFC_DESCRIPTOR_TYPE (source))
      {
      BT_INTEGER:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (int))
	  armci_msg_reduce(arg, size, op, ARMCI_INT, result_image-1);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (long))
	  armci_msg_reduce(arg, size, op, ARMCI_LONG, result_image-1);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (long long))
	  armci_msg_reduce(arg, size, op, ARMCI_LONG_LONG, result_image-1);
        else
	  goto error;
	break;
      BT_REAL:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (float))
	  armci_msg_reduce(arg, size, op, ARMCI_FLOAT, result_image-1);
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (double))
	  armci_msg_reduce(arg, size, op, ARMCI_DOUBLE, result_image-1);
        else
	  goto error;
	break;
      BT_COMPLEX:
	if (GFC_DESCRIPTOR_SIZE (source) == sizeof (float) && size == 1)
	  {
	    double re = __real__ *(_Complex double*) sr;
	    double im = __imag__ *(_Complex double*) sr;
	    armci_msg_reduce(&re, 1, op, ARMCI_FLOAT, result_image-1);
	    armci_msg_reduce(&im, 1, op, ARMCI_FLOAT, result_image-1);
	    if (result)
	      *(_Complex double*) dst = re + im * _Complex_I;
	    else
	      *(_Complex double*) sr = re + im * _Complex_I;
	  }
	else if (GFC_DESCRIPTOR_SIZE (source) == sizeof (double) && size == 1)
  	  {
	    double re = __real__ *(_Complex double*) sr;
	    double im = __imag__ *(_Complex double*) sr;
	    armci_msg_reduce(&re, 1, op, ARMCI_DOUBLE, result_image-1);
	    armci_msg_reduce(&im, 1, op, ARMCI_DOUBLE, result_image-1);
	    if (result)
	      *(_Complex double*) dst = re + im * _Complex_I;
	    else
	      *(_Complex double*) sr = re + im * _Complex_I;
	  }
	else
	  goto error;
	break;
      default:
	goto error;
      }
  return;
error:
    /* FIXME: Handle the other data types as well.  */
    caf_runtime_error ("Unsupported data type in collective\n");
}


static void
co_reduce_1 (char *op, gfc_descriptor_t *source,
	     gfc_descriptor_t *result, int result_image, int *stat,
	     char *errmsg, int errmsg_len)
{
  void *source2, *result2;
  size_t i, size;
  int j, ierr;
  int rank = GFC_DESCRIPTOR_RANK (source);

  if (stat)
    *stat = 0;

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = source->dim[j]._ubound
			    - source->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (size = 1
      || (GFC_DESCRIPTOR_TYPE (source) != BT_COMPLEX
	  && PREFIX (is_contiguous) (source)
	  && (!result || PREFIX (is_contiguous) (result))))
    {
      source2 = source->base_addr;
      result2 = result ? result->base_addr : NULL;
      co_reduce_2 (op, result_image, source, result, source2, result2, size);
      return;
    }

  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset_sr = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      for (j = 0; j < GFC_DESCRIPTOR_RANK (source)-1; j++)
	{
	  array_offset_sr += ((i / (extent*stride))
			   % (source->dim[j]._ubound
			      - source->dim[j].lower_bound + 1))
			  * source->dim[j]._stride;
	  extent = (source->dim[j]._ubound - source->dim[j].lower_bound + 1);
          stride = source->dim[j]._stride;
	}
      array_offset_sr += (i / extent) * source->dim[rank-1]._stride;
      void *sr = (void *)((char *) source->base_addr
			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (source));
      void *dst = NULL;
      if (result)
	{
	  ptrdiff_t array_offset_dst = 0;
	  stride = 1;
	  extent = 1;
	  for (j = 0; j < rank-1; j++)
	    {
	      array_offset_dst += ((i / (extent*stride))
				   % (result->dim[j]._ubound
				   - result->dim[j].lower_bound + 1))
				  * result->dim[j]._stride;
	      extent = (result->dim[j]._ubound - result->dim[j].lower_bound + 1);
	      stride = result->dim[j]._stride;
	    }
	  array_offset_dst += (i / extent) * result->dim[rank-1]._stride;
	  dst = (void *)((char *) result->base_addr
			 + array_offset_dst*GFC_DESCRIPTOR_SIZE (source));
	}

      result2 = result ? dst : NULL;
      co_reduce_2 (op, result_image, source, result, sr, result2, 1);
    }
  return;
}


void
PREFIX (co_sum) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 ("+", source, result, result_image, stat, errmsg, errmsg_len);
}


void
PREFIX (co_min) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 ("min", source, result, result_image, stat, errmsg, errmsg_len);
}


void
PREFIX (co_max) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 ("max", source, result, result_image, stat, errmsg, errmsg_len);
}
#endif


/* ERROR STOP the other images.  */

static void
error_stop (int error)
{
  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  /* FIXME: Do some more effort than just ARMCI_Error.  */
//  ARMCI_Error ("Aborting calculation", error);
  ARMCI_Error (NULL, error);

  /* Should be unreachable, but to make sure also call exit.  */
  exit (error);
}


/* ERROR STOP function for string arguments.  */

void
PREFIX (error_stop_str) (const char *string, int32_t len)
{
  fputs ("ERROR STOP ", stderr);
  while (len--)
    fputc (*(string++), stderr);
  fputs ("\n", stderr);

  error_stop (1);
}


/* ERROR STOP function for numerical arguments.  */

void
PREFIX (error_stop) (int32_t error)
{
  fprintf (stderr, "ERROR STOP %d\n", error);
  error_stop (error);
}
