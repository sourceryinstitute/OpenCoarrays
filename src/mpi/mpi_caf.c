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

/****l* mpi/mpi_caf.c
 * NAME
 *   mpi_caf
 * SYNOPSIS
 *   This program implements the LIBCAF_MPI transport layer.
******
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* For memcpy.  */
#include <stdarg.h>        /* For variadic arguments.  */
#include <alloca.h>
#include <unistd.h>
#include <mpi.h>
#include <pthread.h>

#include "libcaf.h"

/* Define GFC_CAF_CHECK to enable run-time checking.  */
/* #define GFC_CAF_CHECK  1  */


#ifdef GCC_GE_7
typedef struct mpi_caf_token_t
{
  void *local_memptr;
  MPI_Win memptr;
  MPI_Win *desc;
} mpi_caf_token_t;
#define TOKEN(X) &(((mpi_caf_token_t *) (X))->memptr)
#else
typedef MPI_Win *mpi_caf_token_t;
#define TOKEN(X) ((mpi_caf_token_t) (X))
#endif

static void error_stop (int error) __attribute__ ((noreturn));

/* Global variables.  */
static int caf_this_image;
static int caf_num_images = 0;
static int caf_is_finalized = 0;

#if MPI_VERSION >= 3
  MPI_Info mpi_info_same_size;
#endif // MPI_VERSION

/*Sync image part*/

static int *orders;
MPI_Request *handlers;
static int *images_full;
static int *arrived;

/* Pending puts */
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
typedef struct win_sync {
  MPI_Win *win;
  int img;
  struct win_sync *next;
} win_sync;

static win_sync *last_elem = NULL;
static win_sync *pending_puts = NULL;
#endif

caf_static_t *caf_static_list = NULL;
caf_static_t *caf_tot = NULL;

/* Image status variable */

static int *img_status = NULL;
MPI_Win *stat_tok;

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

/* Foo function pointers for coreduce */
int (*foo_int32_t)(void *, void *);
float (*foo_float)(void *, void *);
double (*foo_double)(void *, void *);

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
void explicit_flush()
{
  win_sync *w=pending_puts, *t;
  MPI_Win *p;
  while(w != NULL)
    {
      p = w->win;
      MPI_Win_flush(w->img,*p);
      t = w;
      w = w->next;
      free(t);
    }
  last_elem = NULL;
  pending_puts = NULL;
}
#endif

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
  //  MPI_Finalize();

  /* Should be unreachable, but to make sure also call exit.  */
  exit (EXIT_FAILURE);
}

/* FIXME: CMake chokes on the "inline" keyword below.  If we can detect that CMake is  */
/*        being used, we could add something of the form "#ifdef _CMAKE" to remove the */
/*        keyword only when building with CMake */
/* inline */ void locking_atomic_op(MPI_Win win, int *value, int newval,
			      int compare, int image_index, int index)
{
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, win);
# endif // CAF_MPI_LOCK_UNLOCK
      MPI_Compare_and_swap (&newval,&compare,value, MPI_INT,image_index-1,
                            index*sizeof(int), win);
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image_index-1, win);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image_index-1, win);
# endif // CAF_MPI_LOCK_UNLOCK
}

void mutex_lock(MPI_Win win, int image_index, int index, int *stat,
		int *acquired_lock, char *errmsg, int errmsg_len)
{
  const char msg[] = "Already locked";
#if MPI_VERSION >= 3
  int value=1, compare = 0, newval = caf_this_image, i = 1;

  if(stat != NULL)
    *stat = 0;

  locking_atomic_op(win, &value, newval, compare, image_index, index);

  if(value == caf_this_image && image_index == caf_this_image)
    goto stat_error;

  if(acquired_lock != NULL)
    {
      if(value == 0)
        *acquired_lock = 1;
      else
	*acquired_lock = 0;
      return;
    }

  while(value != 0)
    {
      locking_atomic_op(win, &value, newval, compare, image_index, index);
      usleep(caf_this_image*i);
      i++;
    }

  return;

stat_error:
  if(errmsg != NULL)
    {
      memset(errmsg,' ',errmsg_len);
      memcpy(errmsg, msg, MIN(errmsg_len,strlen(msg)));
    }
  if(stat != NULL)
    *stat = 99;
  else
    error_stop(99);
#else // MPI_VERSION
#warning Locking for MPI-2 is not implemented
  printf ("Locking for MPI-2 is not supported, please update your MPI implementation\n");
#endif // MPI_VERSION
}

void mutex_unlock(MPI_Win win, int image_index, int index, int *stat,
		  char* errmsg, int errmsg_len)
{
  const char msg[] = "Variable is not locked";
  if(stat != NULL)
    *stat = 0;
#if MPI_VERSION >= 3
  int value=1, compare = 1, newval = 0;

  /* locking_atomic_op(win, &value, newval, compare, image_index, index); */

# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, win);
# endif // CAF_MPI_LOCK_UNLOCK
  MPI_Fetch_and_op(&newval, &value, MPI_INT, image_index-1, index*sizeof(int), MPI_REPLACE, win);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image_index-1, win);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image_index-1, win);
# endif // CAF_MPI_LOCK_UNLOCK

  if(value == 0)
    goto stat_error;

  return;

stat_error:
  if(errmsg != NULL)
    {
      memset(errmsg,' ',errmsg_len);
      memcpy(errmsg, msg, MIN(errmsg_len,strlen(msg)));
    }
  if(stat != NULL)
    *stat = 99;
  else
    error_stop(99);
#else // MPI_VERSION
#warning Locking for MPI-2 is not implemented
  printf ("Locking for MPI-2 is not supported, please update your MPI implementation\n");
#endif // MPI_VERSION
}

/* Initialize coarray program.  This routine assumes that no other
   GASNet initialization happened before. */

void
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
_gfortran_caf_init (int *argc, char ***argv)
#else
PREFIX (init) (int *argc, char ***argv)
#endif
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

      for (i = 1; i <= caf_num_images; ++i)
        if (i != caf_this_image)
          {
            images_full[j] = i;
            j++;
          }

      orders = calloc (caf_num_images, sizeof (int));
      arrived = calloc (caf_num_images, sizeof (int));

      handlers = malloc(caf_num_images * sizeof(MPI_Request));

      stat_tok = malloc (sizeof(MPI_Win));

#if MPI_VERSION >= 3
      MPI_Info_create (&mpi_info_same_size);
      MPI_Info_set (mpi_info_same_size, "same_size", "true");
      /* Setting img_status */
      MPI_Win_allocate(sizeof(int), 1, mpi_info_same_size, CAF_COMM_WORLD, &img_status, stat_tok);
# ifndef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock_all(MPI_MODE_NOCHECK, *stat_tok);
# endif // CAF_MPI_LOCK_UNLOCK
#else
      MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &img_status, stat_tok);
      MPI_Win_create(img_status, sizeof(int), 1, MPI_INFO_NULL, CAF_COMM_WORLD, stat_tok);
#endif // MPI_VERSION
      *img_status = 0;
    }
  /* MPI_Barrier(CAF_COMM_WORLD); */
}


/* Finalize coarray program.   */

void
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
_gfortran_caf_finalize (void)
#else
PREFIX (finalize) (void)
#endif
{
  *img_status = STAT_STOPPED_IMAGE; /* GFC_STAT_STOPPED_IMAGE = 6000 */
  MPI_Win_sync(*stat_tok);

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
      p = TOKEN(tmp_tot->token);
#ifdef GCC_GE_7
# ifndef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock_all(*p);
# endif // CAF_MPI_LOCK_UNLOCK
      if (((mpi_caf_token_t *)tmp_tot->token)->desc)
	{
	  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *)tmp_tot->token;
# ifndef CAF_MPI_LOCK_UNLOCK
	  MPI_Win_unlock_all(*(mpi_token->desc));
# endif // CAF_MPI_LOCK_UNLOCK
	  MPI_Win_free (mpi_token->desc);
	  free (mpi_token->desc);
	}
#else
# ifndef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock_all(*p);
# endif // CAF_MPI_LOCK_UNLOCK
      MPI_Win_free(p);
#endif
      free(tmp_tot);
      tmp_tot = prev;
    }
#if MPI_VERSION >= 3
  MPI_Info_free (&mpi_info_same_size);
#endif // MPI_VERSION

  MPI_Comm_free(&CAF_COMM_WORLD);

  /* Only call Finalize if CAF runtime Initialized MPI. */
  if (caf_owns_mpi) {
      MPI_Finalize();
  }
  pthread_mutex_lock(&lock_am);
  caf_is_finalized = 1;
  pthread_mutex_unlock(&lock_am);
  exit(0);
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


#ifdef GCC_GE_7
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
void
  _gfortran_caf_register (size_t size, caf_register_t type, caf_token_t *token,
                gfc_descriptor_t *desc, int *stat, char *errmsg, int errmsg_len)
#else
void
  PREFIX (register) (size_t size, caf_register_t type, caf_token_t *token,
                gfc_descriptor_t *desc, int *stat, char *errmsg, int errmsg_len)
#endif
#else
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
void *
  _gfortran_caf_register (size_t size, caf_register_t type, caf_token_t *token,
                int *stat, char *errmsg, int errmsg_len)
#else
void *
  PREFIX (register) (size_t size, caf_register_t type, caf_token_t *token,
                int *stat, char *errmsg, int errmsg_len)
