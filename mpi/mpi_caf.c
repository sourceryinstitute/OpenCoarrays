/* One-sided MPI implementation of Libcaf

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

/****l* mpi/mpi_caf.c
 * NAME
 *   mpi_caf
 * SYNOPSIS
 *   This program implements the LIBCAF_MPI transport layer.
******
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* For memcpy.  */
#include <stdarg.h>	/* For variadic arguments.  */
#include <alloca.h>
#include <mpi.h>
#include <pthread.h>

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

/* Active messages variables */

char **buff_am;
MPI_Status *s_am;
MPI_Request *req_am;
MPI_Datatype *dts;
char *msgbody;
pthread_mutex_t lock_am;
int done_am=0;

char err_buffer[MPI_MAX_ERROR_STRING];

/* All CAF runtime calls should use this comm instead of
   MPI_COMM_WORLD for interoperability purposes. */
MPI_Comm CAF_COMM_WORLD;

/* For MPI interoperability, allow external initialization
   (and thus finalization) of MPI. */
bool caf_owns_mpi = false;

#ifdef HELPER
void helperFunction()
{
  int i = 0, flag = 0, msgid = 0;
  int ndim = 0, position = 0;

  s_am = calloc(caf_num_images, sizeof(MPI_Status));
  req_am = calloc(caf_num_images, sizeof(MPI_Request));
  dts = calloc(caf_num_images, sizeof(MPI_Datatype));

  for(i=0;i<caf_num_images;i++)
    MPI_Irecv(buff_am[i], 1000, MPI_PACKED, i, 1, CAF_COMM_WORLD, &req_am[i]);

  while(1)
    {
      pthread_mutex_lock(&lock_am);
      for(i=0;i<caf_num_images;i++)
        {
          if(!caf_is_finalized)
            {
              MPI_Test(&req_am[i], &flag, &s_am[i]);
              if(flag==1)
                {
                  position = 0;
                  MPI_Unpack(buff_am[i], 1000, &position, &msgid, 1, MPI_INT, CAF_COMM_WORLD);
		  /* msgid=2 was initially assigned to strided transfers, it can be reused */
		  /* Strided transfers Msgid=2 */

		  /* You can add you own function */

                  if(msgid==2)
                    {
                      msgid=0; position=0;
                    }
                  MPI_Irecv(buff_am[i], 1000, MPI_PACKED, i, 1, CAF_COMM_WORLD, &req_am[i]);
                  flag=0;
                }
            }
          else
            {
              done_am=1;
              pthread_mutex_unlock(&lock_am);
              return;
            }
        }
        pthread_mutex_unlock(&lock_am);
    }
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
  MPI_Finalize();

  /* Should be unreachable, but to make sure also call exit.  */
  exit (EXIT_FAILURE);
}

/* Lock implementation for MPI-2 */

void counter_inc(MPI_Win c_win, int *value, int inc, int image_index)
{
  int i, *val;

  val = (int *)calloc(caf_num_images, sizeof(int));

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, image_index-1, 0, c_win);

  for(i=0;i<caf_num_images;i++)
    if(i == caf_this_image-1)
      MPI_Accumulate(&inc, 1, MPI_INT, image_index-1, i*sizeof(int), 1, MPI_INT, MPI_SUM, c_win);
    else
      MPI_Get(&val[i], 1, MPI_INT, image_index-1, i*sizeof(int), 1, MPI_INT, c_win);

  MPI_Win_unlock(image_index-1, c_win);

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, image_index-1, 0, c_win);
  MPI_Get(&val[caf_this_image-1], 1, MPI_INT, image_index-1, (caf_this_image-1)*sizeof(int), 1, MPI_INT, c_win);
  MPI_Win_unlock(image_index-1, c_win);

  *value = -1;

  for(i=0;i<caf_num_images;i++)
    *value = *value + val[i];

  free(val);

}

void mutex_lock(MPI_Win win, int image_index)
{
  int value=0;
  counter_inc(win, &value, 1, image_index);
  while(value != 0)
    {
      counter_inc(win, &value, -1, image_index);
      counter_inc(win, &value, 1, image_index);
    }
}

void mutex_unlock(MPI_Win win, int image_index)
{
  int value;
  counter_inc(win, &value, -1, image_index);
}

