/* GASNet implementation of Libcaf

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

/****l* gasnet/gasnet_caf.c
 * NAME
 *   gasnet_caf
 * SYNOPSIS
 *   This program implements the LIBCAF_GASNET transport layer.
******
*/


#include "libcaf.h"
#include "gasnet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* For memcpy.  */
#include <stdarg.h>	/* For variadic arguments.  */
#include <alloca.h>
#ifdef STRIDED
#include "gasnet_vis.h"
#endif

#ifndef MALLOC_ALIGNMENT
#  define MALLOC_ALIGNMENT   (2 *sizeof(size_t) < __alignof__ (long double)   \
                              ? __alignof__ (long double) : 2*sizeof(size_t))
#endif

/* Define GFC_CAF_CHECK to enable run-time checking.  */
/* #define GFC_CAF_CHECK  1  */

typedef gasnet_seginfo_t * gasnet_caf_token_t;
#define TOKEN(X) ((gasnet_caf_token_t) (X))

#define send_notify1_handler   201

static void error_stop (int error) __attribute__ ((noreturn));

/* Global variables.  */
static int caf_this_image;
static int caf_num_images;
static int caf_is_finalized;

/*Sync image part*/
static int sizeOrders = 0;
static int *orders;
static int *arrived;
static int *images_full;

caf_static_t *caf_static_list = NULL;
static size_t r_pointer;
static void *remote_memory = NULL;
static long remoteMemorySize = 0;


static bool
freeToGo ()
{
  int i = 0;
  bool ret = false;

  gasnet_hold_interrupts ();

  sizeOrders = 0;

  for (i = 0; i < caf_num_images; i++)
    {
      if(orders[i] != 0)
	{
	  sizeOrders++;
	  if(arrived[i]!=0)
	    {
	      orders[i]--;
	      sizeOrders--;
	      arrived[i]--;
	    }
	}
    }

  if (sizeOrders == 0)
    ret = true;

  gasnet_resume_interrupts ();

  return ret;
}


static void
initImageSync()
{
  int i=0,j=0;
  orders = (int *)calloc(caf_num_images,sizeof(int));
  arrived = (int *)calloc(caf_num_images,sizeof(int));
  images_full = (int *)calloc(caf_num_images-1,sizeof(int));

  for(i=0;i<caf_num_images;i++)
    {
      if(i+1 != caf_this_image)
	{
	  images_full[j]=i+1;
	  j++;
	}
    }
}

static void
insOrders (int *images, int n)
{
  int i = 0;

  for (i = 0; i < n; i++)
    {
      orders[images[i]-1]++;
      /* printf("Process: %d order: %d\n",caf_this_image,images[i]); */
      gasnet_AMRequestShort1 (images[i]-1, send_notify1_handler, caf_this_image);
    }
}

static void
insArrived (int image)
{
  arrived[image-1]++;
  /* printf("Process: %d arrived: %d\n",caf_this_image,image); */
}


static void
req_notify1_handler (gasnet_token_t token, int proc)
{
  insArrived (proc);
  /* gasnet_AMReplyShort1 (token, recv_notify1_handler, caf_this_image-1);  */
}


#if 0
static void
rec_notify1_handler (gasnet_token_t token, int proc)
 {
 }
#endif


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
  gasnet_exit(EXIT_FAILURE);

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
      int ierr=0;

      if (argc == NULL)
	{
	  /* GASNet hat the bug that it expects that argv[0] is always
	     available.  Provide a summy argument.  */
	  int i = 0;
	  char *tmpstr = "CAF";
          char **strarray = alloca (sizeof (char *));
          strarray[0] = tmpstr;
	  ierr = gasnet_init (&i, &strarray);
	}
      else
	ierr = gasnet_init (argc, argv);

      if (unlikely ((ierr != GASNET_OK)))
	caf_runtime_error ("Failure when initializing GASNet: %d", ierr);

      caf_num_images = gasnet_nodes ();
      caf_this_image = gasnet_mynode ();

      caf_this_image++;
      caf_is_finalized = 0;

      initImageSync ();

      char *envvar = NULL;

      envvar = getenv ("GASNET_NPAGES");

      if(!envvar)
        {
          remoteMemorySize = gasnet_getMaxLocalSegmentSize();
        }
      else
        {
          long n_pages = 4096;
          sscanf(envvar,"%ld",&n_pages);
#ifdef DEBUG
          printf("n_pages %ld\n",n_pages);
#endif
          remoteMemorySize = n_pages*GASNET_PAGESIZE;
        }

      /* It creates the remote memory on each image */
      if (remote_memory==NULL)
	{
	  gasnet_handlerentry_t htable[] = {
	    { send_notify1_handler,  req_notify1_handler },
	  };

	  if (gasnet_attach (htable, sizeof (htable)/sizeof (gasnet_handlerentry_t),
			     remoteMemorySize, GASNET_PAGESIZE))
	    goto error;

	  r_pointer = 0;

	  remote_memory = malloc (sizeof (gasnet_seginfo_t) * caf_num_images);

	  if (remote_memory == NULL)
	    goto error;

	  /* gasnet_seginfo_t *tt = (gasnet_seginfo_t*)*token;  */

	  ierr = gasnet_getSegmentInfo (TOKEN (remote_memory), caf_num_images);

	  if (unlikely (ierr))
	    {
	      free (remote_memory);
	      goto error;
	    }

	}

    }

  return;