#endif
#endif
{
  /* int ierr; */
  void *mem;
  size_t actual_size;
  int l_var=0, *init_array=NULL;

  if (unlikely (caf_is_finalized))
    goto error;

  /* Start GASNET if not already started.  */
  if (caf_num_images == 0)
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
    _gfortran_caf_init (NULL, NULL);
#else
    PREFIX (init) (NULL, NULL);
#endif

  if(type == CAF_REGTYPE_LOCK_STATIC || type == CAF_REGTYPE_LOCK_ALLOC ||
     type == CAF_REGTYPE_CRITICAL || type == CAF_REGTYPE_EVENT_STATIC ||
     type == CAF_REGTYPE_EVENT_ALLOC)
    {
      actual_size = size*sizeof(int);
      l_var = 1;
    }
  else
    actual_size = size;

#ifdef GCC_GE_7
  *token = malloc (sizeof (mpi_caf_token_t));
  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *) *token;
  MPI_Win *p = TOKEN(mpi_token);
  if (GFC_DESCRIPTOR_RANK (desc) == 0)
    mpi_token->desc = NULL;
  else
    {
      int ierr;
      size_t desc_size = sizeof (gfc_descriptor_t) + /*GFC_DESCRIPTOR_RANK (desc)*/
	  GFC_MAX_DIMENSIONS * sizeof (descriptor_dimension);
      mpi_token->desc = (MPI_Win *)malloc (sizeof (MPI_Win));
      ierr = MPI_Win_create (desc, desc_size, 1, mpi_info_same_size,
		             CAF_COMM_WORLD, mpi_token->desc);
#if MPI_VERSION >= 3 && !defined(CAF_MPI_LOCK_UNLOCK)
      MPI_Win_lock_all(MPI_MODE_NOCHECK, *(mpi_token->desc));
# endif
    }
#else
  /* Token contains only a list of pointers.  */
  *token = malloc (sizeof(MPI_Win));
  MPI_Win *p = *token;
#endif

#if MPI_VERSION >= 3
#ifdef GCC_GE_7
  MPI_Win_allocate(actual_size, 1, MPI_INFO_NULL, CAF_COMM_WORLD, &mem, p);
#else
  MPI_Win_allocate(actual_size, 1, mpi_info_same_size, CAF_COMM_WORLD, &mem, p);
#endif
# ifndef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock_all(MPI_MODE_NOCHECK, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
  MPI_Alloc_mem(actual_size, MPI_INFO_NULL, &mem);
  MPI_Win_create(mem, actual_size, 1, MPI_INFO_NULL, CAF_COMM_WORLD, p);
#endif // MPI_VERSION

  if(l_var)
    {
      init_array = (int *)calloc(size, sizeof(int));
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, caf_this_image-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
      MPI_Put (init_array, size, MPI_INT, caf_this_image-1,
                      0, size, MPI_INT, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock(caf_this_image-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush(caf_this_image-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK
      free(init_array);
    }

  PREFIX(sync_all) (NULL,NULL,0);

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

#ifdef GCC_GE_7
  /* The descriptor will be initialized only after the call to register.  */
  mpi_token->local_memptr = mem;
  desc->base_addr = mem;
  return;
#else
  return mem;
#endif

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
#ifndef GCC_GE_7
  return NULL;
#endif
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
  MPI_Win *p;

  while(tmp)
    {
      prev = tmp->prev;

      if(tmp->token == *token)
        {
          p = TOKEN(*token);
# ifndef CAF_MPI_LOCK_UNLOCK
          MPI_Win_unlock_all(*p);
# endif // CAF_MPI_LOCK_UNLOCK
          MPI_Win_free(p);
#ifdef GCC_GE_7
	  if ((*(mpi_caf_token_t **)token)->desc)
	    {
	      mpi_caf_token_t *mpi_token = *(mpi_caf_token_t **)token;
# ifndef CAF_MPI_LOCK_UNLOCK
	      MPI_Win_unlock_all(*(mpi_token->desc));
# endif // CAF_MPI_LOCK_UNLOCK
	      MPI_Win_free (mpi_token->desc);
	      free (mpi_token->desc);
	    }
#endif

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
PREFIX (sync_memory) (int *stat, char *errmsg, int errmsg_len)
{
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
  explicit_flush();
#endif
}


void
PREFIX (sync_all) (int *stat, char *errmsg, int errmsg_len)
{
  int ierr=0;

  if (unlikely (caf_is_finalized))
    ierr = STAT_STOPPED_IMAGE;
  else
    {
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
      explicit_flush();
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

  MPI_Type_size(MPI_COMPLEX, &t_s);

  if(t_s==size)
    {
      *dt=MPI_COMPLEX;
      return;
    }

  MPI_Type_size(MPI_DOUBLE_COMPLEX, &t_s);

  if(t_s==size)
    {
      *dt=MPI_DOUBLE_COMPLEX;
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
  MPI_Win *p_s = TOKEN(token_s), *p_g = TOKEN(token_g);
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

# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_SHARED, image_index_g-1, 0, *p_g);
# endif // CAF_MPI_LOCK_UNLOCK
      ierr = MPI_Get (tmp, dst_size*size, MPI_BYTE,
                      image_index_g-1, offset_g, dst_size*size, MPI_BYTE, *p_g);
      if (pad_str)
        memcpy ((char *) tmp + src_size, pad_str,
                dst_size-src_size);
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image_index_g-1, *p_g);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image_index_g-1, *p_g);
# endif // CAF_MPI_LOCK_UNLOCK

# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index_s-1, 0, *p_s);
# endif // CAF_MPI_LOCK_UNLOCK
      if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
          && dst_kind == src_kind)
        ierr = MPI_Put (tmp, dst_size*size, MPI_BYTE,
                        image_index_s-1, offset_s,
                        (dst_size > src_size ? src_size : dst_size) * size,
                        MPI_BYTE, *p_s);
      if (pad_str)
        ierr = MPI_Put (pad_str, dst_size-src_size, MPI_BYTE, image_index_s-1,
                        offset_s, dst_size - src_size, MPI_BYTE, *p_s);
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image_index_s-1, *p_s);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image_index_s-1, *p_s);
# endif // CAF_MPI_LOCK_UNLOCK

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

	  extent = (dest->dim[rank-1]._ubound - dest->dim[rank-1].lower_bound + 1);
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

	      extent = (src->dim[rank-1]._ubound - src->dim[rank-1].lower_bound + 1);
              array_offset_sr += (i / extent) * src->dim[rank-1]._stride;
              array_offset_sr *= GFC_DESCRIPTOR_SIZE (src);
            }
          src_offset = offset_g + array_offset_sr;

# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_lock (MPI_LOCK_SHARED, image_index_g-1, 0, *p_g);
# endif // CAF_MPI_LOCK_UNLOCK

          ierr = MPI_Get (tmp, dst_size, MPI_BYTE,
                          image_index_g-1, src_offset, src_size, MPI_BYTE, *p_g);

# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_unlock (image_index_g-1, *p_g);
# else // CAF_MPI_LOCK_UNLOCK
          MPI_Win_flush (image_index_g-1, *p_g);
# endif // CAF_MPI_LOCK_UNLOCK

# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index_s-1, 0, *p_s);
# endif // CAF_MPI_LOCK_UNLOCK

          ierr = MPI_Put (tmp, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, image_index_s-1,
                          dst_offset, GFC_DESCRIPTOR_SIZE (dest), MPI_BYTE, *p_s);
          if (pad_str)
            ierr = MPI_Put (pad_str, dst_size - src_size, MPI_BYTE, image_index_s-1,
                            dst_offset, dst_size - src_size, MPI_BYTE, *p_s);

# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_unlock (image_index_s-1, *p_s);
# else // CAF_MPI_LOCK_UNLOCK
          MPI_Win_flush (image_index_s-1, *p_s);
# endif // CAF_MPI_LOCK_UNLOCK

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
  MPI_Win *p = TOKEN(token);
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
          /* The address of source passed by the compiler points on the right
           * memory location. No offset summation is needed.  */
          void *dest_tmp = (void *) ((char *) dest->base_addr);// + offset);
          memmove (dest_tmp,src->base_addr,size*dst_size);
          return;
        }
      else
        {
#ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
#endif // CAF_MPI_LOCK_UNLOCK
          if (GFC_DESCRIPTOR_TYPE (dest) == GFC_DESCRIPTOR_TYPE (src)
              && dst_kind == src_kind)
            ierr = MPI_Put (src->base_addr, (dst_size > src_size ? src_size : dst_size)*size, MPI_BYTE,
                            image_index-1, offset,
                            (dst_size > src_size ? src_size : dst_size) * size,
                            MPI_BYTE, *p);
          if (pad_str)
	    {
	      size_t newoff = offset + (dst_size > src_size ? src_size : dst_size) * size;
	      ierr = MPI_Put (pad_str, dst_size-src_size, MPI_BYTE, image_index-1,
			      newoff, dst_size - src_size, MPI_BYTE, *p);
	    }
#ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_unlock (image_index-1, *p);
#elif NONBLOCKING_PUT
	  /* Pending puts init */
	  if(pending_puts == NULL)
	    {
	      pending_puts = calloc(1,sizeof(win_sync));
	      pending_puts->next=NULL;
	      pending_puts->win = token;
	      pending_puts->img = image_index-1;
	      last_elem = pending_puts;
	      last_elem->next = NULL;
	    }
	  else
	    {
	      last_elem->next = calloc(1,sizeof(win_sync));
	      last_elem = last_elem->next;
	      last_elem->win = token;
	      last_elem->img = image_index-1;
	      last_elem->next = NULL;
	    }
#else
	  MPI_Win_flush (image_index-1, *p);
#endif // CAF_MPI_LOCK_UNLOCK
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

      selectType (GFC_DESCRIPTOR_SIZE (src), &base_type_src);
      selectType (GFC_DESCRIPTOR_SIZE (dest), &base_type_dst);

      if(rank == 1)
        {
          MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src, &dt_s);
          MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst, &dt_d);
        }
      /* else if(rank == 2) */
      /*   { */
      /*     MPI_Type_vector(size/src->dim[0]._ubound, src->dim[0]._ubound, src->dim[1]._stride, base_type_src, &dt_s); */
      /*     MPI_Type_vector(size/dest->dim[0]._ubound, dest->dim[0]._ubound, dest->dim[1]._stride, base_type_dst, &dt_d); */
      /*   } */
      else
        {
          arr_bl = calloc (size, sizeof (int));
          arr_dsp_s = calloc (size, sizeof (int));
          arr_dsp_d = calloc (size, sizeof (int));

          for (i = 0; i < size; i++)
            arr_bl[i] = 1;

          for (i = 0; i < size; i++)
            {
              ptrdiff_t array_offset_dst = 0;
              ptrdiff_t stride = 1;
              ptrdiff_t extent = 1;
	      ptrdiff_t tot_ext = 1;
              for (j = 0; j < rank-1; j++)
                {
                  array_offset_dst += ((i / tot_ext)
                                       % (dest->dim[j]._ubound
                                          - dest->dim[j].lower_bound + 1))
                    * dest->dim[j]._stride;
                  extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
                  stride = dest->dim[j]._stride;
		  tot_ext *= extent;
                }

              array_offset_dst += (i / tot_ext) * dest->dim[rank-1]._stride;
              arr_dsp_d[i] = array_offset_dst;

              if (GFC_DESCRIPTOR_RANK (src) != 0)
                {
                  ptrdiff_t array_offset_sr = 0;
                  stride = 1;
                  extent = 1;
		  tot_ext = 1;
                  for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
                    {
                      array_offset_sr += ((i / tot_ext)
                                          % (src->dim[j]._ubound
                                             - src->dim[j].lower_bound + 1))
                        * src->dim[j]._stride;
                      extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
                      stride = src->dim[j]._stride;
		      tot_ext *= extent;
                    }

                  array_offset_sr += (i / tot_ext) * src->dim[rank-1]._stride;
                  arr_dsp_s[i] = array_offset_sr;
                }
              else
                arr_dsp_s[i] = 0;
            }

          MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
          MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);

          free (arr_bl);
          free (arr_dsp_s);
          free (arr_dsp_d);
        }

      MPI_Type_commit(&dt_s);
      MPI_Type_commit(&dt_d);

      dst_offset = offset;

# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
      ierr = MPI_Put (sr, 1, dt_s, image_index-1, dst_offset, 1, dt_d, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image_index-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image_index-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK

      if (ierr != 0)
        {
          error_stop (ierr);
          return;
        }

      MPI_Type_free (&dt_s);
      MPI_Type_free (&dt_d);

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
      /*         { */
      /*           array_offset_sr += ((i / (extent*stride)) */
      /*                               % (src->dim[j]._ubound */
      /*                                  - src->dim[j].lower_bound + 1)) */
      /*             * src->dim[j]._stride; */
      /*           extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1); */
      /*         stride = src->dim[j]._stride; */
      /*         } */

      /*     array_offset_sr += (i / extent) * src->dim[rank-1]._stride; */

      /*     void *sr = (void *)((char *) src->base_addr */
      /*                           + array_offset_sr*GFC_DESCRIPTOR_SIZE (src)); */

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

# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image_index-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
      for (i = 0; i < size; i++)
        {
          ptrdiff_t array_offset_dst = 0;
          ptrdiff_t stride = 1;
          ptrdiff_t extent = 1;
	  ptrdiff_t tot_ext = 1;
          for (j = 0; j < rank-1; j++)
            {
              array_offset_dst += ((i / tot_ext)
                                   % (dest->dim[j]._ubound
                                      - dest->dim[j].lower_bound + 1))
                * dest->dim[j]._stride;
              extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
              stride = dest->dim[j]._stride;
	      tot_ext *= extent;
            }

          array_offset_dst += (i / tot_ext) * dest->dim[rank-1]._stride;
          dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);

          void *sr;
          if (GFC_DESCRIPTOR_RANK (src) != 0)
            {
              ptrdiff_t array_offset_sr = 0;
              stride = 1;
              extent = 1;
	      tot_ext = 1;
              for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
                {
                  array_offset_sr += ((i / tot_ext)
                                      % (src->dim[j]._ubound
                                         - src->dim[j].lower_bound + 1))
                    * src->dim[j]._stride;
                  extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
                  stride = src->dim[j]._stride;
		  tot_ext *= extent;
                }

              array_offset_sr += (i / tot_ext) * src->dim[rank-1]._stride;
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
		  ptrdiff_t tot_ext = 1;
                  for (j = 0; j < rank-1; j++)
                    {
                      array_offset_dst += ((i / tot_ext)
                                           % (dest->dim[j]._ubound
                                              - dest->dim[j].lower_bound + 1))
                        * dest->dim[j]._stride;
                      extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
                      stride = dest->dim[j]._stride;
		      tot_ext *= extent;
                    }

		  //extent = (dest->dim[rank-1]._ubound - dest->dim[rank-1].lower_bound + 1);
                  array_offset_dst += (i / tot_ext) * dest->dim[rank-1]._stride;
                  dst_offset = offset + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest);
                  memmove(src->base_addr+dst_offset,t_buff+i*GFC_DESCRIPTOR_SIZE (src),GFC_DESCRIPTOR_SIZE (src));
                }
            }
          free(t_buff);
          free(buff_map);
        }
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image_index-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image_index-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#endif
    }
}


/* Get array data from a remote src to a local dest.  */

void
PREFIX (get) (caf_token_t token, size_t offset,
              int image_index,
              gfc_descriptor_t *src ,
              caf_vector_t *src_vector __attribute__ ((unused)),
              gfc_descriptor_t *dest, int src_kind, int dst_kind,
              bool mrt)
{
  size_t i, size;
  int ierr = 0;
  int j;
  MPI_Win *p = TOKEN(token);
  int rank = GFC_DESCRIPTOR_RANK (src);
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
      /*  if (async == false) */
      if(caf_this_image == image_index)
        {
          /* The address of source passed by the compiler points on the right
           * memory location. No offset summation is needed.  */
          void *src_tmp = (void *) ((char *) src->base_addr);// + offset);
          memmove(dest->base_addr,src_tmp,size*src_size);
          return;
        }
      else
        {
# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
          ierr = MPI_Get (dest->base_addr, dst_size*size, MPI_BYTE,
                          image_index-1, offset, dst_size*size, MPI_BYTE, *p);
          if (pad_str)
            memcpy ((char *) dest->base_addr + src_size, pad_str,
                    dst_size-src_size);
# ifdef CAF_MPI_LOCK_UNLOCK
          MPI_Win_unlock (image_index-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
          MPI_Win_flush (image_index-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK
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

  selectType(GFC_DESCRIPTOR_SIZE (src), &base_type_src);
  selectType(GFC_DESCRIPTOR_SIZE (dest), &base_type_dst);

  if(rank == 1)
    {
      MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src, &dt_s);
      MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst, &dt_d);
    }
  /* else if(rank == 2) */
  /*   { */
  /*     MPI_Type_vector(size/src->dim[0]._ubound, src->dim[0]._ubound, src->dim[1]._stride, base_type_src, &dt_s); */
  /*     MPI_Type_vector(size/dest->dim[0]._ubound, dest->dim[0]._ubound, dest->dim[1]._stride, base_type_dst, &dt_d); */
  /*   } */
  else
    {
      arr_bl = calloc(size, sizeof(int));
      arr_dsp_s = calloc(size, sizeof(int));
      arr_dsp_d = calloc(size, sizeof(int));

      for(i=0;i<size;i++)
        arr_bl[i]=1;

      for (i = 0; i < size; i++)
        {
          ptrdiff_t array_offset_dst = 0;
          ptrdiff_t stride = 1;
          ptrdiff_t extent = 1;
	  ptrdiff_t tot_ext = 1;
          for (j = 0; j < rank-1; j++)
            {
              array_offset_dst += ((i / tot_ext)
                                   % (dest->dim[j]._ubound
                                      - dest->dim[j].lower_bound + 1))
                * dest->dim[j]._stride;
              extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
              stride = dest->dim[j]._stride;
	      tot_ext *= extent;
            }

	  //extent = (dest->dim[rank-1]._ubound - dest->dim[rank-1].lower_bound + 1);
          array_offset_dst += (i / tot_ext) * dest->dim[rank-1]._stride;
          arr_dsp_d[i] = array_offset_dst;

          ptrdiff_t array_offset_sr = 0;
          stride = 1;
          extent = 1;
	  tot_ext = 1;
          for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
            {
              array_offset_sr += ((i / tot_ext)
                                  % (src->dim[j]._ubound
                                     - src->dim[j].lower_bound + 1))
                * src->dim[j]._stride;
              extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
              stride = src->dim[j]._stride;
	      tot_ext *= extent;
            }

	  //extent = (src->dim[rank-1]._ubound - src->dim[rank-1].lower_bound + 1);
          array_offset_sr += (i / tot_ext) * src->dim[rank-1]._stride;
          arr_dsp_s[i] = array_offset_sr;

        }

      MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
      MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);

      free(arr_bl);
      free(arr_dsp_s);
      free(arr_dsp_d);
    }

  MPI_Type_commit(&dt_s);
  MPI_Type_commit(&dt_d);

  //sr_off = offset;

# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK

  ierr = MPI_Get (dst, 1, dt_d, image_index-1, offset, 1, dt_s, *p);

# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image_index-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image_index-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK

  if (ierr != 0)
    error_stop (ierr);

  MPI_Type_free(&dt_s);
  MPI_Type_free(&dt_d);

#else
  if(caf_this_image == image_index && mrt)
    {
      t_buff = calloc(size,GFC_DESCRIPTOR_SIZE (dest));
      buff_map = calloc(size,sizeof(bool));
    }

# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_SHARED, image_index-1, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  for (i = 0; i < size; i++)
    {
      ptrdiff_t array_offset_dst = 0;
      ptrdiff_t stride = 1;
      ptrdiff_t extent = 1;
      ptrdiff_t tot_ext = 1;
      for (j = 0; j < rank-1; j++)
        {
          array_offset_dst += ((i / tot_ext)
                               % (dest->dim[j]._ubound
                                  - dest->dim[j].lower_bound + 1))
                              * dest->dim[j]._stride;
          extent = (dest->dim[j]._ubound - dest->dim[j].lower_bound + 1);
          stride = dest->dim[j]._stride;
	  tot_ext *= extent;
        }

      array_offset_dst += (i / tot_ext) * dest->dim[rank-1]._stride;

      ptrdiff_t array_offset_sr = 0;
      stride = 1;
      extent = 1;
      tot_ext = 1;
      for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
        {
          array_offset_sr += ((i / tot_ext)
                           % (src->dim[j]._ubound
                              - src->dim[j].lower_bound + 1))
                          * src->dim[j]._stride;
          extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
          stride = src->dim[j]._stride;
	  tot_ext *= extent;
        }

      array_offset_sr += (i / tot_ext) * src->dim[rank-1]._stride;

      size_t sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src);
      void *dst = (void *) ((char *) dest->base_addr
                            + array_offset_dst*GFC_DESCRIPTOR_SIZE (dest));
      /* FIXME: Handle image_index == this_image().  */
      /*  if (async == false) */
      if(caf_this_image == image_index)
        {
          /* Is this needed? */
          if(!mrt)
            memmove(dst,src->base_addr+array_offset_sr*GFC_DESCRIPTOR_SIZE(src),GFC_DESCRIPTOR_SIZE (src));
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
	      ptrdiff_t tot_ext = 1;
              for (j = 0; j < GFC_DESCRIPTOR_RANK (src)-1; j++)
                {
                  array_offset_sr += ((i / tot_ext)
                                      % (src->dim[j]._ubound
                                         - src->dim[j].lower_bound + 1))
                    * src->dim[j]._stride;
                  extent = (src->dim[j]._ubound - src->dim[j].lower_bound + 1);
                  stride = src->dim[j]._stride;
		  tot_ext *= extent;
                }

	      //extent = (src->dim[rank-1]._ubound - src->dim[rank-1].lower_bound + 1);
              array_offset_sr += (i / tot_ext) * src->dim[rank-1]._stride;

              size_t sr_off = offset + array_offset_sr*GFC_DESCRIPTOR_SIZE (src);

              memmove(dest->base_addr+sr_off,t_buff+i*GFC_DESCRIPTOR_SIZE (src),GFC_DESCRIPTOR_SIZE (src));
            }
        }
      free(t_buff);
      free(buff_map);
    }
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image_index-1, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image_index-1, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#endif
}