/* Initialize coarray program.  This routine assumes that no other
   GASNet initialization happened before. */

void
PREFIX (init) (int *argc, char ***argv)
{
  if (caf_num_images == 0)
    {
      int ierr = 0, i = 0, j = 0;

      int is_init = 0, prior_thread_level = MPI_THREAD_SINGLE;
      MPI_Initialized(&is_init);

      if (is_init) {
          MPI_Query_thread(&prior_thread_level);
      }
#ifdef HELPER
      int prov_lev=0;
      if (is_init) {
          prov_lev = prior_thread_level;
          caf_owns_mpi = false;
      } else {
          MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &prov_lev);
          caf_owns_mpi = true;
      }

      if(caf_this_image == 0 && MPI_THREAD_MULTIPLE != prov_lev)
	caf_runtime_error ("MPI_THREAD_MULTIPLE is not supported: %d", prov_lev);
#else
      if (is_init) {
          caf_owns_mpi = false;
      } else {
          MPI_Init(argc, argv);
          caf_owns_mpi = true;
      }
#endif
      if (unlikely ((ierr != MPI_SUCCESS)))
	caf_runtime_error ("Failure when initializing MPI: %d", ierr);

      /* Duplicate MPI_COMM_WORLD so that no CAF internal functions
         use it - this is critical for MPI-interoperability. */
      MPI_Comm_dup(MPI_COMM_WORLD, &CAF_COMM_WORLD);

      MPI_Comm_size(CAF_COMM_WORLD, &caf_num_images);
      MPI_Comm_rank(CAF_COMM_WORLD, &caf_this_image);

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

  MPI_Barrier(CAF_COMM_WORLD);

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

  MPI_Comm_free(&CAF_COMM_WORLD);

  /* Only call Finalize if CAF runtime Initialized MPI. */
  if (caf_owns_mpi) {
      MPI_Finalize();
  }
  pthread_mutex_lock(&lock_am);
  caf_is_finalized = 1;
  pthread_mutex_unlock(&lock_am);
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
  size_t actual_size;
  int l_var=0, *init_array=NULL;

  if (unlikely (caf_is_finalized))
    goto error;

  /* Start GASNET if not already started.  */
  if (caf_num_images == 0)
    PREFIX (init) (NULL, NULL);

  /* Token contains only a list of pointers.  */

  *token = malloc (sizeof(MPI_Win));

  if(type == CAF_REGTYPE_LOCK_STATIC)
    {
      /* For a single lock variable we need an array of integers */
      actual_size = size*sizeof(int)*caf_num_images;
      l_var = 1;
    }
  else
    actual_size = size;

#if MPI_VERSION >= 3
  MPI_Win_allocate(actual_size, 1, mpi_info_same_size, CAF_COMM_WORLD, &mem, *token);
#else
  MPI_Alloc_mem(actual_size, MPI_INFO_NULL, &mem);
  MPI_Win_create(mem, actual_size, 1, MPI_INFO_NULL, CAF_COMM_WORLD, *token);
#endif

  MPI_Win *p = *token;

  MPI_Win_fence(0, *p);

  if(l_var)
    {
      init_array = (int *)calloc(caf_num_images, sizeof(int));
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, caf_this_image-1, 0, *p);
      MPI_Put (init_array, caf_num_images, MPI_INT, caf_this_image-1,
       	       0, caf_num_images, MPI_INT, *p);
      MPI_Win_unlock(caf_this_image-1, *p);
      free(init_array);
    }

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
#if 0
      /* MPI_Win_unlock implies synchronization, hence, the MPI_Win_fence is not
	 required.  */
      MPI_Win *p;
      caf_static_t *tmp, *next;
      for (tmp = caf_tot; tmp; )
	{
	  next = tmp->prev;
	  p = tmp->token;
	  MPI_Win_fence (0, *p);
          tmp = next;
	}
#endif
      MPI_Barrier(CAF_COMM_WORLD);
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

void selectType(int size, MPI_Datatype *dt)
{
  int t_s;

  MPI_Type_size(MPI_INT, &t_s);

  if(t_s==size)
    {
      *dt=MPI_INT;
      return;
    }

  MPI_Type_size(MPI_DOUBLE, &t_s);

  if(t_s==size)
    {
      *dt=MPI_DOUBLE;
      return;
    }
}