error:
  {
    char *msg;

    if (caf_is_finalized)
      msg = "Failed to create remote memory space - there are stopped images";
    else
      msg = "Failed during initialization and memory allocation";

    caf_runtime_error (msg);
  }

}


/* Finalize coarray program.   */

void
PREFIX (finalize) (void)
{
  gasnet_barrier_notify (0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait (0, GASNET_BARRIERFLAG_ANONYMOUS);

  while (caf_static_list != NULL)
    {
      caf_static_t *tmp = caf_static_list->prev;

      /* (void) ARMCI_Free (caf_static_list->token[caf_this_image-1]);  */
      /* free (caf_static_list->token);  */
      free (caf_static_list);
      caf_static_list = tmp;
    }

  /* (void) ARMCI_Finalize (); */
  /* armci_msg_finalize (); */
  gasnet_exit (0);

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
  int i;

  if (unlikely (caf_is_finalized))
    goto error;

  /* Start GASNET if not already started.  */
  if (caf_num_images == 0)
    PREFIX (init) (NULL, NULL);

  /* Here there was the if statement for remote allocation */
  /* Now it is included in init */

#ifdef DEBUG
  printf("image: %d memorysize: %ld Requested: %ld, status: %ld\n",caf_this_image,remoteMemorySize,size,r_pointer);
#endif

  /* Allocation check */
  if ((size+r_pointer) > remoteMemorySize)
    goto error;

  /* New variable registration.  */

  /* Token contains only a list of pointers.  */
  *token = malloc (caf_num_images * sizeof (void *));

  for (i = 0; i < caf_num_images; i++)
  {
    gasnet_seginfo_t *rm = TOKEN (remote_memory);
    char * tm = (char *) rm[i].addr;
    tm += r_pointer;
    void ** t = *token;
    t[i] = (void *) tm;
  }

  r_pointer += size;
  size_t align = r_pointer % MALLOC_ALIGNMENT;
  if (align)
    r_pointer += MALLOC_ALIGNMENT - align;

  if (type == CAF_REGTYPE_COARRAY_STATIC)
    {
      caf_static_t *tmp = malloc (sizeof (caf_static_t));
      tmp->prev  = caf_static_list;
      tmp->token = *token;
      caf_static_list = tmp;
    }

  if (stat)
    *stat = 0;

  void **tm = *token;
  return tm[caf_this_image-1];

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
PREFIX (deregister) (caf_token_t *token, int *stat, char *errmsg,
		     int errmsg_len)
{
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

  PREFIX(sync_all) (NULL, NULL, 0);

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
  int ierr;

  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
      gasnet_barrier_notify (0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait (0, GASNET_BARRIERFLAG_ANONYMOUS);
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
PREFIX (send) (caf_token_t token, size_t offset, int image_index,
	       gfc_descriptor_t *dest,
	       caf_vector_t *dst_vector __attribute__ ((unused)),
	       gfc_descriptor_t *src, int dst_kind, int src_kind)
{
  int ierr = 0, j=0;
  size_t i, size;
  void **tm = token;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  ptrdiff_t dst_offset = 0;
  void *pad_str = NULL;
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }


  if (unlikely (size == 0))
    return;  /* Zero-sized array.  */

  /* It works only if contiguous */
  /*if (image_index == caf_this_image)
    {
       void *dest = (void *) ((char *) tm[image_index-1] + offset);
       memmove (dest, src->base_addr, size*dst_size);
       return;
    }*/

  if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
    {
      pad_str = alloca (dst_size - src_size);
      if (dst_kind == 1)
	memset (pad_str, ' ', dst_size-src_size);
      else /* dst_kind == 4.  */
	for (i = 0; i < (dst_size-src_size)/4; i++)
	      ((int32_t*) pad_str)[i] = (int32_t) ' ';
    }

  if (rank == 0
      || (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind && GFC_DESCRIPTOR_RANK (src) != 0
          && (GFC_DESCRIPTOR_TYPE (dest) != BT_CHARACTER || dst_size == src_size)
	  && PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src)))
    {
      /* MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p); */
      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	gasnet_put_bulk (image_index-1, tm[image_index-1]+offset, src->base_addr, (dst_size > src_size ? src_size : dst_size)*size);
	/* ierr = MPI_Put (src->base_addr, dst_size*size, MPI_BYTE, */
	/* 		image_index-1, offset, */
	/* 		(dst_size > src_size ? src_size : dst_size) * size, */
	/* 		MPI_BYTE, *p); */
      if (pad_str)
	gasnet_put_bulk (image_index-1, tm[image_index-1]+offset, pad_str, dst_size-src_size);
	/* ierr = MPI_Put (pad_str, dst_size-src_size, MPI_BYTE, image_index-1, */
	/* 		offset, dst_size - src_size, MPI_BYTE, *p); */
      /* MPI_Win_unlock (image_index-1, *p); */
      if (ierr != 0)
	error_stop (ierr);
      return;
    }
  else
    {
#ifdef STRIDED
      gasnet_memvec_t * arr_dsp_s, *arr_dsp_d;

      void *sr = src->base_addr;

      arr_dsp_s = malloc (size * sizeof (gasnet_memvec_t));
      arr_dsp_d = malloc (size * sizeof (gasnet_memvec_t));

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

	  //arr_dsp_d[i] = calloc(1,sizeof(struct gasnet_memvec_t));

	  arr_dsp_d[i].addr = tm[image_index-1]+(offset+array_offset_dst)*GFC_DESCRIPTOR_SIZE (dest);
	  arr_dsp_d[i].len = GFC_DESCRIPTOR_SIZE (dest);

	  //arr_dsp_s[i] = calloc(1,sizeof(struct gasnet_memvec_t));

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

	      /* arr_dsp_s[i] = array_offset_sr; */

	      arr_dsp_s[i].addr = sr+array_offset_sr*GFC_DESCRIPTOR_SIZE (src);
	      arr_dsp_s[i].len = GFC_DESCRIPTOR_SIZE (src);
	    }
	  else
	    {
	      arr_dsp_s[i].addr = sr;
	      arr_dsp_s[i].len = GFC_DESCRIPTOR_SIZE (src);
	    }
	  //dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);
	  /* void *sr = (void *)((char *) src->base_addr */
	  /* 			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src)); */

	}

      /* gasnet_puts_bulk(image_index-1, tm[image_index-1]+offset, arr_dsp_d, */
      /* 		 sr, arr_dsp_s, arr_count, size); */
      gasnet_putv_bulk(image_index-1,size,arr_dsp_d,size,arr_dsp_s);

      free(arr_dsp_s);
      free(arr_dsp_d);
#else
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
	  dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);

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

	  gasnet_put_bulk (image_index-1, tm[image_index-1]+dst_offset, sr, dst_size);

	  /* ierr = MPI_Put (sr, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index-1, */
	  /* 		  dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p); */
	  if (pad_str)
	    gasnet_put_bulk (image_index-1, tm[image_index-1]+offset, pad_str, dst_size-src_size);
	    /* ierr = MPI_Put (pad_str, dst_size - src_size, MPI_BYTE, image_index-1, */
	    /* 		    dst_offset, dst_size - src_size, MPI_BYTE, *p); */
	  if (ierr != 0)
	    {
	      error_stop (ierr);
	      return;
	    }
	}