#ifdef GCC_GE_7
/* Emitted when a theorectically unreachable part is reached.  */
const char unreachable[] = "Fatal error: unreachable alternative found.\n";

static void
assign_char4_from_char1 (size_t dst_size, size_t src_size, uint32_t *dst,
			 unsigned char *src)
{
  size_t i, n;
  n = dst_size/4 > src_size ? src_size : dst_size/4;
  for (i = 0; i < n; ++i)
    dst[i] = (int32_t) src[i];
  for (; i < dst_size/4; ++i)
    dst[i] = (int32_t) ' ';
}


static void
assign_char1_from_char4 (size_t dst_size, size_t src_size, unsigned char *dst,
			 uint32_t *src)
{
  size_t i, n;
  n = dst_size > src_size/4 ? src_size/4 : dst_size;
  for (i = 0; i < n; ++i)
    dst[i] = src[i] > UINT8_MAX ? (unsigned char) '?' : (unsigned char) src[i];
  if (dst_size > n)
    memset (&dst[n], ' ', dst_size - n);
}

static void
convert_type (void *dst, int dst_type, int dst_kind, void *src, int src_type,
	      int src_kind, int *stat)
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
      return;
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
      return;
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
      return;
    default:
      goto error;
    }

error:
  fprintf (stderr, "libcaf_single RUNTIME ERROR: Cannot convert type %d kind "
	   "%d to type %d kind %d\n", src_type, src_kind, dst_type, dst_kind);
  if (stat)
    *stat = 1;
  else
    abort ();
}


#ifdef CAF_MPI_LOCK_UNLOCK
#define CAF_Win_lock(img, win) MPI_Win_lock (MPI_LOCK_SHARED, img, 0, win)
#define CAF_Win_unlock(img, win) MPI_Win_unlock (img, win)
#else //CAF_MPI_LOCK_UNLOCK
#define CAF_Win_lock(img, win)
#define CAF_Win_unlock(img, win) MPI_Win_flush (img, win)
#endif //CAF_MPI_LOCK_UNLOCK


static void
copy_data (void *ds, mpi_caf_token_t *token, ptrdiff_t offset, int dst_type,
	   int src_type, int dst_kind, int src_kind, size_t dst_size,
	   size_t src_size, size_t num, int *stat, int image_index)
{
  size_t k;
  if (dst_type == src_type && dst_kind == src_kind)
    {
      size_t sz = (dst_size > src_size ? src_size : dst_size) * num;
      CAF_Win_lock (image_index, token->memptr);
      MPI_Get (ds, sz, MPI_BYTE, image_index, offset, sz, MPI_BYTE,
	       token->memptr);
      CAF_Win_unlock (image_index, token->memptr);
      if ((dst_type == BT_CHARACTER || src_type == BT_CHARACTER)
	  && dst_size > src_size)
	{
	  if (dst_kind == 1)
	    memset ((void*)(char*) ds + src_size, ' ', dst_size-src_size);
	  else /* dst_kind == 4.  */
	    for (k = src_size/4; k < dst_size/4; k++)
	      ((int32_t*) ds)[k] = (int32_t) ' ';
	}
    }
  else if (dst_type == BT_CHARACTER && dst_kind == 1)
    {
      /* Get the required amount of memory on the stack.  */
      void *srh = alloca (src_size);
      CAF_Win_lock (image_index, token->memptr);
      MPI_Get (srh, src_size, MPI_BYTE, image_index, offset,
	       src_size, MPI_BYTE, token->memptr);
      CAF_Win_unlock (image_index, token->memptr);
      assign_char1_from_char4 (dst_size, src_size, ds, srh);
    }
  else if (dst_type == BT_CHARACTER)
    {
      /* Get the required amount of memory on the stack.  */
      void *srh = alloca (src_size);
      CAF_Win_lock (image_index, token->memptr);
      MPI_Get (srh, src_size, MPI_BYTE, image_index, offset,
	       src_size, MPI_BYTE, token->memptr);
      CAF_Win_unlock (image_index, token->memptr);
      assign_char4_from_char1 (dst_size, src_size, ds, srh);
    }
  else
    {
      /* Get the required amount of memory on the stack.  */
      void *srh = alloca (src_size * num);
      CAF_Win_lock (image_index, token->memptr);
      MPI_Get (srh, src_size * num, MPI_BYTE, image_index, offset,
	       src_size * num, MPI_BYTE, token->memptr);
      CAF_Win_unlock (image_index, token->memptr);
      for (k = 0; k < num; ++k)
	{
	  convert_type (ds, dst_type, dst_kind, srh, src_type, src_kind, stat);
	  ds += dst_size;
	  srh += src_size;
	}
    }
}


#define COMPUTE_NUM_ITEMS(num, stride, lb, ub) \
  do { \
    ptrdiff_t abs_stride = (stride) > 0 ? (stride) : -(stride); \
    num = (stride) > 0 ? (ub) + 1 - (lb) : (lb) + 1 - (ub); \
    if (num <= 0 || abs_stride < 1) return; \
    num = (abs_stride > 1) ? (1 + (num - 1) / abs_stride) : num; \
  } while (0)


#define GFC_DESCRIPTOR_EXTENT(desc,i) ((desc)->dim[i]._ubound + 1 \
				      - (desc)->dim[i].lower_bound)


#define GET_REMOTE_DESC(mpi_token, src, src_desc_data, image_index) \
  if (mpi_token->desc) \
    { \
      size_t desc_size = sizeof (gfc_descriptor_t) + GFC_MAX_DIMENSIONS /* rank */ \
	* sizeof (descriptor_dimension); \
      int err; \
      CAF_Win_lock (image_index, *(mpi_token->desc)); \
      MPI_Get (&src_desc_data, desc_size, MPI_BYTE, \
	       image_index, 0, desc_size, MPI_BYTE, *(mpi_token->desc)); \
      err = CAF_Win_unlock (image_index, *(mpi_token->desc)); \
      src = (gfc_descriptor_t *)&src_desc_data; \
    } \
  else \
    src = NULL


typedef struct gfc_max_dim_descriptor_t {
  void *base_addr;
  size_t offset;
  ptrdiff_t dtype;
  descriptor_dimension dim[GFC_MAX_DIMENSIONS];
} gfc_max_dim_descriptor_t;

static void
get_for_ref (caf_reference_t *ref, size_t *i, size_t dst_index,
	     mpi_caf_token_t *mpi_token, gfc_descriptor_t *dst,
	     gfc_descriptor_t *src, void *ds, void *sr,
	     ptrdiff_t sr_byte_offset,
	     int dst_kind, int src_kind, size_t dst_dim, size_t src_dim,
	     size_t num, int *stat, int image_index)