void
PREFIX (sendget) (caf_token_t token_s, size_t offset_s, int image_index_s,
		  gfc_descriptor_t *dest,
		  caf_vector_t *dst_vector __attribute__ ((unused)),
		  caf_token_t token_g, size_t offset_g,
		  int image_index_g, gfc_descriptor_t *src ,
		  caf_vector_t *src_vector __attribute__ ((unused)),
		  int src_kind, int dst_kind, bool mrt)
{
  int ierr = 0;
  size_t i, size;
  int j;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  MPI_Win *p_s = token_s, *p_g = token_g;
  ptrdiff_t dst_offset = 0;
  ptrdiff_t src_offset = 0;
  void *pad_str = NULL;
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);
  char *tmp;

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

  if (rank == 0
      || (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind && GFC_DESCRIPTOR_RANK (src) != 0
          && (GFC_DESCRIPTOR_TYPE (dest) != BT_CHARACTER || dst_size == src_size)
	  && PREFIX (is_contiguous) (dest) && PREFIX (is_contiguous) (src)))
    {
      tmp = (char *) calloc (size, dst_size);
      
      MPI_Win_lock (MPI_LOCK_SHARED, image_index_g-1, 0, *p_g);
      ierr = MPI_Get (tmp, dst_size*size, MPI_BYTE,
		      image_index_g-1, offset_g, dst_size*size, MPI_BYTE, *p_g);
      if (pad_str)
	memcpy ((char *) tmp + src_size, pad_str,
		dst_size-src_size);
      MPI_Win_unlock (image_index_g-1, *p_g);
      
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index_s-1, 0, *p_s);
      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	  && dst_kind == src_kind)
	ierr = MPI_Put (tmp, dst_size*size, MPI_BYTE,
			image_index_s-1, offset_s,
			(dst_size > src_size ? src_size : dst_size) * size,
			MPI_BYTE, *p_s);
      if (pad_str)
	ierr = MPI_Put (pad_str, dst_size-src_size, MPI_BYTE, image_index_s-1,
			offset_s, dst_size - src_size, MPI_BYTE, *p_s);
      MPI_Win_unlock (image_index_s-1, *p_s);
      
      if (ierr != 0)
	error_stop (ierr);
      return;
      
      free(tmp);
    }
  else
    {
      tmp = calloc(1, dst_size);

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
	  dst_offset = offset_s + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);

	  ptrdiff_t array_offset_sr = 0;
	  if (GFC_DESCRIPTOR_RANK (src) != 0)
	    {
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
	      array_offset_sr *= GFC_DESCRIPTOR_SIZE (src);
	    }
	  src_offset = offset_g + array_offset_sr;

	  MPI_Win_lock (MPI_LOCK_SHARED, image_index_g-1, 0, *p_g);

	  ierr = MPI_Get (tmp, dst_size, MPI_BYTE,
			  image_index_g-1, src_offset, src_size, MPI_BYTE, *p_g);

	  MPI_Win_unlock (image_index_g-1, *p_g);

	  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index_s-1, 0, *p_s);

	  ierr = MPI_Put (tmp, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index_s-1,
			  dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p_s);
	  if (pad_str)
	    ierr = MPI_Put (pad_str, dst_size - src_size, MPI_BYTE, image_index_s-1,
			    dst_offset, dst_size - src_size, MPI_BYTE, *p_s);

	  MPI_Win_unlock (image_index_s-1, *p_s);

	  if (ierr != 0)
	    {
	      error_stop (ierr);
	      return;
	    }
	}
      free(tmp);
    }

}

/* Send array data from src to dest on a remote image.  */
/* The last argument means may_require_temporary */