#endif
    }

  /* if (async == false) */
    /* gasnet_put_bulk (image_index-1, tm[image_index-1]+offset, data, size); */
  /* else */
  /*   ierr = ARMCI_NbPut(data,t.addr+offset,size,image_index-1,NULL); */
  if(ierr != 0)
    error_stop (ierr);
}


/* Send array data from src to dest on a remote image.  */

/* void */
/* PREFIX (send_desc) (caf_token_t token, size_t offset, int image_index, */
/* 		    gfc_descriptor_t *dest, gfc_descriptor_t *src, bool async) */
/* { */
/*   int ierr = 0; */
/*   size_t i, size; */
/*   int j; */
/*   int rank = GFC_DESCRIPTOR_RANK (dest); */
/*   void **tm = token; */

/*   size = 1; */
/*   for (j = 0; j < rank; j++) */
/*     { */
/*       ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1; */
/*       if (dimextent < 0) */
/* 	dimextent = 0; */
/*       size *= dimextent; */
/*     } */

/*   if (size == 0) */
/*     return; */

/*   if (PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src)) */
/*     { */
/*       void *dst = (void *)((char *) tm[image_index-1] + offset); */
/*       if (image_index == caf_this_image) */
/* 	  memmove (dst, src->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size); */
/*       else /\* if (!async) *\/ */
/*         gasnet_put_bulk (image_index-1, dst, src->base_addr, GFC_DESCRIPTOR_SIZE (dest)*size); */
/*       /\* else *\/ */