/* !!! The image_index is zero-base here.  */
{
  ptrdiff_t extent_src = 1, array_offset_src = 0, stride_src;
  size_t next_dst_dim;
  gfc_max_dim_descriptor_t src_desc_data;

  if (unlikely (ref == NULL))
    /* May be we should issue an error here, because this case should not
       occur.  */
    return;

  if (ref->next == NULL)
    {
      size_t dst_size = GFC_DESCRIPTOR_SIZE (dst);
      size_t dst_rank = GFC_DESCRIPTOR_RANK (dst);
      int src_type = -1;

      switch (ref->type)
	{
	case CAF_REF_COMPONENT:
	  /* Because the token is always registered after the component, its
	     offset is always greater zeor.  */
	  if (ref->u.c.caf_token_offset > 0)
	    copy_data (ds, mpi_token, ref->u.c.offset,
		       GFC_DESCRIPTOR_TYPE (dst), GFC_DESCRIPTOR_TYPE (dst),
		       dst_kind, src_kind, dst_size, ref->item_size, 1, stat,
		       image_index);
	  else
	    copy_data (ds, mpi_token, ref->u.c.offset,
		       GFC_DESCRIPTOR_TYPE (dst), GFC_DESCRIPTOR_TYPE (src),
		       dst_kind, src_kind, dst_size, ref->item_size, 1, stat,
		       image_index);
	  ++(*i);
	  return;
	case CAF_REF_STATIC_ARRAY:
	  src_type = ref->u.a.static_array_type;
	  /* Intentionally fall through.  */
	case CAF_REF_ARRAY:
	  if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
	    {
	      copy_data (ds + dst_index * dst_size, mpi_token,
			 sr_byte_offset, GFC_DESCRIPTOR_TYPE (dst),
			 src_type == -1 ? GFC_DESCRIPTOR_TYPE (src) : src_type,
			 dst_kind, src_kind, dst_size, ref->item_size, num,
			 stat, image_index);
	      *i += num;
	      return;
	    }
	  break;
	default:
	  caf_runtime_error (unreachable);
	}
    }

  switch (ref->type)
    {
    case CAF_REF_COMPONENT:
      if (ref->u.c.caf_token_offset > 0)
	{
	  mpi_token = *(mpi_caf_token_t**)(sr + ref->u.c.caf_token_offset);
	  GET_REMOTE_DESC (mpi_token, src, src_desc_data, image_index);
	  get_for_ref (ref->next, i, dst_index, mpi_token, dst, src,
		       ds, sr + ref->u.c.offset, ref->u.c.offset,
		       dst_kind, src_kind, dst_dim, 0, 1, stat, image_index);
	}
      else
	get_for_ref (ref->next, i, dst_index, mpi_token, dst,
		     (gfc_descriptor_t *)(sr + ref->u.c.offset), ds,
		     sr + ref->u.c.offset, ref->u.c.offset, dst_kind, src_kind,
		     dst_dim, 0, 1, stat, image_index);
      return;
    case CAF_REF_ARRAY:
      if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
	{
	  get_for_ref (ref->next, i, dst_index, mpi_token, dst,
		       src, ds, sr, sr_byte_offset, dst_kind,
		       src_kind, dst_dim, 0, 1, stat, image_index);
	  return;
	}
      /* Only when on the left most index switch the data pointer to
	 the array's data pointer.  */
      if (src_dim == 0)
	{
	  sr = src->base_addr;
	  sr_byte_offset = 0;
	}
      switch (ref->u.a.mode[src_dim])
	{
	case CAF_ARR_REF_VECTOR:
	  extent_src = GFC_DESCRIPTOR_EXTENT (src, src_dim);
	  array_offset_src = 0;
	  for (size_t idx = 0; idx < ref->u.a.dim[src_dim].v.nvec;
	       ++idx)
	    {
#define KINDCASE(kind, type) case kind: \
	      array_offset_src = (((ptrdiff_t) \
		  ((type *)ref->u.a.dim[src_dim].v.vector)[idx]) \
		  - src->dim[src_dim].lower_bound \
		  * src->dim[src_dim]._stride); \
	      break

	      switch (ref->u.a.dim[src_dim].v.kind)
		{
		KINDCASE (1, int8_t);
		KINDCASE (2, int16_t);
		KINDCASE (4, int32_t);
		KINDCASE (8, int64_t);
#ifdef HAVE_INT128_T
		KINDCASE (16, int128_t);
#endif
		default:
		  caf_runtime_error (unreachable);
		  return;
		}
#undef KINDCASE

	      get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	    }
	  return;
	case CAF_ARR_REF_FULL:
	  COMPUTE_NUM_ITEMS (extent_src,
			     ref->u.a.dim[src_dim].s.stride,
			     src->dim[src_dim].lower_bound,
			     src->dim[src_dim]._ubound);
	  stride_src = src->dim[src_dim]._stride
	      * ref->u.a.dim[src_dim].s.stride;
	  array_offset_src = 0;
	  for (ptrdiff_t idx = 0; idx < extent_src;
	       ++idx, array_offset_src += stride_src)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	    }
	  return;
	case CAF_ARR_REF_RANGE:
	  COMPUTE_NUM_ITEMS (extent_src,
			     ref->u.a.dim[src_dim].s.stride,
			     ref->u.a.dim[src_dim].s.start,
			     ref->u.a.dim[src_dim].s.end);
	  array_offset_src = (ref->u.a.dim[src_dim].s.start
			      - src->dim[src_dim].lower_bound)
	      * src->dim[src_dim]._stride;
	  stride_src = src->dim[src_dim]._stride
	      * ref->u.a.dim[src_dim].s.stride;
	  /* Increase the dst_dim only, when the src_extent is greater one
	     or src and dst extent are both one.  Don't increase when the scalar
	     source is not present in the dst.  */
	  next_dst_dim = extent_src > 1
	      || (GFC_DESCRIPTOR_EXTENT (dst, dst_dim) == 1
		  && extent_src == 1) ? (dst_dim + 1) : dst_dim;
	  for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, next_dst_dim, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	      array_offset_src += stride_src;
	    }
	  return;
	case CAF_ARR_REF_SINGLE:
	  array_offset_src = (ref->u.a.dim[src_dim].s.start
			      - src->dim[src_dim].lower_bound)
	      * src->dim[src_dim]._stride;
	  get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
		       sr + array_offset_src * ref->item_size,
		       sr_byte_offset + array_offset_src * ref->item_size,
		       dst_kind, src_kind, dst_dim, src_dim + 1, 1,
		       stat, image_index);
	  return;
	case CAF_ARR_REF_OPEN_END:
	  COMPUTE_NUM_ITEMS (extent_src,
			     ref->u.a.dim[src_dim].s.stride,
			     ref->u.a.dim[src_dim].s.start,
			     src->dim[src_dim]._ubound);
	  stride_src = src->dim[src_dim]._stride
	      * ref->u.a.dim[src_dim].s.stride;
	  array_offset_src = (ref->u.a.dim[src_dim].s.start
			      - src->dim[src_dim].lower_bound)
	      * src->dim[src_dim]._stride;
	  for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	      array_offset_src += stride_src;
	    }
	  return;
	case CAF_ARR_REF_OPEN_START:
	  COMPUTE_NUM_ITEMS (extent_src,
			     ref->u.a.dim[src_dim].s.stride,
			     src->dim[src_dim].lower_bound,
			     ref->u.a.dim[src_dim].s.end);
	  stride_src = src->dim[src_dim]._stride
	      * ref->u.a.dim[src_dim].s.stride;
	  array_offset_src = 0;
	  for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, src, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	      array_offset_src += stride_src;
	    }
	  return;
	default:
	  caf_runtime_error (unreachable);
	}
      return;
    case CAF_REF_STATIC_ARRAY:
      if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
	{
	  get_for_ref (ref->next, i, dst_index, mpi_token, dst, NULL, ds, sr,
		       sr_byte_offset, dst_kind, src_kind,
		       dst_dim, 0, 1, stat, image_index);
	  return;
	}
      switch (ref->u.a.mode[src_dim])
	{
	case CAF_ARR_REF_VECTOR:
	  array_offset_src = 0;
	  for (size_t idx = 0; idx < ref->u.a.dim[src_dim].v.nvec;
	       ++idx)
	    {
#define KINDCASE(kind, type) case kind: \
	     array_offset_src = ((type *)ref->u.a.dim[src_dim].v.vector)[idx]; \
	      break

	      switch (ref->u.a.dim[src_dim].v.kind)
		{
		KINDCASE (1, int8_t);
		KINDCASE (2, int16_t);
		KINDCASE (4, int32_t);
		KINDCASE (8, int64_t);
#ifdef HAVE_INT128_T
		KINDCASE (16, int128_t);
#endif
		default:
		  caf_runtime_error (unreachable);
		  return;
		}
#undef KINDCASE

	      get_for_ref (ref, i, dst_index, mpi_token, dst, NULL, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	    }
	  return;
	case CAF_ARR_REF_FULL:
	  for (array_offset_src = 0 ;
	       array_offset_src <= ref->u.a.dim[src_dim].s.end;
	       array_offset_src += ref->u.a.dim[src_dim].s.stride)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, NULL, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	    }
	  return;
	case CAF_ARR_REF_RANGE:
	  COMPUTE_NUM_ITEMS (extent_src,
			     ref->u.a.dim[src_dim].s.stride,
			     ref->u.a.dim[src_dim].s.start,
			     ref->u.a.dim[src_dim].s.end);
	  array_offset_src = ref->u.a.dim[src_dim].s.start;
	  for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
	    {
	      get_for_ref (ref, i, dst_index, mpi_token, dst, NULL, ds,
			   sr + array_offset_src * ref->item_size,
			   sr_byte_offset + array_offset_src * ref->item_size,
			   dst_kind, src_kind, dst_dim + 1, src_dim + 1,
			   1, stat, image_index);
	      dst_index += dst->dim[dst_dim]._stride;
	      array_offset_src += ref->u.a.dim[src_dim].s.stride;
	    }
	  return;
	case CAF_ARR_REF_SINGLE:
	  array_offset_src = ref->u.a.dim[src_dim].s.start;
	  get_for_ref (ref, i, dst_index, mpi_token, dst, NULL, ds,
		       sr + array_offset_src * ref->item_size,
		       sr_byte_offset + array_offset_src * ref->item_size,
		       dst_kind, src_kind, dst_dim, src_dim + 1, 1,
		       stat, image_index);
	  return;
	/* The OPEN_* are mapped to a RANGE and therefore can not occur.  */
	case CAF_ARR_REF_OPEN_END:
	case CAF_ARR_REF_OPEN_START:
	default:
	  caf_runtime_error (unreachable);
	}
      return;
    default:
      caf_runtime_error (unreachable);
    }
}


