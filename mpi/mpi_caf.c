/* One-sided MPI implementation of Libcaf

Copyright (c) 2012-2014, OpenCoarray Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the OpenCoarray Consortium nor the
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* For memcpy.  */
#include <stdarg.h>	/* For variadic arguments.  */
#include <alloca.h>
#include <mpi.h>

#include "libcaf.h"

/* Define GFC_CAF_CHECK to enable run-time checking.  */
/* #define GFC_CAF_CHECK  1  */

typedef MPI_Win *mpi_caf_token_t;
#define TOKEN(X) ((mpi_caf_token_t) (X))

static void error_stop (int error) __attribute__ ((noreturn));

/* Global variables.  */
static int caf_this_image;
static int caf_num_images;
static int caf_is_finalized;

#if MPI_VERSION >= 3
  MPI_Info mpi_info_same_size;
#endif

/*Sync image part*/

static int *orders;
MPI_Request *handlers;
static int *images_full;
static int *arrived;

caf_static_t *caf_static_list = NULL;
caf_static_t *caf_tot = NULL;

char err_buffer[MPI_MAX_ERROR_STRING];

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
  MPI_Finalize();

  /* Should be unreachable, but to make sure also call exit.  */
  exit (EXIT_FAILURE);
}


/* Initialize coarray program.  This routine assumes that no other
   GASNet initialization happened before. */

void
PREFIX (init) (int *argc, char ***argv)
{
  if (caf_num_images == 0)
    {
      int ierr = 0, i = 0, j = 0;

      MPI_Init(argc,argv);

      if (unlikely ((ierr != MPI_SUCCESS)))
	caf_runtime_error ("Failure when initializing MPI: %d", ierr);

      MPI_Comm_size(MPI_COMM_WORLD,&caf_num_images);
      MPI_Comm_rank(MPI_COMM_WORLD,&caf_this_image);

      caf_this_image++;
      caf_is_finalized = 0;

      images_full = (int *) calloc (caf_num_images-1, sizeof (int));

      for (i = 0; i < caf_num_images; i++)
	if (i + 1 != caf_this_image)
	  {
	    images_full[j] = i + 1;
	    j++;
	  }

      orders = calloc (caf_num_images, sizeof (int));
      arrived = calloc (caf_num_images, sizeof (int));

      handlers = malloc(caf_num_images * sizeof(MPI_Request));

#if MPI_VERSION >= 3
      MPI_Info_create (&mpi_info_same_size);
      MPI_Info_set (mpi_info_same_size, "same_size", "true");
#endif
    }
}


/* Finalize coarray program.   */

void
PREFIX (finalize) (void)
{

  MPI_Barrier(MPI_COMM_WORLD);

  while (caf_static_list != NULL)
    {
      caf_static_t *tmp = caf_static_list->prev;

      free (caf_static_list);
      caf_static_list = tmp;
    }

  caf_static_t *tmp_tot = caf_tot, *prev = caf_tot;
  MPI_Win *p;

  while(tmp_tot)
    {
      prev = tmp_tot->prev;
      p = tmp_tot->token;
      MPI_Win_free(p);
      free(tmp_tot);  
      tmp_tot = prev;
    }
#if MPI_VERSION >= 3
  MPI_Info_free (&mpi_info_same_size);
#endif

  MPI_Finalize();

  caf_is_finalized = 1;
}


int
PREFIX (this_image)(int distance __attribute__ ((unused)))
{
  return caf_this_image;
}


int
PREFIX (num_images)(int distance __attribute__ ((unused)),
                         int failed __attribute__ ((unused)))
{
  return caf_num_images;
}


void *
PREFIX (register) (size_t size, caf_register_t type, caf_token_t *token,
		  int *stat, char *errmsg, int errmsg_len)
{
  /* int ierr; */
  void *mem;

  if (unlikely (caf_is_finalized))
    goto error;

  /* Start GASNET if not already started.  */
  if (caf_num_images == 0)
    PREFIX (init) (NULL, NULL);

  /* Token contains only a list of pointers.  */

  *token = malloc (sizeof(MPI_Win));

#if MPI_VERSION >= 3
  MPI_Win_allocate(size, 1, mpi_info_same_size, MPI_COMM_WORLD,&mem, *token);
#else
  MPI_Alloc_mem(size, MPI_INFO_NULL, &mem);
  MPI_Win_create(mem, size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, *token);
#endif

  MPI_Win *p = *token;

  MPI_Win_fence(0, *p);

  caf_static_t *tmp = malloc (sizeof (caf_static_t));
  tmp->prev  = caf_tot;
  tmp->token = *token;
  caf_tot = tmp;

  if (type == CAF_REGTYPE_COARRAY_STATIC)
    {
      tmp = malloc (sizeof (caf_static_t));
      tmp->prev  = caf_static_list;
      tmp->token = *token;
      caf_static_list = tmp;
    }

  if (stat)
    *stat = 0;

  return mem;

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
  /* int ierr; */

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

  caf_static_t *tmp = caf_tot, *prev = caf_tot, *next=caf_tot;
  MPI_Win *p = *token;

  while(tmp)
    {
      prev = tmp->prev;
      
      if(tmp->token == *token)
	{
	  p = *token;
	  MPI_Win_free(p);
	  
	  if(prev)
	    next->prev = prev->prev;
	  else
	    next->prev = NULL;
	  
	  if(tmp == caf_tot)
	    caf_tot = prev;

	  free(tmp);
	  break;
	}
      
      next = tmp;
      tmp = prev;
    }
  
  if (stat)
    *stat = 0;

  /* if (unlikely (ierr = ARMCI_Free ((*token)[caf_this_image-1]))) */
  /*   caf_runtime_error ("ARMCI memory freeing failed: Error code %d", ierr); */
  //gasnet_exit(0);

  free (*token);
}