/*       if (ierr != 0) */
/* 	error_stop (ierr); */
/*       return; */
/*     } */

/*   for (i = 0; i < size; i++) */
/*     { */
/*       ptrdiff_t array_offset_dst = 0; */
/*       ptrdiff_t stride = 1; */
/*       ptrdiff_t extent = 1; */
/*       for (j = 0; j < rank-1; j++) */
/* 	{ */
/* 	  array_offset_dst += ((i / (extent*stride)) */
/* 			       % (dest->dim[j]._ubound */
/* 				  - dest->dim[j].lower_bound + 1)) */
/* 			      * dest->dim[j]._stride; */
/* 	  extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1); */
/*           stride = dest->dim[j]._stride; */
/* 	} */
/*       array_offset_dst += (i / extent) * dest->dim[rank-1]._stride; */

/*       ptrdiff_t array_offset_sr = 0; */
/*       stride = 1; */
/*       extent = 1; */
/*       for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++) */
/* 	{ */
/* 	  array_offset_sr += ((i / (extent*stride)) */
/* 			   % (src->dim[j]._ubound */
/* 			      - src->dim[j].lower_bound + 1)) */
/* 			  * src->dim[j]._stride; */
/* 	  extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1); */
/*           stride = src->dim[j]._stride; */
/* 	} */
/*       array_offset_sr += (i / extent) * src->dim[rank-1]._stride; */

/*       void *dst = (void *)((char *) tm[image_index-1] + offset */
/* 			   + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest)); */
/*       void *sr = (void *)((char *) src->base_addr */
/* 			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src)); */
/*       if (image_index == caf_this_image) */
/* 	memmove (dst, sr, GFC_DESCRIPTOR_SIZE (dest)); */
/*       else /\* if (!async) *\/ */
/*         gasnet_put_bulk (image_index-1, dst, sr, GFC_DESCRIPTOR_SIZE (dest)); */
/*       /\* else *\/ */

/*       if (ierr != 0) */
/* 	{ */
/* 	  error_stop (ierr); */
/* 	  return; */
/* 	} */
/*     } */
/* } */


/* Send scalar data from src to array dest on a remote image.  */

void
PREFIX (send_desc_scalar) (caf_token_t token, size_t offset, int image_index,
			   gfc_descriptor_t *dest, void *buffer, bool async)
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  void **tm = token;

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
      void *dst = (void *)((char *) tm[image_index-1] + offset
			   + array_offset*GFC_DESCRIPTOR_SIZE (dest));
      if (image_index == caf_this_image)
	memmove (dst, buffer, GFC_DESCRIPTOR_SIZE (dest));
      else /* if (!async) */
        gasnet_put_bulk (image_index-1, dst, buffer,
			 GFC_DESCRIPTOR_SIZE (dest));
      /* else */

      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}
    }
}


void
PREFIX (get) (caf_token_t token, size_t offset,
	      int image_index __attribute__ ((unused)),
	      gfc_descriptor_t *src ,
	      caf_vector_t *src_vector __attribute__ ((unused)),
	      gfc_descriptor_t *dest, int src_kind, int dst_kind)