void
PREFIX (send) (caf_token_t token, size_t offset, int image_index,
	       gfc_descriptor_t *dest,
	       caf_vector_t *dst_vector __attribute__ ((unused)),
	       gfc_descriptor_t *src, int dst_kind, int src_kind, 
	       bool mrt)
{
  /* FIXME: Implement vector subscripts, type conversion and check whether
     string-kind conversions are permitted.
     FIXME: Implement sendget as well.  */
  int ierr = 0;
  size_t i, size;
  int j;
  /* int position, msg = 0;  */
  int rank = GFC_DESCRIPTOR_RANK (dest);
  MPI_Win *p = token;
  ptrdiff_t dst_offset = 0;
  void *pad_str = NULL;
  void *t_buff = NULL;
  bool *buff_map = NULL;
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

  if (size == 0)
    return;

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
      if(caf_this_image == image_index)
	{
	  void *dest_tmp = (void *) ((char *) dest->base_addr + offset);
	  memmove (dest_tmp,src->base_addr,size*dst_size);
	  return;
	}
      else
	{
	  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
	  if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
	      && dst_kind == src_kind)
	    ierr = MPI_Put (src->base_addr, dst_size*size, MPI_BYTE,
			    image_index-1, offset,
			    (dst_size > src_size ? src_size : dst_size) * size,
			    MPI_BYTE, *p);
	  if (pad_str)
	    ierr = MPI_Put (pad_str, dst_size-src_size, MPI_BYTE, image_index-1,
			    offset, dst_size - src_size, MPI_BYTE, *p);
	  MPI_Win_unlock (image_index-1, *p);
	} 

      if (ierr != 0)
	error_stop (ierr);
      return;
    }
  else
    {
#ifdef STRIDED
      MPI_Datatype dt_s, dt_d, base_type_src, base_type_dst;
      int *arr_bl;
      int *arr_dsp_s, *arr_dsp_d;

      void *sr = src->base_addr;

      arr_bl = calloc (size, sizeof (int));
      arr_dsp_s = calloc (size, sizeof (int));
      arr_dsp_d = calloc (size, sizeof (int));

      selectType (GFC_DESCRIPTOR_SIZE (src), &base_type_src);
      selectType (GFC_DESCRIPTOR_SIZE (dest), &base_type_dst);

      if(rank == 1)
        {
          MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src, &dt_s);
          MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst, &dt_d);
        }
      else
	{
	  for (i = 0; i < size; i++)
	    arr_bl[i] = 1;

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

	      arr_dsp_d[i] = array_offset_dst;

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
		  arr_dsp_s[i] = array_offset_sr;
		}
	      else
		arr_dsp_s[i] = 0;

	      //dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);
	      /* void *sr = (void *)((char *) src->base_addr */
	      /* 			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src)); */
	    }

	  MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
	  MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
	}

      MPI_Type_commit(&dt_s);
      MPI_Type_commit(&dt_d);

      dst_offset = offset;

      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
      ierr = MPI_Put (sr, 1, dt_s, image_index-1, dst_offset, 1, dt_d, *p);
      MPI_Win_unlock (image_index-1, *p);

      if (ierr != 0)
	{
	  error_stop (ierr);
	  return;
	}

      MPI_Type_free (&dt_s);
      MPI_Type_free (&dt_d);

      free (arr_bl);
      free (arr_dsp_s);
      free (arr_dsp_d);

      /* msg = 2; */
      /* MPI_Pack(&msg, 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */
      /* MPI_Pack(&rank, 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */

      /* for(j=0;j<rank;j++) */
      /*   { */
      /*     MPI_Pack(&(dest->dim[j]._stride), 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */
      /*     MPI_Pack(&(dest->dim[j].lower_bound), 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */
      /*     MPI_Pack(&(dest->dim[j]._ubound), 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */
      /*   } */

      /* MPI_Pack(&size, 1, MPI_INT, buff_am[caf_this_image], 1000, &position, CAF_COMM_WORLD); */

      /* /\* non-blocking send *\/ */

      /* MPI_Issend(buff_am[caf_this_image], position, MPI_PACKED, image_index-1, 1, CAF_COMM_WORLD, &reqdt); */

      /* msgbody = calloc(size, sizeof(char)); */

      /* ptrdiff_t array_offset_sr = 0; */
      /* ptrdiff_t stride = 1; */
      /* ptrdiff_t extent = 1; */

      /* for(i = 0; i < size; i++) */
      /*   { */
      /*     for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++) */
      /* 	{ */
      /* 	  array_offset_sr += ((i / (extent*stride)) */
      /* 			      % (src->dim[j]._ubound */
      /* 				 - src->dim[j].lower_bound + 1)) */
      /* 	    * src->dim[j]._stride; */
      /* 	  extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1); */
      /*         stride = src->dim[j]._stride; */
      /* 	} */

      /*     array_offset_sr += (i / extent) * src->dim[rank-1]._stride; */

      /*     void *sr = (void *)((char *) src->base_addr */
      /* 			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (src)); */

      /*     memmove (msgbody+p_mb, sr, GFC_DESCRIPTOR_SIZE (src)); */

      /*     p_mb += GFC_DESCRIPTOR_SIZE (src); */
      /*   } */

      /* MPI_Wait(&reqdt, &stadt); */

      /* MPI_Ssend(msgbody, size, MPI_BYTE, image_index-1, 1, CAF_COMM_WORLD); */

      /* free(msgbody); */