void
PREFIX (sync_all) (int *stat, char *errmsg, int errmsg_len)
{
  int ierr=0;

  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
      MPI_Win *p;
      caf_static_t *tmp, *next;
      for (tmp = caf_tot; tmp; )
	{
	  next = tmp->prev;
	  p = tmp->token;
#if MPI_VERSION >= 3
	  MPI_Win_flush_all (*p);
#else
	  MPI_Win_fence (0, *p);
#endif
          tmp = next;
	}
      MPI_Barrier(MPI_COMM_WORLD);
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

/* token: The token of the array to be written to. */
/* offset: Difference between the coarray base address and the actual data, used for caf(3)[2] = 8 or caf[4]%a(4)%b = 7. */
/* image_index: Index of the coarray (typically remote, though it can also be on this_image). */
/* data: Pointer to the to-be-transferred data. */
/* size: The number of bytes to be transferred. */
/* asynchronous: Return before the data transfer has been complete  */

void
PREFIX (send) (caf_token_t token, size_t offset, int image_index, void *data,
	      size_t size, bool async  __attribute__ ((unused)))
{
  int ierr = 0;

  MPI_Win *p = token;

  /* if(async==false) */
    MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
    ierr = MPI_Put (data, size, MPI_BYTE, image_index-1, offset, size,
		    MPI_BYTE, *p);
    MPI_Win_unlock (image_index-1, *p);
    //gasnet_put_bulk(image_index-1, tm[image_index-1]+offset, data, size);
  /* else */
  /*   ierr = ARMCI_NbPut(data,t.addr+offset,size,image_index-1,NULL); */
  if(ierr != 0)
    error_stop (ierr);
}

/* Send array data from src to dest on a remote image.  */

void
PREFIX (send_desc) (caf_token_t token, size_t offset, int image_index,
		    gfc_descriptor_t *dest, gfc_descriptor_t *src,
		    bool async  __attribute__ ((unused)))
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  MPI_Win *p = token;

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
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
      ierr = MPI_Put (src->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size, MPI_BYTE,
		      image_index-1, offset, GFC_DESCRIPTOR_SIZE (dest)*size,
		      MPI_BYTE, *p);
      MPI_Win_unlock (image_index-1, *p);
      if (ierr != 0)
	error_stop (ierr);
      return;
    }

  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
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
      array_offset_sr += (i / extent) * dest->dim[rank-1]._stride;

      ptrdiff_t dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);
      void *sr = (void *)((char *) src->base_addr
			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src));
      ierr = MPI_Put (sr, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index-1,
		      dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p);
      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}
    }
  MPI_Win_unlock (image_index-1, *p);
}


/* Send scalar data from src to array dest on a remote image.  */

void
PREFIX (send_desc_scalar) (caf_token_t token, size_t offset, int image_index,
			   gfc_descriptor_t *dest, void *buffer,
			   bool async  __attribute__ ((unused)))
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  MPI_Win *p = token;

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
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
      ptrdiff_t dst_offset = offset + array_offset*GFC_DESCRIPTOR_SIZE (dest);
      ierr = MPI_Put (buffer, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index-1,
		      dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p);
      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}
    }
  MPI_Win_unlock (image_index-1, *p);
}