{
  size_t i, size;
  int ierr = 0, j = 0;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);
  void *pad_str = NULL;

  void **tm = token;

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (unlikely (size == 0))
    return;  /* Zero-sized array.  */

  /* if (image_index == caf_this_image) */
  /*   { */
  /*      void *src = (void *) ((char *) tm[image_index-1] + offset); */
  /*      memmove (data, src, size); */
  /*      return; */
  /*   } */

  if (GFC_DESCRIPTOR_TYPE (dest) == BT_CHARACTER && dst_size > src_size)
    {
      pad_str = alloca (dst_size - src_size);
      if (dst_kind == 1)
	memset (pad_str, ' ', dst_size-src_size);
      else /* dst_kind == 4.  */
	for (i = 0; i < (dst_size-src_size)/4; i++)
	      ((int32_t*) pad_str)[i] = (int32_t) ' ';
    }

  if (rank == 0
      || (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind
          && (GFC_DESCRIPTOR_TYPE (dest) != BT_CHARACTER || dst_size == src_size)
	  && PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src)))
    {
      gasnet_get_bulk (dest->base_addr, image_index-1, tm[image_index-1]+offset, size*dst_size);

      if (pad_str)
	memcpy ((char *) dest->base_addr + src_size, pad_str,
		dst_size-src_size);
    }
  else
    {
#ifdef STRIDED
      gasnet_memvec_t * arr_dsp_s, *arr_dsp_d;

      void *dst = dest->base_addr;

      arr_dsp_s = malloc (size * sizeof (gasnet_memvec_t));
      arr_dsp_d = malloc (size * sizeof (gasnet_memvec_t));

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

	  arr_dsp_d[i].addr = dst+array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);
	  arr_dsp_d[i].len = dst_size;

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

	  arr_dsp_s[i].addr = tm[image_index-1]+(array_offset_sr+offset)*GFC_DESCRIPTOR_SIZE (src);
	  arr_dsp_s[i].len = src_size;

	}

      gasnet_getv_bulk(size,arr_dsp_d,image_index-1,size,arr_dsp_s);

      free(arr_dsp_s);
      free(arr_dsp_d);
#else
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

	  size_t sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src);
	  void *dst = (void *) ((char *) dest->base_addr
				+ array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
	  /* FIXME: Handle image_index == this_image().  */
	  /*  if (async == false) */
	  gasnet_get_bulk (dst, image_index-1, tm[image_index-1]+sr_off, GFC_DESCRIPTOR_SIZE (dest));
	  /* ierr = MPI_Get (dst, GFC_DESCRIPTOR_SIZE (dest), */
	  /* 		    MPI_BYTE, image_index-1, sr_off, */
	  /* 		    GFC_DESCRIPTOR_SIZE (src), MPI_BYTE, *p); */
	  if (pad_str)
	    memcpy ((char *) dst + src_size, pad_str, dst_size-src_size);

	  if (ierr != 0)
	    error_stop (ierr);
	}
#endif
    }

  /* if (async == false) */
    /* gasnet_get_bulk (data, image_index-1, tm[image_index-1]+offset, size); */
  /* else */
  /*   ierr = ARMCI_NbPut(data,t.addr+offset,size,image_index-1,NULL); */
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
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  void **tm = token;

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
      void *sr = (void *) ((char *) tm[image_index-1] + offset);
      if (image_index == caf_this_image)
	memmove (dest->base_addr, sr, GFC_DESCRIPTOR_SIZE (dest)*size);
      else
	gasnet_get_bulk (dest->base_addr, image_index-1, sr,
			 GFC_DESCRIPTOR_SIZE (dest)*size);
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

      void *sr = (void *)((char *) tm[image_index-1] + offset
			   + array_offset_sr*GFC_DESCRIPTOR_SIZE (src));
      void *dst = (void *)((char *) dest->base_addr
			  + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      if (image_index == caf_this_image)
	memmove (dst, sr, GFC_DESCRIPTOR_SIZE (dest));
      else
	gasnet_get_bulk (dst, image_index-1, sr, GFC_DESCRIPTOR_SIZE (dest));
    }
}


/* SYNC IMAGES. Note: SYNC IMAGES(*) is passed as count == -1 while
   SYNC IMAGES([]) has count == 0. Note further that SYNC IMAGES(*)
   is not equivalent to SYNC ALL. */
void
PREFIX (sync_images) (int count, int images[], int *stat, char *errmsg,
		      int errmsg_len)
{
  int ierr;
  if (count == 0 || (count == 1 && images[0] == caf_this_image))
    {
      if (stat)
	*stat = 0;
      return;
    }

#ifdef GFC_CAF_CHECK
  {
    int i;
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
	insOrders (images_full, caf_num_images-1);
      else
	insOrders (images, count);

      GASNET_BLOCKUNTIL(freeToGo() == true);

      ierr = 0;
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


/* ERROR STOP the other images.  */

static void
error_stop (int error)
{
  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  /* FIXME: Do some more effort than just gasnet_exit().  */
  gasnet_exit(error);

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