void
_gfortran_caf_get_by_ref (caf_token_t token, int image_index,
			  gfc_descriptor_t *dst, caf_reference_t *refs,
			  int dst_kind, int src_kind,
			  bool may_require_tmp __attribute__ ((unused)),
			  bool dst_reallocatable, int *stat)
{
  const char vecrefunknownkind[] = "libcaf_single::caf_get_by_ref(): "
				   "unknown kind in vector-ref.\n";
  const char unknownreftype[] = "libcaf_single::caf_get_by_ref(): "
				"unknown reference type.\n";
  const char unknownarrreftype[] = "libcaf_single::caf_get_by_ref(): "
				   "unknown array reference type.\n";
  const char rankoutofrange[] = "libcaf_single::caf_get_by_ref(): "
				"rank out of range.\n";
  const char extentoutofrange[] = "libcaf_single::caf_get_by_ref(): "
				  "extent out of range.\n";
  const char cannotallocdst[] = "libcaf_single::caf_get_by_ref(): "
				"can not allocate memory.\n";
  const char nonallocextentmismatch[] = "libcaf_single::caf_get_by_ref(): "
      "extent of non-allocatable arrays mismatch (%lu != %lu).\n";
  const char doublearrayref[] = "libcaf_single::caf_get_by_ref(): "
      "two or more array part references are not supported.\n";
  size_t size, i;
  size_t dst_index;
  int dst_rank = GFC_DESCRIPTOR_RANK (dst);
  int dst_cur_dim = 0;
  size_t src_size;
  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *) token;
  void *local_memptr = mpi_token->local_memptr;
  gfc_descriptor_t *src;
  gfc_max_dim_descriptor_t src_desc_data, primary_src_desc_data;
  caf_reference_t *riter = refs;
  long delta;
  /* Reallocation of dst.data is needed (e.g., array to small).  */
  bool realloc_needed;
  /* Reallocation of dst.data is required, because data is not alloced at
     all.  */
  bool realloc_required;
  bool extent_mismatch = false;
  /* Set when the first non-scalar array reference is encountered.  */
  bool in_array_ref = false;
  bool array_extent_fixed = false;
  realloc_needed = realloc_required = dst->base_addr == NULL;

  if (stat)
    *stat = 0;

  GET_REMOTE_DESC (mpi_token, src, primary_src_desc_data, image_index - 1);
  /* Compute the size of the result.  In the beginning size just counts the
     number of elements.  */
  size = 1;
  while (riter)
    {
      switch (riter->type)
	{
	case CAF_REF_COMPONENT:
	  if (riter->u.c.caf_token_offset)
	    {
	      mpi_token = *(mpi_caf_token_t**)
				   (local_memptr + riter->u.c.caf_token_offset);
	      local_memptr = mpi_token->local_memptr;
	      GET_REMOTE_DESC (mpi_token, src, src_desc_data, image_index - 1);
	    }
	  else
	    {
	      local_memptr += riter->u.c.offset;
	      src = (gfc_descriptor_t *)local_memptr;
	    }
	  break;
	case CAF_REF_ARRAY:
	  for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
	    {
	      switch (riter->u.a.mode[i])
		{
		case CAF_ARR_REF_VECTOR:
		  delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type) case kind: \
		    local_memptr += (((ptrdiff_t) \
			((type *)riter->u.a.dim[i].v.vector)[0]) \
			- src->dim[i].lower_bound) \
			* src->dim[i]._stride \
			* riter->item_size; \
		    break

		  switch (riter->u.a.dim[i].v.kind)
		    {
		    KINDCASE (1, int8_t);
		    KINDCASE (2, int16_t);
		    KINDCASE (4, int32_t);
		    KINDCASE (8, int64_t);
#if HAVE_INT128_T
		    KINDCASE (16, int128_t);
#endif
		    default:
		      caf_runtime_error (vecrefunknownkind, stat, NULL, 0);
		      return;
		    }
#undef KINDCASE
		  break;
		case CAF_ARR_REF_FULL:
		  COMPUTE_NUM_ITEMS (delta,
				     riter->u.a.dim[i].s.stride,
				     src->dim[i].lower_bound,
				     src->dim[i]._ubound);
		  /* The memptr stays unchanged when ref'ing the first element
		     in a dimension.  */
		  break;
		case CAF_ARR_REF_RANGE:
		  COMPUTE_NUM_ITEMS (delta,
				     riter->u.a.dim[i].s.stride,
				     riter->u.a.dim[i].s.start,
				     riter->u.a.dim[i].s.end);
		  local_memptr += (riter->u.a.dim[i].s.start
			     - src->dim[i].lower_bound)
		      * src->dim[i]._stride
		      * riter->item_size;
		  break;
		case CAF_ARR_REF_SINGLE:
		  delta = 1;
		  local_memptr += (riter->u.a.dim[i].s.start
			     - src->dim[i].lower_bound)
		      * src->dim[i]._stride
		      * riter->item_size;
		  break;
		case CAF_ARR_REF_OPEN_END:
		  COMPUTE_NUM_ITEMS (delta,
				     riter->u.a.dim[i].s.stride,
				     riter->u.a.dim[i].s.start,
				     src->dim[i]._ubound);
		  local_memptr += (riter->u.a.dim[i].s.start
			     - src->dim[i].lower_bound)
		      * src->dim[i]._stride
		      * riter->item_size;
		  break;
		case CAF_ARR_REF_OPEN_START:
		  COMPUTE_NUM_ITEMS (delta,
				     riter->u.a.dim[i].s.stride,
				     src->dim[i].lower_bound,
				     riter->u.a.dim[i].s.end);
		  /* The memptr stays unchanged when ref'ing the first element
		     in a dimension.  */
		  break;
		default:
		  caf_runtime_error (unknownarrreftype, stat, NULL, 0);
		  return;
		}
	      if (delta <= 0)
		return;
	      /* Check the various properties of the destination array.
		 Is an array expected and present?  */
	      if (delta > 1 && dst_rank == 0)
		{
		  /* No, an array is required, but not provided.  */
		  caf_runtime_error (extentoutofrange, stat, NULL, 0);
		  return;
		}
	      /* When dst is an array.  */
	      if (dst_rank > 0)
		{
		  /* Check that dst_cur_dim is valid for dst.  Can be
		     superceeded only by scalar data.  */
		  if (dst_cur_dim >= dst_rank && delta != 1)
		    {
		      caf_runtime_error (rankoutofrange, stat, NULL, 0);
		      return;
		    }
		  /* Do further checks, when the source is not scalar.  */
		  else if (delta != 1)
		    {
		      /* Check that the extent is not scalar and we are not in
			 an array ref for the dst side.  */
		      if (!in_array_ref)
			{
			  /* Check that this is the non-scalar extent.  */
			  if (!array_extent_fixed)
			    {
			      /* In an array extent now.  */
			      in_array_ref = true;
			      /* Check that we haven't skipped any scalar
				 dimensions yet and that the dst is
				 compatible.  */
			      if (i > 0
				  && dst_rank == GFC_DESCRIPTOR_RANK (src))
				{
				  if (dst_reallocatable)
				    {
				      /* Dst is reallocatable, which means that
					 the bounds are not set.  Set them.  */
				      for (dst_cur_dim= 0; dst_cur_dim < (int)i;
					   ++dst_cur_dim)
					{
					  dst->dim[dst_cur_dim].lower_bound = 1;
					  dst->dim[dst_cur_dim]._ubound = 1;
					  dst->dim[dst_cur_dim]._stride = 1;
					}
				    }
				  else
				    dst_cur_dim = i;
				}
			      /* Else press thumbs, that there are enough
				 dimensional refs to come.  Checked below.  */
			    }
			  else
			    {
			      caf_runtime_error (doublearrayref, stat, NULL,
						  0);
			      return;
			    }
			}
		      /* When the realloc is required, then no extent may have
			 been set.  */
		      extent_mismatch = realloc_required
			  || GFC_DESCRIPTOR_EXTENT (dst, dst_cur_dim) != delta;
		      /* When it already known, that a realloc is needed or
			 the extent does not match the needed one.  */
		      if (realloc_required || realloc_needed
			  || extent_mismatch)
			{
			  /* Check whether dst is reallocatable.  */
			  if (unlikely (!dst_reallocatable))
			    {
			      caf_runtime_error (nonallocextentmismatch, stat,
						  NULL, 0, delta,
						  GFC_DESCRIPTOR_EXTENT (dst,
								  dst_cur_dim));
			      return;
			    }
			  /* Only report an error, when the extent needs to be
			     modified, which is not allowed.  */
			  else if (!dst_reallocatable && extent_mismatch)
			    {
			      caf_runtime_error (extentoutofrange, stat, NULL,
						  0);
			      return;
			    }
			  realloc_needed = true;
			}
		      /* Only change the extent when it does not match.  This is
			 to prevent resetting given array bounds.  */
		      if (extent_mismatch)
			{
			  dst->dim[dst_cur_dim].lower_bound = 1;
			  dst->dim[dst_cur_dim]._ubound = delta;
			  dst->dim[dst_cur_dim]._stride = size;
			}
		    }

		  /* Only increase the dim counter, when in an array ref.  */
		  if (in_array_ref && dst_cur_dim < dst_rank)
		    ++dst_cur_dim;
		}
	      size *= (ptrdiff_t)delta;
	    }
	  if (in_array_ref)
	    {
	      array_extent_fixed = true;
	      in_array_ref = false;
	    }
	  break;
	case CAF_REF_STATIC_ARRAY:
	  for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
	    {
	      switch (riter->u.a.mode[i])
		{
		case CAF_ARR_REF_VECTOR:
		  delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type) case kind: \
		    local_memptr += ((type *)riter->u.a.dim[i].v.vector)[0] \
			* riter->item_size; \
		    break

		  switch (riter->u.a.dim[i].v.kind)
		    {
		    KINDCASE (1, int8_t);
		    KINDCASE (2, int16_t);
		    KINDCASE (4, int32_t);
		    KINDCASE (8, int64_t);
#if HAVE_INT128_T
		    KINDCASE (16, int128_t);
#endif
		    default:
		      caf_runtime_error (vecrefunknownkind, stat, NULL, 0);
		      return;
		    }
#undef KINDCASE
		  break;
		case CAF_ARR_REF_FULL:
		  delta = riter->u.a.dim[i].s.end / riter->u.a.dim[i].s.stride
		      + 1;
		  /* The memptr stays unchanged when ref'ing the first element
		     in a dimension.  */
		  break;
		case CAF_ARR_REF_RANGE:
		  COMPUTE_NUM_ITEMS (delta,
				     riter->u.a.dim[i].s.stride,
				     riter->u.a.dim[i].s.start,
				     riter->u.a.dim[i].s.end);
		  local_memptr += riter->u.a.dim[i].s.start
		      * riter->u.a.dim[i].s.stride
		      * riter->item_size;
		  break;
		case CAF_ARR_REF_SINGLE:
		  delta = 1;
		  local_memptr += riter->u.a.dim[i].s.start
		      * riter->u.a.dim[i].s.stride
		      * riter->item_size;
		  break;
		case CAF_ARR_REF_OPEN_END:
		  /* This and OPEN_START are mapped to a RANGE and therefore
		     can not occur here.  */
		case CAF_ARR_REF_OPEN_START:
		default:
		  caf_runtime_error (unknownarrreftype, stat, NULL, 0);
		  return;
		}
	      if (delta <= 0)
		return;
	      /* Check the various properties of the destination array.
		 Is an array expected and present?  */
	      if (delta > 1 && dst_rank == 0)
		{
		  /* No, an array is required, but not provided.  */
		  caf_runtime_error (extentoutofrange, stat, NULL, 0);
		  return;
		}
	      /* When dst is an array.  */
	      if (dst_rank > 0)
		{
		  /* Check that dst_cur_dim is valid for dst.  Can be
		     superceeded only by scalar data.  */
		  if (dst_cur_dim >= dst_rank && delta != 1)
		    {
		      caf_runtime_error (rankoutofrange, stat, NULL, 0);
		      return;
		    }
		  /* Do further checks, when the source is not scalar.  */
		  else if (delta != 1)
		    {
		      /* Check that the extent is not scalar and we are not in
			 an array ref for the dst side.  */
		      if (!in_array_ref)
			{
			  /* Check that this is the non-scalar extent.  */
			  if (!array_extent_fixed)
			    {
			      /* In an array extent now.  */
			      in_array_ref = true;
			      /* The dst is not reallocatable, so nothing more
				 to do, then correct the dim counter.  */
			      dst_cur_dim = i;
			    }
			  else
			    {
			      caf_runtime_error (doublearrayref, stat, NULL,
						  0);
			      return;
			    }
			}
		      /* When the realloc is required, then no extent may have
			 been set.  */
		      extent_mismatch = realloc_required
			  || GFC_DESCRIPTOR_EXTENT (dst, dst_cur_dim) != delta;
		      /* When it is already known, that a realloc is needed or
			 the extent does not match the needed one.  */
		      if (realloc_required || realloc_needed
			  || extent_mismatch)
			{
			  /* Check whether dst is reallocatable.  */
			  if (unlikely (!dst_reallocatable))
			    {
			      caf_runtime_error (nonallocextentmismatch, stat,
						  NULL, 0, delta,
						  GFC_DESCRIPTOR_EXTENT (dst,
								  dst_cur_dim));
			      return;
			    }
			  /* Only report an error, when the extent needs to be
			     modified, which is not allowed.  */
			  else if (!dst_reallocatable && extent_mismatch)
			    {
			      caf_runtime_error (extentoutofrange, stat, NULL,
						  0);
			      return;
			    }
			  realloc_needed = true;
			}
		      /* Only change the extent when it does not match.  This is
			 to prevent resetting given array bounds.  */
		      if (extent_mismatch)
			{
			  dst->dim[dst_cur_dim].lower_bound = 1;
			  dst->dim[dst_cur_dim]._ubound = delta;
			  dst->dim[dst_cur_dim]._stride = size;
			}
		    }
		  /* Only increase the dim counter, when in an array ref.  */
		  if (in_array_ref && dst_cur_dim < dst_rank)
		    ++dst_cur_dim;
		}
	      size *= (ptrdiff_t)delta;
	    }
	  if (in_array_ref)
	    {
	      array_extent_fixed = true;
	      in_array_ref = false;
	    }
	  break;
	default:
	  caf_runtime_error (unknownreftype, stat, NULL, 0);
	  return;
	}
      src_size = riter->item_size;
      riter = riter->next;
    }
  if (size == 0 || src_size == 0)
    return;
  /* Postcondition:
     - size contains the number of elements to store in the destination array,
     - src_size gives the size in bytes of each item in the destination array.
  */

  if (realloc_needed)
    {
      if (!array_extent_fixed)
	{
	  /* This can happen only, when the result is scalar.  */
	  for (dst_cur_dim = 0; dst_cur_dim < dst_rank; ++dst_cur_dim)
	    {
	      dst->dim[dst_cur_dim].lower_bound = 1;
	      dst->dim[dst_cur_dim]._ubound = 1;
	      dst->dim[dst_cur_dim]._stride = 1;
	    }
	}
      dst->base_addr = malloc (size * GFC_DESCRIPTOR_SIZE (dst));
      if (unlikely (dst->base_addr == NULL))
	{
	  caf_runtime_error (cannotallocdst, stat, NULL, 0);
	  return;
	}
    }

  /* Reset the token.  */
  mpi_token = (mpi_caf_token_t *) token;
  local_memptr = mpi_token->local_memptr;
  src = (gfc_descriptor_t *)&primary_src_desc_data;
  dst_index = 0;
  i = 0;
  get_for_ref (refs, &i, dst_index, mpi_token, dst, src,
	       dst->base_addr, local_memptr, 0, dst_kind, src_kind, 0, 0,
	       1, stat, image_index - 1);
}