void
PREFIX (get) (caf_token_t token, size_t offset, int image_index, void *data,
	      size_t size, bool async  __attribute__ ((unused)))
{
  int ierr = 0;
  MPI_Win *p = token;

  /* FIXME: Handle image_index == this_image().  */
/*  if (async == false) */
    {
      MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
      ierr = MPI_Get (data, size, MPI_BYTE, image_index-1, offset, size, MPI_BYTE, *p);
      MPI_Win_unlock (image_index-1, *p);
    }
  //gasnet_put_bulk(image_index-1, tm[image_index-1]+offset, data, size);
  /* else */
  /*   ierr = ARMCI_NbPut(data,t.addr+offset,size,image_index-1,NULL); */
  if(ierr != 0)
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
  MPI_Win *p = token;
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
      /* FIXME: Handle image_index == this_image().  */
      /*  if (async == false) */
	{
	  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
	  ierr = MPI_Get (dest->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size,
			  MPI_BYTE, image_index-1, offset,
			  GFC_DESCRIPTOR_SIZE (dest)*size, MPI_BYTE, *p);
	  MPI_Win_unlock (image_index-1, *p);
	}
      if (ierr != 0)
	error_stop (ierr);
      return;
    }

  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
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
      array_offset_sr += (i / extent) * dest->dim[rank-1]._stride;

      size_t sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src);
      void *dst = (void *) ((char *) dest->base_addr
			    + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      /* FIXME: Handle image_index == this_image().  */
      /*  if (async == false) */
	{
	  ierr = MPI_Get (dst, GFC_DESCRIPTOR_SIZE (dest)*size,
			  MPI_BYTE, image_index-1, sr_off, size, MPI_BYTE, *p);
	}
      if (ierr != 0)
	error_stop (ierr);
    }
  MPI_Win_unlock (image_index-1, *p);
}


/* SYNC IMAGES. Note: SYNC IMAGES(*) is passed as count == -1 while
   SYNC IMAGES([]) has count == 0. Note further that SYNC IMAGES(*)
   is not equivalent to SYNC ALL. */