#else
      if(caf_this_image == image_index && mrt)
	{
	  t_buff = calloc(size,GFC_DESCRIPTOR_SIZE (dest));
	  buff_map = calloc(size,sizeof(bool));
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

	  if(caf_this_image == image_index)
	    {
	      if(!mrt)
		memmove(dest->base_addr+dst_offset,sr,GFC_DESCRIPTOR_SIZE (src));
	      else
		{
		  memmove(t_buff+i*GFC_DESCRIPTOR_SIZE (src),sr,GFC_DESCRIPTOR_SIZE (src));
		  buff_map[i] = true;
		}
	    }
	  else
	    {
	      ierr = MPI_Put (sr, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index-1,
			      dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p);
	      if (pad_str)
		ierr = MPI_Put (pad_str, dst_size - src_size, MPI_BYTE, image_index-1,
				dst_offset, dst_size - src_size, MPI_BYTE, *p);
	    }

	  if (ierr != 0)
	    {
	      error_stop (ierr);
	      return;
	    }
	}

      if(caf_this_image == image_index && mrt)
	{
	  for(i=0;i<size;i++)
	    {
	      if(buff_map[i])
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
		  memmove(src->base_addr+dst_offset,t_buff+i*GFC_DESCRIPTOR_SIZE (src),GFC_DESCRIPTOR_SIZE (src));
		}
	    }
	  free(t_buff);
	  free(buff_map);
	}
      MPI_Win_unlock (image_index-1, *p);
#endif
    }
}


/* Get array data from a remote src to a local dest.  */

void
PREFIX (get) (caf_token_t token, size_t offset,
	      int image_index __attribute__ ((unused)),
	      gfc_descriptor_t *src ,
	      caf_vector_t *src_vector __attribute__ ((unused)),
	      gfc_descriptor_t *dest, int src_kind, int dst_kind,
	      bool mrt)
{
  size_t i, size;
  int ierr = 0;
  int j;
  MPI_Win *p = token;
  int rank = GFC_DESCRIPTOR_RANK (dest);
  size_t src_size = GFC_DESCRIPTOR_SIZE (src);
  size_t dst_size = GFC_DESCRIPTOR_SIZE (dest);
  void *t_buff = NULL;
  bool *buff_map = NULL;
  void *pad_str = NULL;
  /* size_t sr_off = 0;  */

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
      /* FIXME: Handle image_index == this_image().  */
      /*  if (async == false) */
      if(caf_this_image == image_index)
	{
	  void *src_tmp = (void *) ((char *) src->base_addr + offset);
	  memmove(dest->base_addr,src_tmp,size*src_size);
	  return;
	}
      else
	{
	  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
	  ierr = MPI_Get (dest->base_addr, dst_size*size, MPI_BYTE,
			  image_index-1, offset, dst_size*size, MPI_BYTE, *p);
	  if (pad_str)
	    memcpy ((char *) dest->base_addr + src_size, pad_str,
		    dst_size-src_size);
	  MPI_Win_unlock (image_index-1, *p);
	}
      if (ierr != 0)
	error_stop (ierr);
      return;
    }