void
PREFIX(send_by_ref) (caf_token_t token, int image_index,
                         gfc_descriptor_t *src, caf_reference_t *refs,
                         int dst_kind, int src_kind, bool may_require_tmp,
                         bool dst_reallocatable, int *stat)
{
  fprintf (stderr, "COARRAY ERROR: caf_send_by_ref() not implemented yet ");
  error_stop (1);
}


void
PREFIX(sendget_by_ref) (caf_token_t dst_token, int dst_image_index,
                            caf_reference_t *dst_refs, caf_token_t src_token,
                            int src_image_index, caf_reference_t *src_refs,
                            int dst_kind, int src_kind,
                            bool may_require_tmp, int *dst_stat, int *src_stat)
{
  fprintf (stderr, "COARRAY ERROR: caf_sendget_by_ref() not implemented yet ");
  error_stop (1);
}
#endif


/* SYNC IMAGES. Note: SYNC IMAGES(*) is passed as count == -1 while
   SYNC IMAGES([]) has count == 0. Note further that SYNC IMAGES(*)
   is not equivalent to SYNC ALL. */
void
PREFIX (sync_images) (int count, int images[], int *stat, char *errmsg,
                     int errmsg_len)
{
  int ierr = 0, i = 0, remote_stat = 0, j = 0;
  MPI_Status s;

  if (count == 0 || (count == 1 && images[0] == caf_this_image))
    {
      if (stat)
        *stat = 0;
      return;
    }

  /* halt execution if sync images contains duplicate image numbers */
  for(i = 0; i < count; ++i)
    for(j = 0; j < i; ++j)
      if(images[i] == images[j])
	{
	  ierr = STAT_DUP_SYNC_IMAGES;
	  if(stat)
	    *stat = ierr;
	  goto sync_images_err_chk;
	}

#ifdef GFC_CAF_CHECK
  {
    for (i = 0; i < count; ++i)
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
          for (i = 0; i < caf_num_images - 1; ++i)
            ++orders[images_full[i] - 1];
          count = caf_num_images - 1;
          images = images_full;
        }
      else
        {
          for (i = 0; i < count; ++i)
            ++orders[images[i] - 1];
        }

#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
      explicit_flush();
#endif

      for(i = 0; i < count; ++i)
	/* Need to have the request handlers contigously in the handlers
	   array or waitany below will trip about the handler as illegal.  */
	ierr = MPI_Irecv (&arrived[images[i] - 1], 1, MPI_INT, images[i] - 1, 0,
	    CAF_COMM_WORLD, &handlers[i]);
      for(i = 0; i < count; ++i)
	{
# ifdef CAF_MPI_LOCK_UNLOCK
	  MPI_Win_lock (MPI_LOCK_SHARED, images[i] - 1, 0, *stat_tok);
# endif // CAF_MPI_LOCK_UNLOCK
	  ierr = MPI_Get (&remote_stat, 1, MPI_INT,
			  images[i] - 1, 0, 1, MPI_INT, *stat_tok);
# ifdef CAF_MPI_LOCK_UNLOCK
	  MPI_Win_unlock (images[i] - 1, *stat_tok);
# else // CAF_MPI_LOCK_UNLOCK
	  MPI_Win_flush (images[i] - 1, *stat_tok);
# endif // CAF_MPI_LOCK_UNLOCK
	  if(remote_stat != 0)
	    {
	      ierr = STAT_STOPPED_IMAGE;
	      /* Let the other images know, that at least one image is
		  stopped by sending STAT_STOPPED_IMAGE instead of our id.  */
	      for(i = 0; i < count; ++i)
		MPI_Send (&ierr, 1, MPI_INT, images[i] - 1, 0, CAF_COMM_WORLD);
	      break;
	    }
	}
      if (ierr == 0)
	{
	  int done_count = 0;
	  for(i = 0; i < count; ++i)
	    {
	      if (arrived[images[i] - 1] != STAT_STOPPED_IMAGE)
		/* Only send, when no stopped images have been found.  */
		ierr = MPI_Send (&caf_this_image, 1, MPI_INT, images[i] - 1, 0,
				 CAF_COMM_WORLD);
	      else
		ierr = STAT_STOPPED_IMAGE;
	    }

	  while (ierr != STAT_STOPPED_IMAGE && done_count < count)
	    {
	      ierr = MPI_Waitany (count, handlers, &i, &s);
	      if (i != MPI_UNDEFINED)
		++done_count;
	      if (i != MPI_UNDEFINED && arrived[i] == STAT_STOPPED_IMAGE)
		ierr = STAT_STOPPED_IMAGE;
	      else if (ierr != MPI_SUCCESS)
		break;
	    }
	}

      memset(arrived, 0, sizeof(int) * caf_num_images);
    }

sync_images_err_chk:
  if (stat)
    *stat = ierr;

  if (ierr && stat == NULL)
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

#define FOOFUNC(TYPE) foo_ ## TYPE

#define GEN_COREDUCE(name, dt)			\
static void \
name (void *invec, void *inoutvec, int *len, \
      MPI_Datatype *datatype)		     \
{ \
  int i;	     \
  for(i=0;i<*len;i++)				\
    {								\
      *((dt*)inoutvec) = (dt)(FOOFUNC(dt)((dt *)invec,(dt *)inoutvec));	\
     invec+=sizeof(dt); inoutvec+=sizeof(dt);	\
   } \
}