void
PREFIX (sync_images) (int count, int images[], int *stat, char *errmsg,
		     int errmsg_len)
{
  int ierr = 0, i=0;

  MPI_Status s;

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

  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
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

       for(i = 0; i < count; i++)
	   ierr = MPI_Irecv(&arrived[images[i]-1], 1, MPI_INT, images[i]-1, 0, MPI_COMM_WORLD, &handlers[images[i]-1]);

       for(i=0; i < count; i++)
	 ierr = MPI_Send(&caf_this_image, 1, MPI_INT, images[i]-1, 0, MPI_COMM_WORLD);

       for(i=0; i < count; i++)
	 ierr = MPI_Wait(&handlers[images[i]-1], &s);

       memset(arrived,0,sizeof(int)*caf_num_images);

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


#define GEN_REDUCTION(name, datatype, operator) \
void \
name (datatype *invec, datatype *inoutvec, int *len, \
	       MPI_Datatype *datatype __attribute__ ((unused))) \
{ \
  int i; \
  for (i = 0; i < len; i++) \
    operator; \
}

#ifndef MPI_INTEGER1
GEN_REDUCTION (do_sum_int1, int8_t, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_int1, int8_t,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_int1, int8_t,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
#endif

#ifndef MPI_INTEGER2
GEN_REDUCTION (do_sum_int1, int16_t, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_int1, int16_t,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_int1, int16_t,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
#endif

#if defined(MPI_INTEGER16) && defined(GFC_INTEGER_16)
GEN_REDUCTION (do_sum_int1, GFC_INTEGER_16, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_int1, GFC_INTEGER_16,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_int1, GFC_INTEGER_16,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
#endif

#if defined(GFC_DTYPE_REAL_10) \
    || (!defined(GFC_DTYPE_REAL_10)  && defined(GFC_DTYPE_REAL_16))
GEN_REDUCTION (do_sum_real10, long double, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_real10, long double,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_real10, long double,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_sum_complex10, _Complex long double, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_complex10, _Complex long double,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_complex10, _Complex long double,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
#endif

#if defined(GFC_DTYPE_REAL_10) && defined(GFC_DTYPE_REAL_16)
GEN_REDUCTION (do_sum_real10, __float128, inoutvec[i] += invec[i])
GEN_REDUCTION (do_min_real10, __float128,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_real10, __float128,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_sum_complex10, _Complex __float128, inoutvec[i] += invec[i])
GEN_REDUCTION (do_mincomplexl10, _Complex __float128,
               inoutvec[i] = invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i])
GEN_REDUCTION (do_max_complex10, _Complex __float128,
               inoutvec[i] = invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i])
#endif
#undef GEN_REDUCTION


static MPI_Datatype
get_MPI_datatype (gfc_descriptor_t *desc)
{
  /* FIXME: Better check whether the sizes are okay and supported;
     MPI3 adds more types, e.g. MPI_INTEGER1.  */
  switch (GFC_DTYPE_TYPE_SIZE (desc))
    {
#ifdef MPI_INTEGER1
    case GFC_DTYPE_INTEGER_1:
      return MPI_INTEGER1;
#endif
#ifdef MPI_INTEGER2
    case GFC_DTYPE_INTEGER_2:
      return MPI_INTEGER2;
#endif
    case GFC_DTYPE_INTEGER_4:
#ifdef MPI_INTEGER4
      return MPI_INTEGER4;
#else
      return MPI_INTEGER;
#endif
#ifdef MPI_INTEGER8
    case GFC_DTYPE_INTEGER_8:
      return MPI_INTEGER8;
#endif
#ifdef MPI_INTEGER16
    case GFC_DTYPE_INTEGER_16:
      return MPI_INTEGER16;
#endif

    case GFC_DTYPE_REAL_4:
#ifdef MPI_REAL4
      return MPI_REAL4;
#else
      return MPI_REAL;
#endif
    case GFC_DTYPE_REAL_8:
#ifdef MPI_REAL8
      return MPI_REAL8;
#else
      return MPI_DOUBLE_PRECISION;
#endif

/* Note that we cannot use REAL_16 as we do not know whether it matches REAL(10)
   or REAL(16), which have both the same bitsize and only make use of less
   bits.  */
    case GFC_DTYPE_COMPLEX_4:
      return MPI_COMPLEX;
    case GFC_DTYPE_COMPLEX_8:
      return MPI_DOUBLE_COMPLEX;
    }
  caf_runtime_error ("Unsupported data type in collective\n");
  return 0;
}


static void
co_reduce_1 (MPI_Op op, gfc_descriptor_t *source,
	     gfc_descriptor_t *result, int result_image, int *stat,
	     char *errmsg, int errmsg_len)
{
  void *source2, *result2;
  size_t i, size;
  int j, ierr;
  int rank = GFC_DESCRIPTOR_RANK (source);

  MPI_Datatype datatype = get_MPI_datatype (source);

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = source->dim[j]._ubound
			    - source->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (rank == 0
      || (PREFIX (is_contiguous) (source)
	  && (!result || PREFIX (is_contiguous) (result))))
    {
      if (result_image == 0)
	{
	  source2 = result ? source->base_addr : MPI_IN_PLACE;
	  result2 = result ? result->base_addr : source->base_addr;
	  ierr = MPI_Allreduce (source2, result2, size, datatype, op,
				MPI_COMM_WORLD);
	}
      else if (!result && result_image == caf_this_image)
	ierr = MPI_Reduce (MPI_IN_PLACE, source->base_addr, size, datatype, op,
			   result_image-1, MPI_COMM_WORLD);
      else
	{
	  result2 = result ? result->base_addr : NULL;
	  ierr = MPI_Reduce (source->base_addr, result2, size, datatype, op,
			     result_image-1, MPI_COMM_WORLD);
	}
      if (ierr)
	goto error;
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

      if (result_image == 0)
	{
	  source2 = result ? sr : MPI_IN_PLACE;
	  result2 = result ? dst : sr;
	  ierr = MPI_Allreduce (source2, result2, 1, datatype, op,
				MPI_COMM_WORLD);
	}
      else if (!result && result_image == caf_this_image)
	ierr = MPI_Reduce (MPI_IN_PLACE, sr, 1, datatype, op,
			   result_image-1, MPI_COMM_WORLD);
      else
	ierr = MPI_Reduce (sr, dst, 1, datatype, op, result_image-1,
			   MPI_COMM_WORLD);
      if (ierr)
	goto error;
    }
  return;
error:
  /* FIXME: Put this in an extra function and use it elsewhere.  */
  if (stat)
    {
      *stat = ierr;
      if (!errmsg)
	return;
    }

  int len = sizeof (err_buffer);
  MPI_Error_string (ierr, err_buffer, &len);
  if (!stat)
    {
      err_buffer[len == sizeof (err_buffer) ? len-1 : len] = '\0';
      caf_runtime_error ("CO_SUM failed with %s\n", err_buffer);
    }
  memcpy (errmsg, err_buffer, errmsg_len > len ? len : errmsg_len);
  if (errmsg_len > len)
    memset (&errmsg[len], '\0', errmsg_len - len);
}


void
PREFIX (co_sum) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 (MPI_SUM, source, result, result_image, stat, errmsg, errmsg_len);
}


void
PREFIX (co_min) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 (MPI_MIN, source, result, result_image, stat, errmsg, errmsg_len);
}


void
PREFIX (co_max) (gfc_descriptor_t *source, gfc_descriptor_t *result,
                 int result_image, int *stat, char *errmsg, int errmsg_len)
{
  co_reduce_1 (MPI_MAX, source, result, result_image, stat, errmsg, errmsg_len);
}


/* ERROR STOP the other images.  */

static void
error_stop (int error)
{
  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  /* FIXME: Do some more effort than just gasnet_exit().  */
  MPI_Abort(MPI_COMM_WORLD, error);

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