#ifdef STRIDED

  MPI_Datatype dt_s, dt_d, base_type_src, base_type_dst;
  int *arr_bl;
  int *arr_dsp_s, *arr_dsp_d;

  void *dst = dest->base_addr;

  arr_bl = calloc(size, sizeof(int));
  arr_dsp_s = calloc(size, sizeof(int));
  arr_dsp_d = calloc(size, sizeof(int));

  selectType(GFC_DESCRIPTOR_SIZE (src), &base_type_src);
  selectType(GFC_DESCRIPTOR_SIZE (dest), &base_type_dst);

  if(rank == 1)
    {
      MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src, &dt_s);
      MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst, &dt_d);
    }
  else
    {
      for(i=0;i<size;i++)
	arr_bl[i]=1;

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
	
	  arr_dsp_d[i] = array_offset_dst;

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

	  arr_dsp_s[i] = array_offset_sr;

	  /* sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src); */
	  /* void *dst = (void *) ((char *) dest->base_addr */
	  /* 			    + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest)); */
	  /* FIXME: Handle image_index == this_image().  */
	  /*  if (async == false) */
	  /* { */
	  /*   ierr = MPI_Get (dst, GFC_DESCRIPTOR_SIZE (dest), */
	  /* 		  MPI_BYTE, image_index-1, sr_off, */
	  /* 		  GFC_DESCRIPTOR_SIZE (src), MPI_BYTE, *p); */
	  /* } */
	}
      MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
      MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
    }

  MPI_Type_commit(&dt_s);
  MPI_Type_commit(&dt_d);

  //sr_off = offset;

  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);

  ierr = MPI_Get (dst, 1, dt_d, image_index-1, offset, 1, dt_s, *p);

  MPI_Win_unlock (image_index-1, *p);

  if (ierr != 0)
    error_stop (ierr);

  MPI_Type_free(&dt_s);
  MPI_Type_free(&dt_d);

  free(arr_bl);
  free(arr_dsp_s);
  free(arr_dsp_d);

#else
  if(caf_this_image == image_index && mrt)
    {
      t_buff = calloc(size,GFC_DESCRIPTOR_SIZE (dest));
      buff_map = calloc(size,sizeof(bool));
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

      array_offset_sr += (i / extent) * src->dim[rank-1]._stride;

      size_t sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src);
      void *dst = (void *) ((char *) dest->base_addr
			    + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      /* FIXME: Handle image_index == this_image().  */
      /*  if (async == false) */
      if(caf_this_image == image_index)
	{
	  /* Is this needed? */
	  if(!mrt)
	    memmove(dst,src->base_addr+array_offset_sr,GFC_DESCRIPTOR_SIZE (src));
	  else
	    {
	      memmove(t_buff+i*GFC_DESCRIPTOR_SIZE (dest),dst,GFC_DESCRIPTOR_SIZE (dest));
	      buff_map[i] = true;
	    }
	}
      else
	{
	  ierr = MPI_Get (dst, GFC_DESCRIPTOR_SIZE (dest),
			  MPI_BYTE, image_index-1, sr_off,
			  GFC_DESCRIPTOR_SIZE (src), MPI_BYTE, *p);
	  if (pad_str)
	    memcpy ((char *) dst + src_size, pad_str, dst_size-src_size);
	}
      if (ierr != 0)
	error_stop (ierr);
    }

  if(caf_this_image == image_index && mrt)
    {
      for(i=0;i<size;i++)
	{
	  if(buff_map[i])
	    {
	      ptrdiff_t array_offset_sr = 0;
	      ptrdiff_t stride = 1;
	      ptrdiff_t extent = 1;
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
	      
	      memmove(dest->base_addr+sr_off,t_buff+i*GFC_DESCRIPTOR_SIZE (src),GFC_DESCRIPTOR_SIZE (src));
	    }
	}
      free(t_buff);
      free(buff_map);
    }
  MPI_Win_unlock (image_index-1, *p);