GEN_COREDUCE (redux_int32, int32_t)
GEN_COREDUCE (redux_real32, float)
GEN_COREDUCE (redux_real64, double)

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

    case GFC_DTYPE_LOGICAL_4:
      return MPI_INT;

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
/* gfortran passes character string arguments with a
   GFC_DTYPE_TYPE_SIZE == GFC_TYPE_CHARACTER + 64*strlen
*/
  if ( (GFC_DTYPE_TYPE_SIZE(desc)-GFC_DTYPE_CHARACTER)%64==0 )
      return MPI_CHARACTER;

  caf_runtime_error ("Unsupported data type in collective: %ld\n",GFC_DTYPE_TYPE_SIZE (desc));
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

  if (rank == 0 || PREFIX (is_contiguous) (source))
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
      if (datatype != MPI_CHARACTER)
        ierr = MPI_Bcast(a->base_addr, size, datatype, source_image-1, CAF_COMM_WORLD);
      else
      {
        int a_length;
        if (caf_this_image==source_image)
          a_length=strlen(a->base_addr);
        /* Broadcast the string lenth */
        ierr = MPI_Bcast(&a_length, 1, MPI_INT, source_image-1, CAF_COMM_WORLD);
        if (ierr)
          goto error;
        /* Broadcast the string itself */
        ierr = MPI_Bcast(a->base_addr, a_length, datatype, source_image-1, CAF_COMM_WORLD);
      }

      if (ierr)
        goto error;
      return;
    }
    else if (datatype == MPI_CHARACTER) /* rank !=0  */
    {
        caf_runtime_error ("Co_broadcast of character arrays not yet supported\n");
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
PREFIX (co_reduce) (gfc_descriptor_t *a, void *(*opr) (void *, void *), int opr_flags,
		    int result_image, int *stat, char *errmsg, int a_len, int errmsg_len)
{
  MPI_Op op;
  if(GFC_DESCRIPTOR_TYPE(a) == BT_INTEGER)
    {
      foo_int32_t = (typeof(foo_int32_t))opr;
      MPI_Op_create(redux_int32, 1, &op);
    }
  else if(GFC_DESCRIPTOR_TYPE(a) == BT_REAL)
    {
      if(GFC_DESCRIPTOR_SIZE(a) == sizeof(float))
  	{
  	  foo_float = (typeof(foo_float))opr;
  	  MPI_Op_create(redux_real32, 1, &op);
  	}
      else
  	{
  	  foo_double = (typeof(foo_double))opr;
  	  MPI_Op_create(redux_real64, 1, &op);
  	}
    }
  else if(GFC_DESCRIPTOR_TYPE(a) == BT_LOGICAL)
    {
      foo_int32_t = (typeof(foo_int32_t))opr;
      MPI_Op_create(redux_int32, 1, &op);
    }
  else
    {
      caf_runtime_error ("Data type not yet supported for co_reduce\n");
    }

  co_reduce_1 (op, a, result_image, stat, errmsg, 0, errmsg_len);
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
               int *acquired_lock, int *stat, char *errmsg,
               int errmsg_len)
{
  int dest_img;
  MPI_Win *p = TOKEN(token);

  if(image_index == 0)
    dest_img = caf_this_image;
  else
    dest_img = image_index;

  mutex_lock(*p, dest_img, index, stat, acquired_lock, errmsg, errmsg_len);
}


void
PREFIX (unlock) (caf_token_t token, size_t index, int image_index,
                 int *stat, char *errmsg, int errmsg_len)
{
  int dest_img;
  MPI_Win *p = TOKEN(token);

  if(image_index == 0)
    dest_img = caf_this_image;
  else
    dest_img = image_index;

  mutex_unlock(*p, dest_img, index, stat, errmsg, errmsg_len);
}

/* Atomics operations */

void
PREFIX (atomic_define) (caf_token_t token, size_t offset,
                        int image_index, void *value, int *stat,
                        int type __attribute__ ((unused)), int kind)
{
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Accumulate (value, 1, dt, image, offset, 1, dt, MPI_REPLACE, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Put (value, 1, dt, image, offset, 1, dt, *p);
  MPI_Win_unlock (image, *p);
#endif // MPI_VERSION

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
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Fetch_and_op(NULL, value, dt, image, offset, MPI_NO_OP, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
  ierr = MPI_Get (value, 1, dt, image, offset, 1, dt, *p);
  MPI_Win_unlock (image, *p);
#endif // MPI_VERSION

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
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0;
  int image;

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType (kind, &dt);

#if MPI_VERSION >= 3
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Compare_and_swap (new_val, compare, old, dt, image,
                               offset, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
#warning atomic_cas for MPI-2 is not yet implemented
  printf ("We apologize but atomic_cas for MPI-2 is not yet implemented\n");
  ierr = 1;
#endif // MPI_VERSION

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
  MPI_Datatype dt;
  MPI_Win *p = TOKEN(token);
  int image;

#if MPI_VERSION >= 3
  old = malloc(kind);

  if(image_index != 0)
    image = image_index-1;
  else
    image = caf_this_image-1;

  selectType (kind, &dt);

# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  /* Atomic_add */
      switch(op) {
          case 1:
              ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_SUM, *p);
              break;
          case 2:
              ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BAND, *p);
              break;
          case 4:
              ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BOR, *p);
              break;
          case 5:
              ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BXOR, *p);
              break;
          default:
              printf ("We apologize but the atomic operation requested for MPI is not yet implemented\n");
              break;
      }
# ifdef CAF_MPI_LOCK_UNLOCK
      MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
      MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK

  free(old);
#else // MPI_VERSION
 #warning atomic_op for MPI is not yet implemented
   printf ("We apologize but atomic_op for MPI is not yet implemented\n");
#endif // MPI_VERSION
  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    error_stop (ierr);

  return;
}

/* Events */

void
PREFIX (event_post) (caf_token_t token, size_t index,
		     int image_index, int *stat,
		     char *errmsg, int errmsg_len)
{
  int image, value=1, ierr=0;
  MPI_Win *p = TOKEN(token);
  const char msg[] = "Error on event post";

  if(image_index == 0)
    image = caf_this_image-1;
  else
    image = image_index-1;

  if(stat != NULL)
    *stat = 0;

  #if MPI_VERSION >= 3
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Accumulate (&value, 1, MPI_INT, image, index*sizeof(int), 1, MPI_INT, MPI_SUM, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
  #warning Events for MPI-2 are not implemented
  printf ("Events for MPI-2 are not supported, please update your MPI implementation\n");
#endif // MPI_VERSION
  if(ierr != MPI_SUCCESS)
    {
      if(stat != NULL)
	*stat = ierr;
      if(errmsg != NULL)
	{
	  memset(errmsg,' ',errmsg_len);
	  memcpy(errmsg, msg, MIN(errmsg_len,strlen(msg)));
	}
    }
}

void
PREFIX (event_wait) (caf_token_t token, size_t index,
		     int until_count, int *stat,
		     char *errmsg, int errmsg_len)
{
  int ierr=0,count=0,i,image=caf_this_image-1;
  int *var=NULL,flag,old=0;
  int newval=0;
  const int spin_loop_max = 20000;
  MPI_Win *p = TOKEN(token);
  const char msg[] = "Error on event wait";

  if(stat != NULL)
    *stat = 0;

  MPI_Win_get_attr(*p,MPI_WIN_BASE,&var,&flag);

  for(i = 0; i < spin_loop_max; ++i)
    {
      MPI_Win_sync(*p);
      count = var[index];
      if(count >= until_count)
	break;
    }

  i=1;
  while(count < until_count)
    /* for(i = 0; i < spin_loop_max; ++i) */
      {
	MPI_Win_sync(*p);
	count = var[index];
	/* if(count >= until_count) */
	/*   break; */
	usleep(5*i);
	i++;
      }

  newval = -until_count;

# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Fetch_and_op(&newval, &old, MPI_INT, image, index*sizeof(int), MPI_SUM, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  if(ierr != MPI_SUCCESS)
    {
      if(stat != NULL)
	*stat = ierr;
      if(errmsg != NULL)
	{
	  memset(errmsg,' ',errmsg_len);
	  memcpy(errmsg, msg, MIN(errmsg_len,strlen(msg)));
	}
    }
}

void
PREFIX (event_query) (caf_token_t token, size_t index,
		      int image_index, int *count, int *stat)
{
  int image,ierr=0;
  MPI_Win *p = TOKEN(token);

  if(image_index == 0)
    image = caf_this_image-1;
  else
    image = image_index-1;

  if(stat != NULL)
    *stat = 0;

#if MPI_VERSION >= 3
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_lock (MPI_LOCK_EXCLUSIVE, image, 0, *p);
# endif // CAF_MPI_LOCK_UNLOCK
  ierr = MPI_Fetch_and_op(NULL, count, MPI_INT, image, index*sizeof(int), MPI_NO_OP, *p);
# ifdef CAF_MPI_LOCK_UNLOCK
  MPI_Win_unlock (image, *p);
# else // CAF_MPI_LOCK_UNLOCK
  MPI_Win_flush (image, *p);
# endif // CAF_MPI_LOCK_UNLOCK
#else // MPI_VERSION
#warning Events for MPI-2 are not implemented
  printf ("Events for MPI-2 are not supported, please update your MPI implementation\n");
#endif // MPI_VERSION
  if(ierr != MPI_SUCCESS && stat != NULL)
    *stat = ierr;
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

/* STOP function for integer arguments.  */
void
PREFIX (stop_numeric) (int32_t stop_code)
{
  fprintf (stderr, "STOP %d\n", stop_code);
  PREFIX (finalize) ();
}

/* STOP function for string arguments.  */
void
PREFIX (stop_str) (const char *string, int32_t len)
{
  fputs ("STOP ", stderr);
  while (len--)
    fputc (*(string++), stderr);
  fputs ("\n", stderr);

  PREFIX (finalize) ();
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