#endif
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
	   ierr = MPI_Irecv(&arrived[images[i]-1], 1, MPI_INT, images[i]-1, 0, CAF_COMM_WORLD, &handlers[images[i]-1]);

       for(i=0; i < count; i++)
	 ierr = MPI_Send(&caf_this_image, 1, MPI_INT, images[i]-1, 0, CAF_COMM_WORLD);

       for(i=0; i < count; i++)
	 ierr = MPI_Wait(&handlers[images[i]-1], &s);

       memset(arrived, 0, sizeof(int)*caf_num_images);

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
static void \
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
#if defined(MPI_INTEGER16) && defined(GFC_DTYPE_INTEGER_16)
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
co_reduce_1 (MPI_Op op, gfc_descriptor_t *source, int result_image, int *stat,
	     char *errmsg, int src_len __attribute__ ((unused)), int errmsg_len)
{
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

  if (rank == 0)
    {
      if (result_image == 0)
	ierr = MPI_Allreduce (MPI_IN_PLACE, source->base_addr, size, datatype,
			      op, CAF_COMM_WORLD);
      else if (result_image == caf_this_image)
	ierr = MPI_Reduce (MPI_IN_PLACE, source->base_addr, size, datatype, op,
			   result_image-1, CAF_COMM_WORLD);
      else
	ierr = MPI_Reduce (source->base_addr, NULL, size, datatype, op,
			   result_image-1, CAF_COMM_WORLD);
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
      if (result_image == 0)
	ierr = MPI_Allreduce (MPI_IN_PLACE, sr, 1, datatype, op,
			      CAF_COMM_WORLD);
      else if (result_image == caf_this_image)
	ierr = MPI_Reduce (MPI_IN_PLACE, sr, 1, datatype, op,
			   result_image-1, CAF_COMM_WORLD);
      else
	ierr = MPI_Reduce (sr, NULL, 1, datatype, op, result_image-1,
			   CAF_COMM_WORLD);
      if (ierr)
	goto error;
    }

  if (stat)
    *stat = 0;
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
PREFIX (co_broadcast) (gfc_descriptor_t *a, int source_image, int *stat, char *errmsg,
		       int errmsg_len)
{
  size_t i, size;
  int j, ierr;
  int rank = GFC_DESCRIPTOR_RANK (a);
  
  MPI_Datatype datatype = get_MPI_datatype (a);

  size = 1;
  for (j = 0; j < rank; j++)
    {
      ptrdiff_t dimextent = a->dim[j]._ubound
			    - a->dim[j].lower_bound + 1;
      if (dimextent < 0)
	dimextent = 0;
      size *= dimextent;
    }

  if (rank == 0)
    {
      ierr = MPI_Bcast(a->base_addr, size, datatype, source_image-1, CAF_COMM_WORLD);

      if (ierr)
	goto error;
      return;
    }

  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset_sr = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      for (j = 0; j < GFC_DESCRIPTOR_RANK (a)-1; j++)
	{
	  array_offset_sr += ((i / (extent*stride))
			   % (a->dim[j]._ubound
			      - a->dim[j].lower_bound + 1))
			  * a->dim[j]._stride;
	  extent = (a->dim[j]._ubound - a->dim[j].lower_bound + 1);
          stride = a->dim[j]._stride;
	}
      array_offset_sr += (i / extent) * a->dim[rank-1]._stride;
      void *sr = (void *)((char *) a->base_addr
			  + array_offset_sr*GFC_DESCRIPTOR_SIZE (a));

      ierr = MPI_Bcast(sr, 1, datatype, source_image-1, CAF_COMM_WORLD);
      
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
PREFIX (co_sum) (gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
		 int errmsg_len)
{
  co_reduce_1 (MPI_SUM, a, result_image, stat, errmsg, 0, errmsg_len);
}


void
PREFIX (co_min) (gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
		 int src_len, int errmsg_len)
{
  co_reduce_1 (MPI_MIN, a, result_image, stat, errmsg, src_len, errmsg_len);
}


void
PREFIX (co_max) (gfc_descriptor_t *a, int result_image, int *stat,
		 char *errmsg, int src_len, int errmsg_len)
{
  co_reduce_1 (MPI_MAX, a, result_image, stat, errmsg, src_len, errmsg_len);
}


/* Locking functions */

void
PREFIX (lock) (caf_token_t token, size_t index, int image_index,
	       int *aquired_lock, int *stat, char *errmsg,
	       int errmsg_len)
{
  MPI_Win *p = token;
  /* int ierr = 0, value = 0;  */

  mutex_lock(*p, image_index);
}


void
PREFIX (unlock) (caf_token_t token, size_t index, int image_index,
		 int *stat, char *errmsg, int errmsg_len)
{
  MPI_Win *p = token;
  /* int value = 0, ierr = 0;  */

  mutex_unlock(*p, image_index);
}

/* Atomics operations */

void
PREFIX (atomic_define) (caf_token_t token, size_t offset,
			int image_index, void *value, int *stat,
			int type __attribute__ ((unused)), int kind)
{
  MPI_Win *p = token;
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
  void *bef_acc;
  bef_acc = malloc(kind);
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Fetch_and_op(value, bef_acc, dt, image, offset,
			  MPI_REPLACE, *p);
  MPI_Win_unlock (image, *p);
  free(bef_acc);
#else
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Put (value, 1, dt, image, offset, 1, dt, *p);
  MPI_Win_unlock (image, *p);
#endif

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    error_stop (ierr);

  return;
}

void
PREFIX(atomic_ref) (caf_token_t token, size_t offset,
		    int image_index,
		    void *value, int *stat,
		    int type __attribute__ ((unused)), int kind)
{
  MPI_Win *p = token;
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Fetch_and_op(NULL, value, dt, image, offset, MPI_NO_OP, *p);
  MPI_Win_unlock (image, *p);
#else
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Get (value, 1, dt, image, offset, 1, dt, *p);
  MPI_Win_unlock (image, *p);
#endif

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    error_stop (ierr);

  return;
}


void
PREFIX(atomic_cas) (caf_token_t token, size_t offset,
		    int image_index, void *old, void *compare,
		    void *new_val, int *stat,
		    int type __attribute__ ((unused)), int kind)
{
  MPI_Win *p = token;
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType (kind, &dt);

#if MPI_VERSION >= 3
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Compare_and_swap (new_val, compare, old, dt, image,
			       offset, *p);
  MPI_Win_unlock (image, *p);
#else
#warning atomic_cas for MPI-2 is not yet implemented
  printf ("We apologize but atomic_cas for MPI-2 is not yet implemented\n");
  ierr = 1;
  /* value = malloc(kind); */
/*   MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p); */
/*   ierr = MPI_Get (value, 1, dt, image_index-1, offset, 1, dt, *p); */
/*   // We need something to guarantee that MPI_Get terminates without releasing the lock */
/*   // on the window. */
/*   if(memcmp(compare, value, kind)==0) */
/*     { */
/*       ierr = MPI_Put (new_val, 1, dt, image_index-1, offset, 1, dt, *p); */
/*       memcpy(old, value, kind); */
/*     } */

/*   if (stat) */
/*     *stat = ierr; */

/*   if (ierr != 0) */
/*     error_stop (ierr); */
/*   return; */

/*   MPI_Win_unlock (image_index-1, *p); */
/*   free(value); */
#endif

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    error_stop (ierr);

  return;
}

void
PREFIX (atomic_op) (int op, caf_token_t token ,
		    size_t offset, int image_index,
		    void *value, void *old, int *stat, 
		    int type __attribute__ ((unused)),
		    int kind)
{
  int ierr = 0;
  void *bef_acc;
  MPI_Datatype dt;
  MPI_Win *p = token;
  int image;

#if MPI_VERSION >= 3
  old = malloc(kind);

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType (kind, &dt);

  /* Atomic_add */
  if(op == 1)
    {
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_SUM, *p);
      MPI_Win_unlock (image, *p);
    }
  /* Atomic_and */
  else if(op == 2)
    {
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BAND, *p);
      MPI_Win_unlock (image, *p);
    }
  /* Atomic_or */
  else if(op == 4)
    {
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BOR, *p);
      MPI_Win_unlock (image, *p);
    }
  else if(op == 5)
    {
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BXOR, *p);
      MPI_Win_unlock (image, *p);
    }
  else
    printf ("We apologize but the atomic operation requested for MPI is not yet implemented\n");

  free(old);
#else
 #warning atomic_op for MPI is not yet implemented
   printf ("We apologize but atomic_op for MPI is not yet implemented\n");
#endif
  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    error_stop (ierr);

  return;
}


/* ERROR STOP the other images.  */

static void
error_stop (int error)
{
  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.  */
  /* FIXME: Do some more effort than just gasnet_exit().  */
  MPI_Abort(CAF_COMM_WORLD, error);

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
