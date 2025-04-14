/* One-sided MPI implementation of Libcaf
 *
 * Copyright (c) 2012-2022, Sourcery Institute
 * Copyright (c) 2012-2022, Archaeolgoic Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sourcery, Inc., nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL SOURCERY, INC., BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <float.h>  /* For type conversion of floating point numbers. */
#include <stdarg.h> /* For variadic arguments. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* For memcpy. */
#ifndef ALLOCA_MISSING
#include <alloca.h> /* Assume functionality provided elsewhere if missing */
#endif
#include <mpi.h>
#define __USE_GNU
#include <pthread.h>
#include <signal.h> /* For raise */
#include <stdint.h> /* For int32_t. */
#include <unistd.h>

#ifdef HAVE_MPI_EXT_H
#include <mpi-ext.h>
#endif
#ifdef USE_FAILED_IMAGES
#define WITH_FAILED_IMAGES 1
#endif

#include "../../application-binary-interface/libcaf.h"

/* Define GFC_CAF_CHECK to enable run-time checking. */
/* #define GFC_CAF_CHECK  1 */

/* Debug array referencing  */
static char *caf_array_ref_str[]
    = {"CAF_ARR_REF_NONE",      "CAF_ARR_REF_VECTOR", "CAF_ARR_REF_FULL",
       "CAF_ARR_REF_RANGE",     "CAF_ARR_REF_SINGLE", "CAF_ARR_REF_OPEN_END",
       "CAF_ARR_REF_OPEN_START"};

static char *caf_ref_type_str[] = {
    "CAF_REF_COMPONENT",
    "CAF_REF_ARRAY",
    "CAF_REF_STATIC_ARRAY",
};

#ifndef EXTRA_DEBUG_OUTPUT
#define dprint(...)
#define chk_err(...)
#else
#define dprint(format, ...)                                                    \
  fprintf(stderr, "%d/%d (t:%d/%d): %s(%d) " format, global_this_image + 1,    \
          global_num_images, caf_this_image, caf_num_images, __FUNCTION__,     \
          __LINE__, ##__VA_ARGS__)
#define chk_err(ierr)                                                          \
  do                                                                           \
  {                                                                            \
    if (ierr != MPI_SUCCESS)                                                   \
    {                                                                          \
      int err_class, err_len;                                                  \
      char err_str[MPI_MAX_ERROR_STRING];                                      \
      MPI_Error_class(ierr, &err_class);                                       \
      MPI_Error_string(ierr, err_str, &err_len);                               \
      dprint("MPI-error: err_class=%d ierr=%d [%s]", err_class, ierr,          \
             err_str);                                                         \
    }                                                                          \
  } while (0)
#endif

#ifdef GCC_GE_7
/* The caf-token of the mpi-library.
 * Objects of this data structure are owned by the library and are treated as a
 * black box by the compiler.  In the coarray-program the tokens are opaque
 * pointers, i.e. black boxes.
 *
 * For each coarray (allocatable|save|pointer) (scalar|array|event|lock) a
 * token needs to be present. */
typedef struct mpi_caf_token_t
{
  /* The pointer to memory associated to this token's data on the local image.
   * The compiler uses the address for direct access to the memory of the object
   * this token is assocated to, i.e., the memory pointed to be local_memptr is
   * the scalar or array.
   * When the library is responsible for deleting the memory, then this is the
   * one to free. */
  void *memptr;
  /* The MPI window to associated to the object's data.
   * The window is used to access the data on other images. In pre GCC_GE_7
   * installations this was the token. */
  MPI_Win memptr_win;
  /* The pointer to the primary array, i.e., to coarrays that are arrays and
   * not a derived type. */
  gfc_descriptor_t *desc;
} mpi_caf_token_t;

/* For components of derived type coarrays a slave_token is needed when the
 * component has the allocatable or pointer attribute. The token is reduced in
 * size, because the other data is already accessible and has been read from
 * the remote to fullfill the request.
 *
 *   TYPE t
 *   +------------------+
 *   | comp *           |
 *   | comp_token *     |
 *   +------------------+
 *
 *   TYPE(t) : o                struct T // the mpi_caf_token to t
 *                              +----------------+
 *                              | ...            |
 *                              +----------------+
 *
 *   o[2]%.comp                 // using T to get o of [2]
 *
 *   +-o-on-image-2----+  "copy" of the required parts of o[2] on current image
 *   | 0x4711          |  comp * in global_dynamic_window
 *   | 0x2424          |  comp_token * of type slave_token
 *   +-----------------+
 *   now all required data is present on the current image to access the remote
 *   components. This nests without limit. */
typedef struct mpi_caf_slave_token_t
{
  /* The pointer to the memory associated to this slave token's data on the
   * local image.  When the library is responsible for deleting the memory,
   * then this is the one to free.  And this is the only reason why its stored
   * here. */
  void *memptr;
  /* The pointer to the descriptor or NULL for scalars.
   * When referencing a remote component array, then the extensions of the array
   * are needed. Usually the data pointer is at offset zero of the descriptor_t
   * structure, but we don't rely on it. So store the pointer to the base
   * address of the descriptor. The descriptor always is in the window of the
   * master data or the allocated component and is never stored at an address
   * not accessible by a window. */
  gfc_descriptor_t *desc;
} mpi_caf_slave_token_t;

#define TOKEN(X) &(((mpi_caf_token_t *)(X))->memptr_win)
#else
typedef MPI_Win *mpi_caf_token_t;
#define TOKEN(X) ((mpi_caf_token_t)(X))
#endif

/* Forward declaration of prototype. */

static void
terminate_internal(int stat_code, int exit_code) __attribute__((noreturn));
static void
sync_images_internal(int count, int images[], int *stat, char *errmsg,
                     size_t errmsg_len, bool internal);
static void
error_stop_str(const char *string, size_t len, bool quiet)
    __attribute__((noreturn));

/* Global variables. */
static int caf_this_image;
static int mpi_this_image;
static int caf_num_images = 0;
static int caf_is_finalized = 0;
static int global_this_image;
static int global_num_images;
static MPI_Win global_dynamic_win;

#if MPI_VERSION >= 3
MPI_Info mpi_info_same_size;
#endif // MPI_VERSION

/* The size of pointer on this plattform. */
static const size_t stdptr_size = sizeof(void *);

/* Variables needed for syncing images. */

static int *images_full;
MPI_Request *sync_handles;
static int *arrived;
static const int MPI_TAG_CAF_SYNC_IMAGES = 424242;

/* Pending puts */
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
typedef struct win_sync
{
  MPI_Win *win;
  int img;
  struct win_sync *next;
} win_sync;

static win_sync *last_elem = NULL;
static win_sync *pending_puts = NULL;
#endif

#ifndef GCC_GE_16
/* Linked list of static coarrays registered.  Do not expose to public in the
 * header, because it is implementation specific.
 *
 * From gcc-15 on, this list is contained in the teams handling. */
struct caf_allocated_tokens_t
{
  caf_token_t token;
  struct caf_allocated_tokens_t *prev;
} *caf_allocated_tokens = NULL;
#endif

#ifdef GCC_GE_7
/* Linked list of slave coarrays registered. */
struct caf_allocated_slave_tokens_t
{
  mpi_caf_slave_token_t *token;
  struct caf_allocated_slave_tokens_t *prev;
} *caf_allocated_slave_tokens = NULL;
#endif

/* Image status variable */
static int img_status = 0;
static MPI_Win stat_tok;

/* Active messages variables */
char **buff_am;
MPI_Status *s_am;
MPI_Request *req_am;
MPI_Datatype *dts;
char *msgbody;
pthread_mutex_t lock_am;
int done_am = 0;

#ifdef GCC_GE_15
/* Communication thread variables, constants and structures. */
static const int CAF_CT_TAG = 13;
pthread_t commthread;
MPI_Comm ct_COMM;
bool commthread_running = true;
enum CT_MSG_FLAGS
{
  CT_DST_HAS_DESC = 1,
  CT_SRC_HAS_DESC = 1 << 1,
  CT_CHAR_ARRAY = 1 << 2,
  CT_INCLUDE_DESCRIPTOR = 1 << 3,
  CT_TRANSFER_DESC = 1 << 4,
  /* Use 1 << 5 for next flag. */
};

typedef void (*getter_t)(void *, const int *, void **, int32_t *, void *,
                         caf_token_t, const size_t, size_t *, const size_t *);
typedef void (*is_present_t)(void *, const int *, int32_t *, void *,
                             caf_token_t, const size_t);
typedef void (*receiver_t)(void *, const int *, void *, const void *,
                           caf_token_t, const size_t, const size_t *,
                           const size_t *);

struct accessor_hash_t
{
  int hash;
  int pad;
  union {
    getter_t getter;
    is_present_t is_present;
    receiver_t receiver;
  } u;
};

static struct accessor_hash_t *accessor_hash_table = NULL;
static int aht_cap = 0;
static int aht_size = 0;
static enum
{
  AHT_UNINITIALIZED,
  AHT_OPEN,
  AHT_PREPARED
} accessor_hash_table_state = AHT_UNINITIALIZED;

typedef ptrdiff_t rat_id_t;
static struct running_accesses_t
{
  rat_id_t id;
  void *memptr;
  struct running_accesses_t *next;
} *running_accesses = NULL;

static rat_id_t running_accesses_id_cnt = 0;

enum remote_command
{
  remote_command_unset = 0,
  remote_command_get = 1,
  remote_command_present,
  remote_command_send,
  remote_command_transfer,
};

/* The structure to communicate with the communication thread. Make sure, that
 * data[] starts on pointer aligned address to not loss any performance. */
typedef struct
{
  int cmd;
  int flags;
  size_t transfer_size;
  size_t opt_charlen;
  MPI_Win win;
  int dest_image;
  int dest_tag;
  int accessor_index;
  rat_id_t ra_id;
  size_t dest_opt_charlen;
  char data[];
} ct_msg_t;

struct transfer_msg_data_t
{
  size_t dst_msg_size;
  size_t dst_desc_size;
  size_t dst_add_data_size;
  char data[];
};
#endif

/* Define the descriptor of max rank.
 *
 *  This typedef is made to allow storing a copy of a remote descriptor on the
 *  stack without having to care about the rank. */
typedef struct gfc_max_dim_descriptor_t
{
  gfc_descriptor_t base;
  descriptor_dimension dim[GFC_MAX_DIMENSIONS];
} gfc_max_dim_descriptor_t;

char err_buffer[MPI_MAX_ERROR_STRING];

#ifdef GCC_GE_16
/* All CAF runtime calls should use this comm instead of MPI_COMM_WORLD for
 * interoperability purposes. */
MPI_Comm *CAF_COMM_WORLD_store;
#define CAF_COMM_WORLD (*CAF_COMM_WORLD_store)

typedef struct caf_teams_list
{
  /* The communicator having all team members.  */
  MPI_Comm communicator;
  /* The unique id (over all other formed teams) of this team. */
  int team_id;
  /* The id of this image in this team. */
  int image_id;
  struct caf_teams_list *prev;
} caf_teams_list_t;

typedef struct caf_team_stack_node
{
  struct caf_teams_list *team_list_elem;
  /* The list of tokens allocated in the current team. Needed to free them
   * when the team is left. */
  struct allocated_tokens_t
  {
    mpi_caf_token_t *token;
    struct allocated_tokens_t *next;
  } *allocated_tokens;
  struct caf_team_stack_node *parent;
} caf_team_stack_node_t;

static caf_teams_list_t *teams_list = NULL;
static caf_team_stack_node_t *current_team = NULL, *initial_team;
#else
/* All CAF runtime calls should use this comm instead of MPI_COMM_WORLD for
 * interoperability purposes. */
MPI_Comm CAF_COMM_WORLD;

typedef struct caf_teams_list
{
  caf_team_t team;
  int team_id;
  struct caf_teams_list *prev;
} caf_teams_list;

typedef struct caf_used_teams_list
{
  struct caf_teams_list *team_list_elem;
  struct caf_used_teams_list *prev;
} caf_used_teams_list;

static caf_teams_list *teams_list = NULL;
static caf_used_teams_list *used_teams = NULL;
#endif

/* Emitted when a theorectically unreachable part is reached. */
const char unreachable[] = "Fatal error: unreachable alternative found.\n";

#ifdef WITH_FAILED_IMAGES
/* The stati of the other images.  image_stati is an array of size
 * caf_num_images at the beginning the status of each image is noted here where
 * the index is the image number minus one. */
int *image_stati;

/* This gives the number of all images that are known to have failed. */
int num_images_failed = 0;

/* This is the number of all images that are known to have stopped. */
int num_images_stopped = 0;

/* The async. request-handle to all participating images. */
MPI_Request alive_request;

/* This dummy is used for the alive request.  Its content is arbitrary and
 * never read.  Its just a memory location where one could put something,
 * which is never done. */
int alive_dummy;

/* The mpi error-handler object associate to CAF_COMM_WORLD. */
MPI_Errhandler failed_CAF_COMM_mpi_err_handler;

/* The monitor comm for detecting failed images. We can not attach the monitor
 * to CAF_COMM_WORLD or the messages send by sync images would be caught by the
 * monitor. */
MPI_Comm alive_comm;

/* Set when entering a sync_images_internal, to prevent the error handler from
 * eating our messages. */
bool no_stopped_images_check_in_errhandler = 0;
#endif

/* For MPI interoperability, allow external initialization
 * (and thus finalization) of MPI. */
bool caf_owns_mpi = false;

/* Foo function pointers for coreduce.
 * The handles when arguments are passed by reference. */
int (*int8_t_by_reference)(void *, void *);
int (*int16_t_by_reference)(void *, void *);
int (*int32_t_by_reference)(void *, void *);
int (*int64_t_by_reference)(void *, void *);
float (*float_by_reference)(void *, void *);
double (*double_by_reference)(void *, void *);
/* Strings are always passed by reference. */
void (*char_by_reference)(void *, int, void *, void *, int, int);
/* The handles when arguments are passed by value. */
int8_t (*int8_t_by_value)(int8_t, int8_t);
int16_t (*int16_t_by_value)(int16_t, int16_t);
int (*int32_t_by_value)(int32_t, int32_t);
int64_t (*int64_t_by_value)(int64_t, int64_t);
float (*float_by_value)(float, float);
double (*double_by_value)(double, double);

/* Define shortcuts for Win_lock and _unlock depending on whether the primitives
 * are available in the MPI implementation.  When they are not available the
 * shortcut is expanded to nothing by the preprocessor else to the API call.
 * This prevents having #ifdef #else #endif constructs strewn all over the code
 * reducing its readability. */
// #ifdef CAF_MPI_LOCK_UNLOCK
#define CAF_Win_lock(type, img, win) MPI_Win_lock(type, img, 0, win)
#define CAF_Win_unlock(img, win) MPI_Win_unlock(img, win)
#define CAF_Win_lock_all(win)
#define CAF_Win_unlock_all(win)
// #else // CAF_MPI_LOCK_UNLOCK
// #define CAF_Win_lock(type, img, win)
// #define CAF_Win_unlock(img, win) MPI_Win_flush (img, win)
// #if MPI_VERSION >= 3
// #define CAF_Win_lock_all(win) MPI_Win_lock_all (MPI_MODE_NOCHECK, win)
// #else
// #define CAF_Win_lock_all(win)
// #endif
// #define CAF_Win_unlock_all(win) MPI_Win_unlock_all (win)
// #endif // CAF_MPI_LOCK_UNLOCK

/* Convenience macro to get the extent of a descriptor in a certain dimension
 *
 * Copied from gcc:libgfortran/libgfortran.h. */
#define GFC_DESCRIPTOR_EXTENT(desc, i)                                         \
  ((desc)->dim[i]._ubound + 1 - (desc)->dim[i].lower_bound)

#define sizeof_desc_for_rank(rank)                                             \
  (sizeof(gfc_descriptor_t) + (rank) * sizeof(descriptor_dimension))

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
void
explicit_flush()
{
  win_sync *w = pending_puts, *t;
  MPI_Win *p;
  int ierr;
  while (w != NULL)
  {
    p = w->win;
    ierr = MPI_Win_flush(w->img, *p);
    chk_err(ierr);
    t = w;
    w = w->next;
    free(t);
  }
  last_elem = NULL;
  pending_puts = NULL;
}
#endif

#ifdef HELPER
void
helperFunction()
{
  int i = 0, flag = 0, msgid = 0, ierr;
  int ndim = 0, position = 0;

  s_am = calloc(caf_num_images, sizeof(MPI_Status));
  req_am = calloc(caf_num_images, sizeof(MPI_Request));
  dts = calloc(caf_num_images, sizeof(MPI_Datatype));

  for (i = 0; i < caf_num_images; i++)
  {
    ierr = MPI_Irecv(buff_am[i], 1000, MPI_PACKED, i, 1, CAF_COMM_WORLD,
                     &req_am[i]);
    chk_err(ierr);
  }

  while (1)
  {
    pthread_mutex_lock(&lock_am);
    for (i = 0; i < caf_num_images; i++)
    {
      if (!caf_is_finalized)
      {
        ierr = MPI_Test(&req_am[i], &flag, &s_am[i]);
        chk_err(ierr);
        if (flag == 1)
        {
          position = 0;
          ierr = MPI_Unpack(buff_am[i], 1000, &position, &msgid, 1, MPI_INT,
                            CAF_COMM_WORLD);
          chk_err(ierr);
          /* msgid=2 was initially assigned to strided transfers,
           * it can be reused
           * Strided transfers Msgid=2
           * You can add you own function */

          if (msgid == 2)
          {
            msgid = 0;
            position = 0;
          }
          ierr = MPI_Irecv(buff_am[i], 1000, MPI_PACKED, i, 1, CAF_COMM_WORLD,
                           &req_am[i]);
          chk_err(ierr);
          flag = 0;
        }
      }
      else
      {
        done_am = 1;
        pthread_mutex_unlock(&lock_am);
        return;
      }
    }
    pthread_mutex_unlock(&lock_am);
  }
}
#endif

/* Keep in sync with single.c. */

static void
caf_runtime_error(const char *message, ...)
{
  va_list ap;
  fprintf(stderr, "OpenCoarrays internal error on image %d: ", caf_this_image);
  va_start(ap, message);
  vfprintf(stderr, message, ap);
  va_end(ap);
  fprintf(stderr, "\n");

  /* FIXME: Shutdown the Fortran RTL to flush the buffer.  PR 43849.
   * FIXME: Do some more effort than just to abort. */
  //  MPI_Finalize();

  /* Should be unreachable, but to make sure also call exit. */
  exit(EXIT_FAILURE);
}

/* Error handling is similar everytime. Keep in sync with single.c, too. */
static void
caf_internal_error(const char *msg, int *stat, char *errmsg, size_t errmsg_len,
                   ...)
{
  va_list args;
  va_start(args, errmsg_len);
  if (stat)
  {
    *stat = 1;
    if (errmsg_len > 0)
    {
      int len = snprintf(errmsg, errmsg_len, msg, args);
      if (len >= 0 && errmsg_len > (size_t)len)
        memset(&errmsg[len], ' ', errmsg_len - len);
    }
    va_end(args);
    return;
  }
  else
  {
    fprintf(stderr, "Fortran runtime error on image %d: ", caf_this_image);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
  }
  va_end(args);
  exit(EXIT_FAILURE);
}

#ifdef EXTRA_DEBUG_OUTPUT
void
dump_mem(const char *pre, void *m, const size_t s)
{
  const size_t str_len = s && m ? s * 3 + 1 : 8;
  char *str = (char *)alloca(str_len), *p, *pend = str + str_len;

  if (m && s)
  {
    p = str;
    for (size_t i = 0; i < s && p < pend; ++i, p += 3)
      sprintf(p, "%02x ", ((unsigned char *)m)[i]);
    if (p >= pend)
      dprint("dump_mem: output buffer exhausted.\n");
  }
  else
    memcpy(str, "*EMPTY*", 8);
  dprint("%s: %p: (len = %zd) %s\n", pre, m, s, str);
}
#else
#define dump_mem(pre, m, s) ;
#endif

size_t
compute_arr_data_size_sz(const gfc_descriptor_t *desc, size_t sz)
{
  for (int i = 0; i < GFC_DESCRIPTOR_RANK(desc); ++i)
    sz *= GFC_DESCRIPTOR_EXTENT(desc, i);

  return sz;
}

size_t
compute_arr_data_size(const gfc_descriptor_t *desc)
{
  return compute_arr_data_size_sz(desc, desc->span);
}

#ifdef GCC_GE_15
size_t
handle_getting(ct_msg_t *msg, int cb_image, void *baseptr, void *dst_ptr,
               void **buffer, int32_t *free_buffer, void *dbase)
{
  void *src_ptr;
  size_t charlen, send_size;
  int i;
  mpi_caf_token_t src_token = {(void *)msg->ra_id, MPI_WIN_NULL, NULL};

  if (msg->flags & CT_SRC_HAS_DESC)
  {
    ((gfc_descriptor_t *)dbase)->base_addr = baseptr;
    src_ptr = dbase;
    dbase += sizeof_desc_for_rank(
        GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr));
    dprint("ct: src_desc base: %p, rank: %d, offset: %zd.\n",
           ((gfc_descriptor_t *)src_ptr)->base_addr,
           GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr),
           ((gfc_descriptor_t *)src_ptr)->offset);
    // for (int i = 0; i < GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr);
    // ++i)
    //   dprint("ct: src_desc (dim: %d) lb: %d, ub: %d, stride: %d\n", i,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i].lower_bound,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i]._ubound,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i]._stride);
  }
  else
    src_ptr = baseptr;

  charlen = msg->dest_opt_charlen;
  accessor_hash_table[msg->accessor_index].u.getter(
      dbase, &cb_image, dst_ptr, free_buffer, src_ptr, &src_token, 0, &charlen,
      &msg->opt_charlen);
  dprint("ct: getter executed.\n");
  if (msg->flags & CT_DST_HAS_DESC)
  {
    size_t dsize = ((gfc_descriptor_t *)dst_ptr)->span;
    dprint("ct: dst_desc base: %p, rank: %d, offset: %zd.\n",
           ((gfc_descriptor_t *)dst_ptr)->base_addr,
           GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr),
           ((gfc_descriptor_t *)dst_ptr)->offset);
    for (int i = 0; i < GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr); ++i)
    {
      dprint("ct: dst_desc (dim: %d) lb: %td, ub: %td, stride: %td, extend: "
             "%td\n",
             i, ((gfc_descriptor_t *)dst_ptr)->dim[i].lower_bound,
             ((gfc_descriptor_t *)dst_ptr)->dim[i]._ubound,
             ((gfc_descriptor_t *)dst_ptr)->dim[i]._stride,
             GFC_DESCRIPTOR_EXTENT((gfc_descriptor_t *)dst_ptr, i));
      dsize *= GFC_DESCRIPTOR_EXTENT((gfc_descriptor_t *)dst_ptr, i);
    }
    dump_mem("ct", ((gfc_descriptor_t *)dst_ptr)->base_addr, dsize);
    *buffer = ((gfc_descriptor_t *)dst_ptr)->base_addr;
    if ((msg->flags & (CT_CHAR_ARRAY | CT_INCLUDE_DESCRIPTOR)) == 0)
      send_size = msg->transfer_size;
    else
    {
      if (msg->flags & CT_INCLUDE_DESCRIPTOR)
        send_size = ((gfc_descriptor_t *)dst_ptr)->span;
      else
        send_size = charlen * msg->transfer_size;
      for (i = 0; i < GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr); ++i)
      {
        const ptrdiff_t ext
            = GFC_DESCRIPTOR_EXTENT((gfc_descriptor_t *)dst_ptr, i);
        if (ext < 0)
          dprint("ct: dst extend in dim %d is < 0: %ld.\n", i, ext);
        send_size *= ext;
      }
    }
    if (msg->flags & CT_INCLUDE_DESCRIPTOR)
    {
      const size_t desc_size = sizeof_desc_for_rank(
          GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr));
      void *tbuff = malloc(desc_size + send_size);
      dprint("ct: Including dst descriptor: %p, sizeof(desc): %zd, rank: "
             "%d, sizeof(buffer): %zd, incoming free_buffer: %b.\n",
             tbuff, desc_size, GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr),
             send_size, *free_buffer);
      /* Copy the descriptor contents. */
      memcpy(tbuff, dst_ptr, desc_size);
      /* Copy the data to the end of buffer (i.e. behind the descriptor).
       * Does not copy anything, when send_size is 0.  */
      memcpy(tbuff + desc_size, *buffer, send_size);
      if (*free_buffer)
      {
        dprint("ct: Freeing buffer: %p.\n", *buffer);
        free(*buffer);
      }
      /* For debugging only: */
      ((gfc_descriptor_t *)tbuff)->base_addr = tbuff + desc_size;
      *free_buffer = true;
      *buffer = tbuff;
      send_size += desc_size;
    }
  }
  else
  {
    *buffer = *(void **)dst_ptr;
    dprint("ct: dst_ptr: %p, buffer: %p.\n", dst_ptr, *buffer);
    send_size = charlen * msg->transfer_size;
    dprint("ct: buffer %p, send_size: %zd.\n", *buffer, send_size);
  }
  return send_size;
}

void
handle_get_message(ct_msg_t *msg, void *baseptr)
{
  int ierr = 0;
  void *buffer, *dst_ptr, *get_data;
  size_t send_size;
  int32_t free_buffer;

  if (msg->flags & CT_DST_HAS_DESC)
  {
    buffer = msg->data;
    ((gfc_descriptor_t *)buffer)->base_addr = NULL;
    get_data = msg->data
               + sizeof_desc_for_rank(
                   GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)buffer));
    /* The destination is a descriptor which address is not mutable. */
    dst_ptr = buffer;
  }
  else
  {
    get_data = msg->data;
    /* The destination is raw memory block, which adress is mutable. */
    buffer = NULL;
    dst_ptr = &buffer;
    dprint("ct: dst_ptr: %p, buffer: %p.\n", dst_ptr, buffer);
  }

  send_size = handle_getting(msg, msg->dest_image, baseptr, dst_ptr, &buffer,
                             &free_buffer, get_data);

  dump_mem("ct", buffer, send_size);
  dprint("ct: Sending %zd bytes to image %d, tag %d.\n", send_size,
         msg->dest_image, msg->dest_tag);
  ierr = MPI_Send(buffer, send_size, MPI_BYTE, msg->dest_image, msg->dest_tag,
                  CAF_COMM_WORLD);
  chk_err(ierr);
  if (free_buffer)
  {
    dprint("ct: going to free buffer: %p.\n", buffer);
    free(buffer);
  }
}

void
handle_is_present_message(ct_msg_t *msg, void *baseptr)
{
  int ierr = 0;
  void *add_data, *ptr;
  int32_t result;
  mpi_caf_token_t src_token = {(void *)msg->ra_id, MPI_WIN_NULL, NULL};

  add_data = msg->data;
  if (msg->flags & CT_SRC_HAS_DESC)
  {
    ((gfc_descriptor_t *)add_data)->base_addr = baseptr;
    ptr = add_data;
    add_data
        += sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)ptr));
  }
  else
    ptr = &baseptr;

  accessor_hash_table[msg->accessor_index].u.is_present(
      add_data, &msg->dest_image, &result, ptr, &src_token, 0);
  dprint("ct: is_present executed.\n");
  dprint("ct: Sending %d bytes to image %d, tag %d.\n", 1, msg->dest_image,
         msg->dest_tag);
  ierr = MPI_Send(&result, 1, MPI_BYTE, msg->dest_image, msg->dest_tag,
                  CAF_COMM_WORLD);
  chk_err(ierr);
}

void
handle_send_message(ct_msg_t *msg, void *baseptr)
{
  int ierr = 0;
  void *src_ptr, *buffer, *dst_ptr, *add_data;
  mpi_caf_token_t src_token = {(void *)msg->ra_id, MPI_WIN_NULL, NULL};

  dprint("ct: putting data using %d accessor.\n", msg->accessor_index);
  buffer = msg->data;
  add_data = msg->data + msg->transfer_size;
  if (msg->flags & CT_SRC_HAS_DESC)
  {
    src_ptr = add_data;
    ((gfc_descriptor_t *)add_data)->base_addr = buffer;
    add_data += sizeof_desc_for_rank(
        GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr));
    dprint("ct: src_desc base: %p, rank: %d, offset: %td.\n",
           ((gfc_descriptor_t *)src_ptr)->base_addr,
           GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr),
           ((gfc_descriptor_t *)src_ptr)->offset);
    // for (int i = 0; i < GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)src_ptr);
    // ++i)
    //   dprint("ct: src_desc (dim: %d) lb: %td, ub: %td, stride: %td\n", i,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i].lower_bound,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i]._ubound,
    //          ((gfc_descriptor_t *)src_ptr)->dim[i]._stride);
    /* The destination is a descriptor which address is not mutable. */
  }
  else
  {
    /* The destination is raw memory block, which adress is mutable. */
    src_ptr = buffer;
    dprint("ct: src_ptr: %p, buffer: %p.\n", src_ptr, buffer);
  }
  if (msg->flags & CT_DST_HAS_DESC)
  {
    ((gfc_descriptor_t *)add_data)->base_addr = baseptr;
    dst_ptr = add_data;
    add_data += sizeof_desc_for_rank(
        GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr));
    dprint("ct: dst_desc base: %p, rank: %d, offset: %zd.\n",
           ((gfc_descriptor_t *)dst_ptr)->base_addr,
           GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr),
           ((gfc_descriptor_t *)dst_ptr)->offset);
    // for (int i = 0; i < GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)dst_ptr);
    // ++i)
    //   dprint("ct: dst_desc (dim: %d) lb: %td, ub: %td, stride: %td\n", i,
    //          ((gfc_descriptor_t *)dst_ptr)->dim[i].lower_bound,
    //          ((gfc_descriptor_t *)dst_ptr)->dim[i]._ubound,
    //          ((gfc_descriptor_t *)dst_ptr)->dim[i]._stride);
    // dump_mem("send dst", ((gfc_descriptor_t *)dst_ptr)->base_addr,
    //          (((gfc_descriptor_t *)dst_ptr)->dim[0]._ubound + 1
    //           - ((gfc_descriptor_t *)dst_ptr)->dim[0].lower_bound
    //           + ((gfc_descriptor_t *)dst_ptr)->offset)
    //              * GFC_DESCRIPTOR_SIZE((gfc_descriptor_t *)dst_ptr));
  }
  else
  {
    dst_ptr = baseptr;
    dprint("ct: scalar dst_ptr: %p.\n", dst_ptr);
  }

  accessor_hash_table[msg->accessor_index].u.receiver(
      add_data, &msg->dest_image, dst_ptr, src_ptr, &src_token, 0,
      &msg->dest_opt_charlen, &msg->opt_charlen);
  dprint("ct: setter executed.\n");
  {
    char c = 0;
    dprint("ct: Sending %d bytes to image %d, tag %d.\n", 1,
           msg->dest_image + 1, msg->dest_tag);
    ierr = MPI_Send(&c, 1, MPI_BYTE, msg->dest_image, msg->dest_tag,
                    CAF_COMM_WORLD);
    chk_err(ierr);
  }
}

void
handle_transfer_message(ct_msg_t *msg, void *baseptr)
{
  int ierr;
  int32_t free_buffer;
  gfc_max_dim_descriptor_t transfer_desc;
  void *transfer_ptr, *buffer = NULL;
  size_t send_size, src_size, offset;
  bool free_send_msg;
  ct_msg_t *incoming_send_msg = (ct_msg_t *)msg->data, *send_msg;
  struct transfer_msg_data_t *tmd
      = (struct transfer_msg_data_t *)(incoming_send_msg)->data;
  void *get_msg_data_base = msg->data + tmd->dst_msg_size;

  if (msg->flags & CT_TRANSFER_DESC)
  {
    memset(&transfer_desc, 0, sizeof(transfer_desc));
    transfer_ptr = &transfer_desc;
    msg->flags |= CT_DST_HAS_DESC | CT_INCLUDE_DESCRIPTOR;
    incoming_send_msg->flags |= CT_SRC_HAS_DESC;
  }
  else
  {
    msg->flags &= ~(CT_DST_HAS_DESC | CT_INCLUDE_DESCRIPTOR);
    transfer_ptr = &buffer;
  }

  src_size
      = handle_getting(msg, incoming_send_msg->dest_image, baseptr,
                       transfer_ptr, &buffer, &free_buffer, get_msg_data_base);

  send_size = sizeof(ct_msg_t) + src_size + tmd->dst_desc_size
              + tmd->dst_add_data_size;

  dprint("ct: src_size: %zd, send_size: %zd, dst_desc_size: %zd, "
         "dst_add_data_size: %zd, buffer: %p.\n",
         src_size, send_size, tmd->dst_desc_size, tmd->dst_add_data_size,
         buffer);

  if ((free_send_msg = ((send_msg = alloca(send_size)) == NULL)))
  {
    send_msg = malloc(send_size);
    if (send_msg == NULL)
      caf_runtime_error("Unable to allocate memory "
                        "for internal message in handle_transfer_message().");
  }
  memcpy(send_msg, incoming_send_msg, sizeof(ct_msg_t));
  offset = 0;
  if (msg->flags & CT_TRANSFER_DESC)
  {
    const gfc_descriptor_t *d = (gfc_descriptor_t *)buffer;
    const int rank = GFC_DESCRIPTOR_RANK(d);
    const size_t desc_size = sizeof_desc_for_rank(rank),
                 sz = compute_arr_data_size(d);
    /* Add the data first.  */
    send_msg->transfer_size = sz;
    memcpy(send_msg->data, ((gfc_descriptor_t *)buffer)->base_addr, sz);
    offset += sz;
    memcpy(send_msg->data + offset, buffer, desc_size);
    offset += desc_size;
  }
  else
  {
    memcpy(send_msg->data, buffer, src_size);
    offset += src_size;
  }
  memcpy(send_msg->data + offset, tmd->data,
         tmd->dst_desc_size + tmd->dst_add_data_size);

  if (msg->dest_image != global_this_image)
  {
    dprint("ct: sending message of size %zd to image %d for processing.\n",
           send_size, msg->dest_image);
    ierr = MPI_Send(send_msg, send_size, MPI_BYTE, msg->dest_image,
                    msg->dest_tag, ct_COMM);
    chk_err(ierr);
  }
  else
  {
    int flag;
    dprint("ct: self handling message of size %zd.\n", send_size);
    ierr = MPI_Win_get_attr(send_msg->win, MPI_WIN_BASE, &baseptr, &flag);
    chk_err(ierr);
    handle_send_message(send_msg, baseptr);
  }

  if (free_send_msg)
    free(send_msg);
  if (free_buffer)
  {
    dprint("ct: going to free buffer: %p.\n", buffer);
    free(buffer);
  }
}

void
handle_incoming_message(MPI_Status *status_in, MPI_Message *msg_han,
                        const int cnt)
{
  int ierr = 0;
  void *baseptr;
  int flag;
  ct_msg_t *msg = alloca(cnt);

  ierr = MPI_Mrecv(msg, cnt, MPI_BYTE, msg_han, status_in);
  chk_err(ierr);
  dprint("ct: Received request of size %d (sizeof(ct_msg) = %zd).\n", cnt,
         sizeof(ct_msg_t));

  if (msg->win != MPI_WIN_NULL)
  {
    ierr = MPI_Win_get_attr(msg->win, MPI_WIN_BASE, &baseptr, &flag);
    chk_err(ierr);
  }
  else
  {
    struct running_accesses_t *ra = running_accesses;
    for (; ra && ra->id != msg->ra_id; ra = ra->next)
      ;
    baseptr = ra->memptr;
  }

  dprint("ct: Local base for win %d is %p (set: %b) Executing accessor at "
         "index %d address %p for command %i.\n",
         msg->win, baseptr, flag, msg->accessor_index,
         accessor_hash_table[msg->accessor_index].u.getter, msg->cmd);
  if (!flag)
  {
    dprint("ct: Error: Window %d memory is not allocated.\n", msg->win);
  }

  switch (msg->cmd)
  {
    case remote_command_get:
      handle_get_message(msg, baseptr);
      break;
    case remote_command_present:
      handle_is_present_message(msg, baseptr);
      break;
    case remote_command_send:
      handle_send_message(msg, baseptr);
      break;
    case remote_command_transfer:
      handle_transfer_message(msg, baseptr);
      break;
    default:
      caf_runtime_error("unknown command %d in message for remote execution",
                        msg->cmd);
      break;
  }
}

void *
communication_thread(void *)
{
  int ierr = 0, cnt;
  MPI_Status status;
  MPI_Message msg_han;
  void *baseptr;

#if defined(__have_pthread_attr_t) && defined(EXTRA_DEBUG_OUTPUT)
  pthread_t self;
  pthread_attr_t pattr;
  size_t stacksize;
  self = pthread_self();
  pthread_getattr_np(self, &pattr);
  pthread_attr_getstacksize(&pattr, &stacksize);
  dprint("ct: Started witch stacksize: %ld.\n", stacksize);
#endif

  memset(&status, 0, sizeof(MPI_Status));
  do
  {
    dprint("ct: Probing for incoming message.\n");
    ierr = MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, ct_COMM, &msg_han, &status);
    chk_err(ierr);
    dprint("ct: Message received from %d, tag %d, mpi-status: %d, processing "
           "...\n",
           status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR);
    if (status.MPI_TAG == CAF_CT_TAG && status.MPI_ERROR == MPI_SUCCESS)
    {
      ierr = MPI_Get_count(&status, MPI_BYTE, &cnt);
      chk_err(ierr);

      if (cnt >= sizeof(ct_msg_t))
      {
        handle_incoming_message(&status, &msg_han, cnt);
      }
      else if (!commthread_running)
      {
        /* Pickup empty message. */
        dprint("ct: Got termination message. Terminating.\n");
        baseptr = NULL;
        ierr = MPI_Mrecv(baseptr, cnt, MPI_BYTE, &msg_han, &status);
        chk_err(ierr);
      }
      else
      {
        dprint("ct: Error: message to small, ignoring (got: %d, exp: %zd).\n",
               cnt, sizeof(ct_msg_t));
      }
    }
    else if (ierr == MPI_SUCCESS)
    {
      /* There is a message, but not for us. */
      dprint("ct: Message not for us received. Setting it free again.\n");
      // ierr = MPI_Request_free(&msg_han);
      chk_err(ierr);
    }
    else
      chk_err(ierr);
  } while (commthread_running);
  dprint("ct: Ended.\n");
  return NULL;
}
#endif

/* Forward declaration of the feature unsupported message for failed images
 * functions. */
static void
unsupported_fail_images_message(const char *functionname);

static void
locking_atomic_op(MPI_Win win, int *value, int newval, int compare,
                  int image_index, size_t index)
{
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index - 1, win);
  int ierr = MPI_Compare_and_swap(&newval, &compare, value, MPI_INT,
                                  image_index - 1, index * sizeof(int), win);
  chk_err(ierr);
  CAF_Win_unlock(image_index - 1, win);
}

/* Define a helper to check whether the image at the given index is healthy,
 * i.e., it hasn't failed. */
#ifdef WITH_FAILED_IMAGES
#define check_image_health(image_index, stat)                                  \
  if (image_stati[image_index - 1] == STAT_FAILED_IMAGE)                       \
  {                                                                            \
    if (stat == NULL)                                                          \
      terminate_internal(STAT_FAILED_IMAGE, 0);                                \
    *stat = STAT_FAILED_IMAGE;                                                 \
    return;                                                                    \
  }
#else
#define check_image_health(image_index, stat)
#endif

#ifdef WITH_FAILED_IMAGES
/* Handle failed image's errors and try to recover the remaining process to
 * allow the user to detect an image fail and exit gracefully. */
static void
failed_stopped_errorhandler_function(MPI_Comm *pcomm, int *perr, ...)
{
  MPI_Comm comm, shrunk, newcomm;
  int num_failed_in_group, i, err, ierr;
  MPI_Group comm_world_group, failed_group;
  int *ranks_of_failed_in_comm_world, *ranks_failed;
  int ns, srank, crank, rc, flag, drank, newrank;
  bool stopped = false;

  comm = *pcomm;

  MPI_Error_class(*perr, &err);
  if (err != MPIX_ERR_PROC_FAILED && err != MPIX_ERR_REVOKED)
  {
    /* We can handle PROC_FAILED and REVOKED ones only. */
    char errstr[MPI_MAX_ERROR_STRING];
    int errlen;
    MPI_Error_string(err, errstr, &errlen);
    /* We can't use caf_runtime_error here, because that would exit, which
     * means only the one process will stop, but we need to stop MPI
     * completely, which can be done by calling MPI_Abort(). */
    fprintf(stderr, "Fortran runtime error on image #%d:\nMPI error: '%s'.\n",
            caf_this_image, errstr);
    MPI_Abort(*pcomm, err);
  }

  dprint("(error = %d)\n", err);

  ierr = MPIX_Comm_failure_ack(comm);
  chk_err(ierr);
  ierr = MPIX_Comm_failure_get_acked(comm, &failed_group);
  chk_err(ierr);
  ierr = MPI_Group_size(failed_group, &num_failed_in_group);
  chk_err(ierr);

  dprint("%d images failed.\n", num_failed_in_group);
  if (num_failed_in_group <= 0)
  {
    *perr = MPI_SUCCESS;
    return;
  }
  if (num_failed_in_group > caf_num_images)
  {
    *perr = MPI_SUCCESS;
    return;
  }

  ierr = MPI_Comm_group(comm, &comm_world_group);
  chk_err(ierr);
  ranks_of_failed_in_comm_world
      = (int *)alloca(sizeof(int) * num_failed_in_group);
  ranks_failed = (int *)alloca(sizeof(int) * num_failed_in_group);
  for (i = 0; i < num_failed_in_group; ++i)
  {
    ranks_failed[i] = i;
  }
  /* Now translate the ranks of the failed images into communicator world. */
  ierr = MPI_Group_translate_ranks(failed_group, num_failed_in_group,
                                   ranks_failed, comm_world_group,
                                   ranks_of_failed_in_comm_world);
  chk_err(ierr);

  num_images_failed += num_failed_in_group;

  if (!no_stopped_images_check_in_errhandler)
  {
    int buffer, flag;
    MPI_Request req;
    MPI_Status request_status;
    dprint("Checking for stopped images.\n");
    ierr = MPI_Irecv(&buffer, 1, MPI_INT, MPI_ANY_SOURCE,
                     MPI_TAG_CAF_SYNC_IMAGES, CAF_COMM_WORLD, &req);
    chk_err(ierr);
    if (ierr == MPI_SUCCESS)
    {
      ierr = MPI_Test(&req, &flag, &request_status);
      chk_err(ierr);
      if (flag)
      {
        // Received a result
        if (buffer == STAT_STOPPED_IMAGE)
        {
          dprint("Image #%d found stopped.\n", request_status.MPI_SOURCE);
          stopped = true;
          if (image_stati[request_status.MPI_SOURCE] == 0)
            ++num_images_stopped;
          image_stati[request_status.MPI_SOURCE] = STAT_STOPPED_IMAGE;
        }
      }
      else
      {
        dprint("No stopped images found.\n");
        ierr = MPI_Cancel(&req);
        chk_err(ierr);
      }
    }
    else
    {
      int err;
      MPI_Error_class(ierr, &err);
      dprint("Error on checking for stopped images %d.\n", err);
    }
  }

  /* TODO: Consider whether removing the failed image from images_full will be
   * necessary. This is more or less politics. */
  for (i = 0; i < num_failed_in_group; ++i)
  {
    if (ranks_of_failed_in_comm_world[i] >= 0
        && ranks_of_failed_in_comm_world[i] < caf_num_images)
    {
      if (image_stati[ranks_of_failed_in_comm_world[i]] == 0)
        image_stati[ranks_of_failed_in_comm_world[i]] = STAT_FAILED_IMAGE;
    }
    else
    {
      dprint("Rank of failed image %d out of range of images 0..%d.\n",
             ranks_of_failed_in_comm_world[i], caf_num_images);
    }
  }

redo:
  dprint("Before shrink. \n");
  ierr = MPIX_Comm_shrink(*pcomm, &shrunk);
  dprint("After shrink, rc = %d.\n", ierr);
  ierr = MPI_Comm_set_errhandler(shrunk, failed_CAF_COMM_mpi_err_handler);
  chk_err(ierr);
  ierr = MPI_Comm_size(shrunk, &ns);
  chk_err(ierr);
  ierr = MPI_Comm_rank(shrunk, &srank);
  chk_err(ierr);
  ierr = MPI_Comm_rank(*pcomm, &crank);
  chk_err(ierr);

  dprint("After getting ranks, ns = %d, srank = %d, crank = %d.\n", ns, srank,
         crank);

  /* Split does the magic: removing spare processes and reordering ranks
   * so that all surviving processes remain at their former place */
  rc = MPI_Comm_split(shrunk, (crank < 0) ? MPI_UNDEFINED : 1, crank, &newcomm);
  ierr = MPI_Comm_rank(newcomm, &newrank);
  chk_err(ierr);
  dprint("After split, rc = %d, rank = %d.\n", rc, newrank);
  flag = (rc == MPI_SUCCESS);
  /* Split or some of the communications above may have failed if
   * new failures have disrupted the process: we need to
   * make sure we succeeded at all ranks, or retry until it works. */
  flag = MPIX_Comm_agree(newcomm, &flag);
  dprint("After agree, flag = %d.\n", flag);

  ierr = MPI_Comm_rank(newcomm, &drank);
  chk_err(ierr);
  dprint("After rank, drank = %d.\n", drank);

  ierr = MPI_Comm_free(&shrunk);
  chk_err(ierr);
  if (MPI_SUCCESS != flag)
  {
    if (MPI_SUCCESS == rc)
    {
      ierr = MPI_Comm_free(&newcomm);
      chk_err(ierr);
    }
    goto redo;
  }

  {
    int cmpres;
    ierr = MPI_Comm_compare(*pcomm, CAF_COMM_WORLD, &cmpres);
    chk_err(ierr);
    dprint("Comm_compare(*comm, CAF_COMM_WORLD, res = %d) = %d.\n", cmpres,
           ierr);
    ierr = MPI_Comm_compare(*pcomm, alive_comm, &cmpres);
    chk_err(ierr);
    dprint("Comm_compare(*comm, alive_comm, res = %d) = %d.\n", cmpres, ierr);
    if (cmpres == MPI_CONGRUENT)
    {
      ierr = MPI_Win_detach(*stat_tok, &img_status);
      chk_err(ierr);
      dprint("detached win img_status.\n");
      ierr = MPI_Win_free(stat_tok);
      chk_err(ierr);
      dprint("freed win img_status.\n");
      ierr = MPI_Win_create(&img_status, sizeof(int), 1, mpi_info_same_size,
                            newcomm, stat_tok);
      chk_err(ierr);
      dprint("(re-)created win img_status.\n");
      CAF_Win_lock_all(*stat_tok);
      dprint("Win_lock_all on img_status.\n");
    }
  }
  /* Also free the old communicator before replacing it. */
  ierr = MPI_Comm_free(pcomm);
  chk_err(ierr);
  *pcomm = newcomm;

  *perr = stopped ? STAT_STOPPED_IMAGE : STAT_FAILED_IMAGE;
}
#endif

void
mutex_lock(MPI_Win win, int image_index, size_t index, int *stat,
           int *acquired_lock, char *errmsg, size_t errmsg_len)
{
  const char msg[] = "Already locked";
#if MPI_VERSION >= 3
  int value = 0, compare = 0, newval = caf_this_image, ierr = 0, i = 0;
#ifdef WITH_FAILED_IMAGES
  int flag, check_failure = 100, zero = 0;
#endif

  if (stat != NULL)
    *stat = 0;

#ifdef WITH_FAILED_IMAGES
  ierr = MPI_Test(&alive_request, &flag, MPI_STATUS_IGNORE);
  chk_err(ierr);
#endif

  locking_atomic_op(win, &value, newval, compare, image_index, index);

  if (value == caf_this_image && image_index == caf_this_image)
    goto stat_error;

  if (acquired_lock != NULL)
  {
    if (value == 0)
      *acquired_lock = 1;
    else
      *acquired_lock = 0;
    return;
  }

  while (value != 0)
  {
    ++i;
#ifdef WITH_FAILED_IMAGES
    if (i == check_failure)
    {
      i = 1;
      ierr = MPI_Test(&alive_request, &flag, MPI_STATUS_IGNORE);
      chk_err(ierr);
    }
#endif

    locking_atomic_op(win, &value, newval, compare, image_index, index);
#ifdef WITH_FAILED_IMAGES
    if (image_stati[value] == STAT_FAILED_IMAGE)
    {
      CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index - 1, win);
      /* MPI_Fetch_and_op(&zero, &newval, MPI_INT, image_index - 1,
       * index * sizeof(int), MPI_REPLACE, win); */
      ierr = MPI_Compare_and_swap(&zero, &value, &newval, MPI_INT,
                                  image_index - 1, index * sizeof(int), win);
      chk_err(ierr);
      CAF_Win_unlock(image_index - 1, win);
      break;
    }
#else
    usleep(caf_this_image * i);
#endif
  }

  if (stat)
    *stat = ierr;
  else if (ierr == STAT_FAILED_IMAGE)
    terminate_internal(ierr, 0);

  return;

stat_error:
  if (errmsg != NULL)
  {
    memset(errmsg, ' ', errmsg_len);
    memcpy(errmsg, msg, MIN(errmsg_len, strlen(msg)));
  }

  if (stat != NULL)
    *stat = 99;
  else
    terminate_internal(99, 1);
#else // MPI_VERSION
#warning Locking for MPI-2 is not implemented
  printf("Locking for MPI-2 is not supported, "
         "please update your MPI implementation\n");
#endif // MPI_VERSION
}

void
mutex_unlock(MPI_Win win, int image_index, size_t index, int *stat,
             char *errmsg, size_t errmsg_len)
{
  const char msg[] = "Variable is not locked";
  if (stat != NULL)
    *stat = 0;
#if MPI_VERSION >= 3
  /* Mark `flag` unused, because of conditional compilation.  */
  int value = 1, ierr = 0, newval = 0, flag __attribute__((unused));
#ifdef WITH_FAILED_IMAGES
  ierr = MPI_Test(&alive_request, &flag, MPI_STATUS_IGNORE);
  chk_err(ierr);
#endif

  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index - 1, win);
  ierr = MPI_Fetch_and_op(&newval, &value, MPI_INT, image_index - 1,
                          index * sizeof(int), MPI_REPLACE, win);
  chk_err(ierr);
  ierr = CAF_Win_unlock(image_index - 1, win);
  chk_err(ierr);

  if (value == 0)
    goto stat_error;

  if (stat)
    *stat = ierr;
  else if (ierr == STAT_FAILED_IMAGE)
    terminate_internal(ierr, 0);

  return;

stat_error:
  if (errmsg != NULL)
  {
    memset(errmsg, ' ', errmsg_len);
    memcpy(errmsg, msg, MIN(errmsg_len, strlen(msg)));
  }
  if (stat != NULL)
    *stat = 99;
  else
    terminate_internal(99, 1);
#else // MPI_VERSION
#warning Locking for MPI-2 is not implemented
  printf("Locking for MPI-2 is not supported, "
         "please update your MPI implementation\n");
#endif // MPI_VERSION
}

/* Initialize coarray program.  This routine assumes that no other
 * MPI initialization happened before. */

void
PREFIX(init)(int *argc, char ***argv)
{
  int flag;
  if (caf_num_images == 0)
  {
    /* Flag rc as unused, because conditional compilation.  */
    int ierr = 0, i = 0, j = 0, rc __attribute__((unused)), prov_lev = 0;
    int is_init = 0, prior_thread_level = MPI_THREAD_MULTIPLE;
#ifdef GCC_GE_16
    MPI_Comm tmp_world;
#endif

    ierr = MPI_Initialized(&is_init);
    chk_err(ierr);

    if (is_init)
    {
      ierr = MPI_Query_thread(&prior_thread_level);
      chk_err(ierr);
    }
    dprint("Main thread: thread level: %d\n", prior_thread_level);
#ifdef HELPER
    if (is_init)
    {
      prov_lev = prior_thread_level;
      caf_owns_mpi = false;
    }
    else
    {
      ierr = MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &prov_lev);
      chk_err(ierr);
      caf_owns_mpi = true;
    }

    if (caf_this_image == 0 && MPI_THREAD_MULTIPLE != prov_lev)
      caf_runtime_error("MPI_THREAD_MULTIPLE is not supported: %d", prov_lev);
#else
    if (is_init)
      caf_owns_mpi = false;
    else
    {
      ierr = MPI_Init_thread(argc, argv, prior_thread_level, &prov_lev);
      chk_err(ierr);
      caf_owns_mpi = true;
      if (caf_this_image == 0 && MPI_THREAD_FUNNELED > prov_lev)
        caf_runtime_error("MPI_THREAD_FUNNELED is not supported: %d %d",
                          MPI_THREAD_FUNNELED, prov_lev);
    }
#endif
    if (unlikely((ierr != MPI_SUCCESS)))
      caf_runtime_error("Failure when initializing MPI: %d", ierr);

    ierr = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    chk_err(ierr);

    /* Duplicate MPI_COMM_WORLD so that no CAF internal functions use it.
     * This is critical for MPI-interoperability. */
#ifdef GCC_GE_16
    rc = MPI_Comm_dup(MPI_COMM_WORLD, &tmp_world);
    CAF_COMM_WORLD_store = &tmp_world;
#else
    rc = MPI_Comm_dup(MPI_COMM_WORLD, &CAF_COMM_WORLD);
#endif
#ifdef WITH_FAILED_IMAGES
    flag = (MPI_SUCCESS == rc);
    rc = MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    if (rc != MPI_SUCCESS)
    {
      dprint("MPIX_Comm_agree(flag = %d) = %d.\n", flag, rc);
      fflush(stderr);
      MPI_Abort(MPI_COMM_WORLD, 10000);
    }
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    ierr = MPI_Comm_size(CAF_COMM_WORLD, &caf_num_images);
    chk_err(ierr);
    ierr = MPI_Comm_rank(CAF_COMM_WORLD, &mpi_this_image);
    chk_err(ierr);

    global_this_image = mpi_this_image;
    caf_this_image = mpi_this_image + 1;
    global_num_images = caf_num_images;
    caf_is_finalized = 0;

#ifdef EXTRA_DEBUG_OUTPUT
    pid_t mypid = getpid();
    dprint("I have pid %d.\n", mypid);
#endif

    /* BEGIN SYNC IMAGE preparation
     * Prepare memory for syncing images. */
    images_full = (int *)calloc(caf_num_images - 1, sizeof(int));
    for (i = 1, j = 0; i <= caf_num_images; ++i)
    {
      if (i != caf_this_image)
        images_full[j++] = i;
    }

    arrived = calloc(caf_num_images, sizeof(int));
    sync_handles = malloc(caf_num_images * sizeof(MPI_Request));
    /* END SYNC IMAGE preparation. */

    /* BEGIN teams preparations. */
#ifdef GCC_GE_16
    teams_list = (caf_teams_list_t *)malloc(sizeof(caf_teams_list_t));
    *teams_list = (struct caf_teams_list){tmp_world, -1, caf_this_image, NULL};
    CAF_COMM_WORLD_store = &teams_list->communicator;
    current_team
        = (caf_team_stack_node_t *)malloc(sizeof(caf_team_stack_node_t));
    *current_team = (struct caf_team_stack_node){teams_list, NULL};
    initial_team = current_team;
#else
    teams_list = (caf_teams_list *)calloc(1, sizeof(caf_teams_list));
    teams_list->team_id = -1;
    MPI_Comm *tmp_comm = (MPI_Comm *)calloc(1, sizeof(MPI_Comm));
    *tmp_comm = CAF_COMM_WORLD;
    teams_list->team = tmp_comm;
    teams_list->prev = NULL;
    used_teams = (caf_used_teams_list *)calloc(1, sizeof(caf_used_teams_list));
    used_teams->team_list_elem = teams_list;
    used_teams->prev = NULL;
#endif
    /* END teams preparations. */

#ifdef WITH_FAILED_IMAGES
    MPI_Comm_dup(MPI_COMM_WORLD, &alive_comm);
    /* Handling of failed/stopped images is done by setting an error handler
     * on a asynchronous request to each other image.  For a failing image
     * the request will trigger the call of the error handler thus allowing
     * each other image to handle the failed/stopped image. */
    ierr = MPI_Comm_create_errhandler(failed_stopped_errorhandler_function,
                                      &failed_CAF_COMM_mpi_err_handler);
    chk_err(ierr);
    ierr = MPI_Comm_set_errhandler(CAF_COMM_WORLD,
                                   failed_CAF_COMM_mpi_err_handler);
    chk_err(ierr);
    ierr = MPI_Comm_set_errhandler(alive_comm, failed_CAF_COMM_mpi_err_handler);
    chk_err(ierr);
    ierr = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    chk_err(ierr);

    ierr = MPI_Irecv(&alive_dummy, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                     alive_comm, &alive_request);
    chk_err(ierr);

    image_stati = (int *)calloc(caf_num_images, sizeof(int));
#endif

#if MPI_VERSION >= 3
    ierr = MPI_Info_create(&mpi_info_same_size);
    chk_err(ierr);
    ierr = MPI_Info_set(mpi_info_same_size, "same_size", "true");
    chk_err(ierr);

    /* Setting img_status */
    ierr = MPI_Win_create(&img_status, sizeof(int), 1, mpi_info_same_size,
                          CAF_COMM_WORLD, &stat_tok);
    chk_err(ierr);
    CAF_Win_lock_all(stat_tok);
#else
    ierr = MPI_Win_create(&img_status, sizeof(int), 1, MPI_INFO_NULL,
                          CAF_COMM_WORLD, &stat_tok);
    chk_err(ierr);
#endif // MPI_VERSION

    /* Create the dynamic window to allow images to asyncronously attach
     * memory. */
    ierr = MPI_Win_create_dynamic(MPI_INFO_NULL, CAF_COMM_WORLD,
                                  &global_dynamic_win);
    chk_err(ierr);

    CAF_Win_lock_all(global_dynamic_win);
#ifdef EXTRA_DEBUG_OUTPUT
    if (caf_this_image == 1)
    {
      int *win_model;
      flag = 0;
      ierr = MPI_Win_get_attr(global_dynamic_win, MPI_WIN_MODEL, &win_model,
                              &flag);
      chk_err(ierr);
      dprint("The mpi memory model is: %s (0x%x, %d).\n",
             *win_model == MPI_WIN_UNIFIED ? "unified " : "separate",
             *win_model, flag);
    }
#endif

#ifdef GCC_GE_15
    ierr = MPI_Comm_dup(CAF_COMM_WORLD, &ct_COMM);
    chk_err(ierr);
    ierr = pthread_create(&commthread, NULL, &communication_thread, NULL);
    chk_err(ierr);
#endif
  }
}

/* Internal finalize of coarray program. */

void
finalize_internal(int status_code)
{
  int ierr;
  dprint("(status_code = %d)\n", status_code);

#ifdef WITH_FAILED_IMAGES
  no_stopped_images_check_in_errhandler = true;
  ierr = MPI_Win_flush_all(*stat_tok);
  chk_err(ierr);
#endif
  /* For future security enclose setting img_status in a lock. */
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_this_image, stat_tok);
  if (status_code == 0)
  {
    img_status = STAT_STOPPED_IMAGE;
#ifdef WITH_FAILED_IMAGES
    image_stati[mpi_this_image] = STAT_STOPPED_IMAGE;
#endif
  }
  else
  {
    img_status = status_code;
#ifdef WITH_FAILED_IMAGES
    image_stati[mpi_this_image] = status_code;
#endif
  }
  CAF_Win_unlock(mpi_this_image, stat_tok);

  /* Announce to all other images, that this one has changed its execution
   * status. */
  for (int i = 0; i < caf_num_images - 1; ++i)
  {
    ierr = MPI_Send(&img_status, 1, MPI_INT, images_full[i] - 1,
                    MPI_TAG_CAF_SYNC_IMAGES, CAF_COMM_WORLD);
    chk_err(ierr);
  }

#ifdef WITH_FAILED_IMAGES
  /* Terminate the async request before revoking the comm, or we will get
   * triggered by the errorhandler, which we don't want here anymore. */
  ierr = MPI_Cancel(&alive_request);
  chk_err(ierr);

  if (status_code == 0)
  {
    /* In finalization do not report stopped or failed images any more. */
    ierr = MPI_Errhandler_set(CAF_COMM_WORLD, MPI_ERRORS_RETURN);
    chk_err(ierr);
    ierr = MPI_Errhandler_set(alive_comm, MPI_ERRORS_RETURN);
    chk_err(ierr);
    /* Only add a conventional barrier to prevent images rom quitting too
     * early, when this images is not failing. */
    dprint("Before MPI_Barrier(CAF_COMM_WORLD)\n");
    ierr = MPI_Barrier(CAF_COMM_WORLD);
    chk_err(ierr);
    dprint("After MPI_Barrier(CAF_COMM_WORLD) = %d\n", ierr);
  }
  else
    return;
#else
  /* Add a conventional barrier to prevent images from quitting too early. */
  if (status_code == 0)
  {
    if (caf_num_images > 1)
    {
      dprint("In barrier for finalize...");
      ierr = MPI_Barrier(CAF_COMM_WORLD);
      chk_err(ierr);
    }
  }
  else
    /* Without failed images support, but a given status_code, we need to
     * return to the caller, or we will hang in the following instead of
     * terminating the program. */
    return;
#endif

#ifdef GCC_GE_7
  struct caf_allocated_slave_tokens_t *cur_stok = caf_allocated_slave_tokens,
                                      *prev_stok = NULL;
  CAF_Win_unlock_all(global_dynamic_win);
  while (cur_stok)
  {
    prev_stok = cur_stok->prev;
    dprint("freeing slave token %p for memory %p", cur_stok->token,
           cur_stok->token->memptr);
    ierr = MPI_Win_detach(global_dynamic_win, cur_stok->token);
    chk_err(ierr);
    if (cur_stok->token->memptr)
    {
      ierr = MPI_Win_detach(global_dynamic_win, cur_stok->token->memptr);
      chk_err(ierr);
      ierr = MPI_Free_mem(cur_stok->token->memptr);
      chk_err(ierr);
    }
    ierr = MPI_Free_mem(cur_stok->token);
    chk_err(ierr);
    free(cur_stok);
    cur_stok = prev_stok;
  }
#else
  CAF_Win_unlock_all(global_dynamic_win);
#endif

  dprint("Freed all slave tokens.\n");

#ifdef GCC_GE_16
  for (caf_team_stack_node_t *node = current_team; node;)
  {
    caf_team_stack_node_t *pn = node->parent;
    MPI_Win *p;

    for (struct allocated_tokens_t *t = node->allocated_tokens; t;)
    {
      struct allocated_tokens_t *nt = t->next;

      p = TOKEN(t->token);
      if (p != NULL)
        CAF_Win_unlock_all(*p);
      /* Unregister the window to the descriptors when freeing the token. */
      ierr = MPI_Win_free(p);
      chk_err(ierr);
      free(t->token);
      free(t);
      t = nt;
    }
    free(node);
    node = pn;
  }
  current_team = initial_team = NULL;

  for (caf_teams_list_t *node = teams_list; node;)
  {
    caf_teams_list_t *pn = node->prev;

    if (node->team_id != -1)
      MPI_Comm_free(&node->communicator);

    free(node);
    node = pn;
  }
#else
  struct caf_allocated_tokens_t *cur_tok = caf_allocated_tokens,
                                *prev = caf_allocated_tokens;
  MPI_Win *p;

  while (cur_tok)
  {
    prev = cur_tok->prev;
    p = TOKEN(cur_tok->token);
    if (p != NULL)
      CAF_Win_unlock_all(*p);
#ifdef GCC_GE_7
    /* Unregister the window to the descriptors when freeing the token. */
    dprint("MPI_Win_free(%p)\n", p);
    ierr = MPI_Win_free(p);
    chk_err(ierr);
    free(cur_tok->token);
#else  // GCC_GE_7
    ierr = MPI_Win_free(p);
    chk_err(ierr);
#endif // GCC_GE_7
    free(cur_tok);
    cur_tok = prev;
  }
#endif
#if MPI_VERSION >= 3
  ierr = MPI_Info_free(&mpi_info_same_size);
  chk_err(ierr);
#endif // MPI_VERSION

#ifdef GCC_GE_15
  dprint("Sending termination signal to communication thread.\n");
  commthread_running = false;
  ierr = MPI_Send(NULL, 0, MPI_BYTE, mpi_this_image, CAF_CT_TAG, ct_COMM);
  chk_err(ierr);
  dprint("Termination signal send, waiting for thread join.\n");
  ierr = pthread_join(commthread, NULL);
  dprint("Communication thread terminated with rc = %d.\n", ierr);
  dprint("Freeing ct_COMM.\n");
  MPI_Comm_free(&ct_COMM);
  dprint("Freeed ct_COMM.\n");
#endif

  /* Free the global dynamic window. */
  ierr = MPI_Win_free(&global_dynamic_win);
  chk_err(ierr);
#ifdef WITH_FAILED_IMAGES
  if (status_code == 0)
  {
    dprint("before Win_unlock_all.\n");
    CAF_Win_unlock_all(*stat_tok);
    dprint("before Win_free(stat_tok)\n");
    ierr = MPI_Win_free(stat_tok);
    chk_err(ierr);
    dprint("before Comm_free(CAF_COMM_WORLD)\n");
    ierr = MPI_Comm_free(&CAF_COMM_WORLD);
    chk_err(ierr);
    ierr = MPI_Comm_free(&alive_comm);
    chk_err(ierr);
    dprint("after Comm_free(CAF_COMM_WORLD)\n");
  }

  ierr = MPI_Errhandler_free(&failed_CAF_COMM_mpi_err_handler);
  chk_err(ierr);

  /* Only call Finalize if CAF runtime Initialized MPI. */
  if (caf_owns_mpi)
  {
    ierr = MPI_Finalize();
    chk_err(ierr);
  }
#else
#ifdef MPI_CLEAR_COMM_BEFORE_FREE
  {
    int probe_flag;
    MPI_Status status;
    do
    {
      ierr = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                        &probe_flag, &status); /* error is not of interest. */
      if (probe_flag)
      {
        int cnt;
        MPI_Get_count(&status, MPI_BYTE, &cnt);
        void *buf = alloca(cnt);
        ierr = MPI_Recv(buf, cnt, MPI_BYTE, status.MPI_SOURCE, status.MPI_TAG,
                        MPI_COMM_WORLD, &status);
        chk_err(ierr);
      }
    } while (probe_flag);
  }
#endif
#ifndef GCC_GE_16
  dprint("freeing caf's communicator.\n");
  ierr = MPI_Comm_free(&CAF_COMM_WORLD);
  chk_err(ierr);
#endif

  CAF_Win_unlock_all(stat_tok);
  ierr = MPI_Win_free(&stat_tok);
  chk_err(ierr);

  /* Only call Finalize if CAF runtime Initialized MPI. */
  if (caf_owns_mpi)
  {
    dprint("Finalizing MPI.\n");
    ierr = MPI_Finalize();
    chk_err(ierr);
  }
#endif

#ifdef HELPER
  pthread_mutex_lock(&lock_am);
  caf_is_finalized = 1;
  pthread_mutex_unlock(&lock_am);
#else
  caf_is_finalized = 1;
#endif
  free(sync_handles);

  dprint("Finalisation done!!!\n");
}

/* Finalize coarray program. */
void
PREFIX(finalize)(void)
{
  finalize_internal(0);
}

#ifdef GCC_GE_16
int
PREFIX(this_image)(caf_team_t team)
{
  return team ? ((caf_teams_list_t *)team)->image_id : caf_this_image;
}

int
PREFIX(num_images)(caf_team_t team, int32_t *team_number)
{
  if (team)
  {
    caf_teams_list_t *t = (caf_teams_list_t *)team;
    int comm_size = 0, ierr;
    ierr = MPI_Comm_size(t->communicator, &comm_size);
    chk_err(ierr);
    return comm_size;
  }
  if (team_number)
  {
    for (caf_teams_list_t *curr = teams_list; curr; curr = curr->prev)
      if (curr->team_id == *team_number)
      {
        int comm_size = 0, ierr;
        ierr = MPI_Comm_size(curr->communicator, &comm_size);
        chk_err(ierr);
        return comm_size;
      }

    caf_runtime_error("NUM_IMAGES: Unknown team_number %d", *team_number);
  }
  return caf_num_images;
}
#else
/* TODO: This is interface is violating the F2015 standard, but not the gfortran
 * API. Fix it (the fortran API). */
int
PREFIX(this_image)(int distance __attribute__((unused)))
{
  return caf_this_image;
}

/* TODO: This is interface is violating the F2015 standard, but not the gfortran
 * API. Fix it (the fortran API). */
int
PREFIX(num_images)(int distance __attribute__((unused)),
                   int failed __attribute__((unused)))
{
  return caf_num_images;
}
#endif

#ifdef GCC_GE_7
/* Register an object with the coarray library creating a token where
 * necessary/requested.
 * See the ABI-documentation of gfortran for the expected behavior.
 * Contrary to this expected behavior is this routine not registering memory
 * in the descriptor, that is already present.  I.e., when the compiler
 * expects the library to allocate the memory for an object in desc, then
 * its data_ptr is NULL. This is still missing here.  At the moment the
 * compiler also does not make use of it, but it is contrary to the
 * documentation. */
void PREFIX(register)(size_t size, caf_register_t type, caf_token_t *token,
                      gfc_descriptor_t *desc, int *stat, char *errmsg,
                      charlen_t errmsg_len)
{
  void *mem = NULL;
  size_t actual_size;
  int l_var = 0, *init_array = NULL, ierr;

  if (unlikely(caf_is_finalized))
    goto error;

  /* Start MPI if not already started. */
  if (caf_num_images == 0)
    PREFIX(init)(NULL, NULL);

  if (type == CAF_REGTYPE_LOCK_STATIC || type == CAF_REGTYPE_LOCK_ALLOC
      || type == CAF_REGTYPE_CRITICAL || type == CAF_REGTYPE_EVENT_STATIC
      || type == CAF_REGTYPE_EVENT_ALLOC)
  {
    actual_size = size * sizeof(int);
    l_var = 1;
  }
  else
    actual_size = size;

  dprint("size = %zd, type = %d, token = %p, desc = %p, rank = %d\n", size,
         type, token, desc, GFC_DESCRIPTOR_RANK(desc));
  switch (type)
  {
    case CAF_REGTYPE_COARRAY_ALLOC_REGISTER_ONLY:
    case CAF_REGTYPE_COARRAY_ALLOC_ALLOCATE_ONLY:
      {
        /* Create or allocate a slave token. */
        mpi_caf_slave_token_t *slave_token;
#ifdef EXTRA_DEBUG_OUTPUT
        MPI_Aint mpi_address = 0;
#endif
        CAF_Win_unlock_all(global_dynamic_win);
        if (type == CAF_REGTYPE_COARRAY_ALLOC_REGISTER_ONLY)
        {
          ierr = MPI_Alloc_mem(sizeof(mpi_caf_slave_token_t), MPI_INFO_NULL,
                               token);
          chk_err(ierr);
          slave_token = (mpi_caf_slave_token_t *)(*token);
          slave_token->memptr = NULL;
          slave_token->desc = NULL;
          ierr = MPI_Win_attach(global_dynamic_win, slave_token,
                                sizeof(mpi_caf_slave_token_t));
          chk_err(ierr);
#ifdef EXTRA_DEBUG_OUTPUT
          ierr = MPI_Get_address(*token, &mpi_address);
          chk_err(ierr);
#endif
          dprint("Attach slave token %p (size: %zd, mpi-address: %lx) to "
                 "global_dynamic_window = %d\n",
                 slave_token, sizeof(mpi_caf_slave_token_t), mpi_address,
                 global_dynamic_win);

          /* Register the memory for auto freeing. */
          struct caf_allocated_slave_tokens_t *tmp
              = malloc(sizeof(struct caf_allocated_slave_tokens_t));
          tmp->prev = caf_allocated_slave_tokens;
          tmp->token = slave_token;
          caf_allocated_slave_tokens = tmp;
        }
        else // (type == CAF_REGTYPE_COARRAY_ALLOC_ALLOCATE_ONLY)
        {
          int ierr;
          slave_token = (mpi_caf_slave_token_t *)(*token);
          ierr = MPI_Alloc_mem(actual_size, MPI_INFO_NULL, &mem);
          chk_err(ierr);
          slave_token->memptr = mem;
          ierr = MPI_Win_attach(global_dynamic_win, mem, actual_size);
          chk_err(ierr);
#ifdef EXTRA_DEBUG_OUTPUT
          ierr = MPI_Get_address(mem, &mpi_address);
          chk_err(ierr);
#endif
          dprint("Attach mem %p (mpi-address: %lx) to global_dynamic_window = "
                 "%d on slave_token %p, size %zd, ierr: %d\n",
                 mem, mpi_address, global_dynamic_win, slave_token, actual_size,
                 ierr);
          if (desc != NULL && GFC_DESCRIPTOR_RANK(desc) != 0)
          {
            slave_token->desc = desc;
#ifdef EXTRA_DEBUG_OUTPUT
            ierr = MPI_Get_address(desc, &mpi_address);
            chk_err(ierr);
#endif
            dprint("Attached descriptor %p (mpi-address: %zd) to "
                   "global_dynamic_window %d at address %p, ierr = %d.\n",
                   desc, mpi_address, global_dynamic_win, &slave_token->desc,
                   ierr);
          }
        }
        CAF_Win_lock_all(global_dynamic_win);
        dprint("Slave token %p on exit: mpi_caf_slave_token_t { memptr: %p, "
               "desc: %p }\n",
               slave_token, slave_token->memptr, slave_token->desc);
      }
      break;
    default:
      {
        mpi_caf_token_t *mpi_token;
        MPI_Win *p;

        *token = calloc(1, sizeof(mpi_caf_token_t));
        mpi_token = (mpi_caf_token_t *)(*token);
        p = TOKEN(mpi_token);

#if MPI_VERSION >= 3
        ierr = MPI_Win_allocate(actual_size, 1, MPI_INFO_NULL, CAF_COMM_WORLD,
                                &mem, p);
        chk_err(ierr);
        CAF_Win_lock_all(*p);
#else
        ierr = MPI_Alloc_mem(actual_size, MPI_INFO_NULL, &mem);
        chk_err(ierr);
        ierr = MPI_Win_create(mem, actual_size, 1, MPI_INFO_NULL,
                              CAF_COMM_WORLD, p);
        chk_err(ierr);
#endif // MPI_VERSION

#ifndef GCC_GE_8
        if (GFC_DESCRIPTOR_RANK(desc) != 0)
#endif
          mpi_token->desc = desc;

        if (l_var)
        {
          init_array = (int *)calloc(size, sizeof(int));
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_this_image, *p);
          ierr = MPI_Put(init_array, size, MPI_INT, mpi_this_image, 0, size,
                         MPI_INT, *p);
          chk_err(ierr);
          CAF_Win_unlock(mpi_this_image, *p);
          free(init_array);
        }

#ifdef GCC_GE_16
        struct allocated_tokens_t *allocated_token
            = malloc(sizeof(struct allocated_tokens_t));
        allocated_token->next = current_team->allocated_tokens;
        allocated_token->token = *token;
        current_team->allocated_tokens = allocated_token;
#else
        struct caf_allocated_tokens_t *tmp
            = malloc(sizeof(struct caf_allocated_tokens_t));
        tmp->prev = caf_allocated_tokens;
        tmp->token = *token;
        caf_allocated_tokens = tmp;
#endif

        if (stat)
          *stat = 0;

        /* The descriptor will be initialized only after the call to
         * register. */
        mpi_token->memptr = mem;
        dprint("Token %p on exit: mpi_caf_token_t "
               "{ (local_)memptr: %p (size: %zd), memptr_win: %d }\n",
               mpi_token, mpi_token->memptr, size, mpi_token->memptr_win);
      } // default:
      break;
  } // switch

  desc->base_addr = mem;
  return;

error:
  {
    char msg[80];
    strcpy(msg, "Failed to allocate coarray");
    if (caf_is_finalized)
      strcat(msg, " - there are stopped images");

    if (stat)
    {
      *stat = caf_is_finalized ? STAT_STOPPED_IMAGE : 1;
      if (errmsg_len > 0)
      {
        size_t len = (strlen(msg) > (size_t)errmsg_len) ? (size_t)errmsg_len
                                                        : strlen(msg);
        memcpy(errmsg, msg, len);
        if ((size_t)errmsg_len > len)
          memset(&errmsg[len], ' ', errmsg_len - len);
      }
    }
    else
      caf_runtime_error(msg);
  }
}
#else // GCC_LT_7
void *PREFIX(register)(size_t size, caf_register_t type, caf_token_t *token,
                       int *stat, char *errmsg, charlen_t errmsg_len)
{
  void *mem;
  size_t actual_size;
  int l_var = 0, *init_array = NULL, ierr;

  if (unlikely(caf_is_finalized))
    goto error;

  /* Start MPI if not already started. */
  if (caf_num_images == 0)
#ifdef COMPILER_SUPPORTS_CAF_INTRINSICS
    _gfortran_caf_init(NULL, NULL);
#else
    PREFIX(init)(NULL, NULL);
#endif

  /* Token contains only a list of pointers. */
  *token = malloc(sizeof(MPI_Win));
  MPI_Win *p = *token;

  if (type == CAF_REGTYPE_LOCK_STATIC || type == CAF_REGTYPE_LOCK_ALLOC
      || type == CAF_REGTYPE_CRITICAL || type == CAF_REGTYPE_EVENT_STATIC
      || type == CAF_REGTYPE_EVENT_ALLOC)
  {
    actual_size = size * sizeof(int);
    l_var = 1;
  }
  else
    actual_size = size;

#if MPI_VERSION >= 3
  ierr = MPI_Win_allocate(actual_size, 1, mpi_info_same_size, CAF_COMM_WORLD,
                          &mem, p);
  chk_err(ierr);
  CAF_Win_lock_all(*p);
#else
  ierr = MPI_Alloc_mem(actual_size, MPI_INFO_NULL, &mem);
  chk_err(ierr);
  ierr = MPI_Win_create(mem, actual_size, 1, MPI_INFO_NULL, CAF_COMM_WORLD, p);
  chk_err(ierr);
#endif // MPI_VERSION

  if (l_var)
  {
    init_array = (int *)calloc(size, sizeof(int));
    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, mpi_this_image, *p);
    ierr = MPI_Put(init_array, size, MPI_INT, mpi_this_image, 0, size, MPI_INT,
                   *p);
    chk_err(ierr);
    CAF_Win_unlock(mpi_this_image, *p);
    free(init_array);
  }

  PREFIX(sync_all)(NULL, NULL, 0);

  struct caf_allocated_tokens_t *tmp
      = malloc(sizeof(struct caf_allocated_tokens_t));
  tmp->prev = caf_allocated_tokens;
  tmp->token = *token;
  caf_allocated_tokens = tmp;

  if (stat)
    *stat = 0;
  return mem;

error:
  {
    char msg[80];
    strcpy(msg, "Failed to allocate coarray");
    if (caf_is_finalized)
      strcat(msg, " - there are stopped images");

    if (stat)
    {
      *stat = caf_is_finalized ? STAT_STOPPED_IMAGE : 1;
      if (errmsg_len > 0)
      {
        size_t len = (strlen(msg) > (size_t)errmsg_len) ? (size_t)errmsg_len
                                                        : strlen(msg);
        memcpy(errmsg, msg, len);
        if (errmsg_len > len)
          memset(&errmsg[len], ' ', errmsg_len - len);
      }
    }
    else
      caf_runtime_error(msg);
  }
  return NULL;
}
#endif // GCC_GE_7

#ifdef GCC_GE_7
void
PREFIX(deregister)(caf_token_t *token, int type, int *stat, char *errmsg,
                   charlen_t errmsg_len)
#else
void
PREFIX(deregister)(caf_token_t *token, int *stat, char *errmsg,
                   charlen_t errmsg_len)
#endif
{
  dprint("deregister(%p)\n", *token);
  int ierr;

  if (unlikely(caf_is_finalized))
  {
    const char msg[]
        = "Failed to deallocate coarray - there are stopped images";
    if (stat)
    {
      *stat = STAT_STOPPED_IMAGE;

      if (errmsg_len > 0)
      {
        size_t len = (sizeof(msg) - 1 > (size_t)errmsg_len) ? (size_t)errmsg_len
                                                            : sizeof(msg) - 1;
        memcpy(errmsg, msg, len);
        if (errmsg_len > len)
          memset(&errmsg[len], ' ', errmsg_len - len);
      }
      return;
    }
    caf_runtime_error(msg);
  }

  if (stat)
    *stat = 0;

#ifdef GCC_GE_7
  if (type != CAF_DEREGTYPE_COARRAY_DEALLOCATE_ONLY)
  {
    /* Sync all images only, when deregistering the token. Just freeing the
     * memory needs no sync. */
#ifdef WITH_FAILED_IMAGES
    ierr = MPI_Barrier(CAF_COMM_WORLD);
    chk_err(ierr);
#else
    PREFIX(sync_all)(NULL, NULL, 0);
#endif
  }
#endif // GCC_GE_7
#ifdef GCC_GE_16
  {
    struct allocated_tokens_t *cur = current_team->allocated_tokens,
                              *prev = current_team->allocated_tokens;
    MPI_Win *p;

    while (cur)
    {
      if (cur->token == *token)
      {
        p = TOKEN(*token);
#ifdef GCC_GE_7
        dprint("Found regular token %p for memptr_win: %d.\n", *token,
               ((mpi_caf_token_t *)*token)->memptr_win);
#endif
        CAF_Win_unlock_all(*p);
        ierr = MPI_Win_free(p);
        chk_err(ierr);

        if (cur == current_team->allocated_tokens)
          current_team->allocated_tokens = cur->next;
        else
          prev->next = cur->next;

        free(cur);
        free(*token);
        *token = NULL;
        return;
      }
      prev = cur;
      cur = cur->next;
    }
  }
#else
  {
    struct caf_allocated_tokens_t *cur = caf_allocated_tokens,
                                  *next = caf_allocated_tokens, *prev;
    MPI_Win *p;

    while (cur)
    {
      prev = cur->prev;

      if (cur->token == *token)
      {
        p = TOKEN(*token);
#ifdef GCC_GE_7
        dprint("Found regular token %p for memptr_win: %d.\n", *token,
               ((mpi_caf_token_t *)*token)->memptr_win);
#endif
        CAF_Win_unlock_all(*p);
        ierr = MPI_Win_free(p);
        chk_err(ierr);

        next->prev = prev ? prev->prev : NULL;

        if (cur == caf_allocated_tokens)
          caf_allocated_tokens = prev;

        free(cur);
        free(*token);
        *token = NULL;
        return;
      }
      next = cur;
      cur = prev;
    }
  }
#endif

#ifdef GCC_GE_7
  /* Feel through: Has to be a component token. */
  {
    struct caf_allocated_slave_tokens_t *cur_stok = caf_allocated_slave_tokens,
                                        *next_stok = caf_allocated_slave_tokens,
                                        *prev_stok;

    while (cur_stok)
    {
      prev_stok = cur_stok->prev;

      if (cur_stok->token == *token)
      {
        dprint("Found sub token %p.\n", *token);

        mpi_caf_slave_token_t *slave_token = *(mpi_caf_slave_token_t **)token;
        CAF_Win_unlock_all(global_dynamic_win);

        if (slave_token->memptr)
        {
          ierr = MPI_Win_detach(global_dynamic_win, slave_token->memptr);
          chk_err(ierr);
          ierr = MPI_Free_mem(slave_token->memptr);
          chk_err(ierr);
          slave_token->memptr = NULL;
          if (type == CAF_DEREGTYPE_COARRAY_DEALLOCATE_ONLY)
          {
            CAF_Win_lock_all(global_dynamic_win);
            return; // All done.
          }
        }
        ierr = MPI_Win_detach(global_dynamic_win, slave_token);
        chk_err(ierr);
        CAF_Win_lock_all(global_dynamic_win);

        next_stok->prev = prev_stok ? prev_stok->prev : NULL;

        if (cur_stok == caf_allocated_slave_tokens)
          caf_allocated_slave_tokens = prev_stok;

        free(cur_stok);
        ierr = MPI_Free_mem(*token);
        chk_err(ierr);
        *token = NULL;
        return;
      }

      next_stok = cur_stok;
      cur_stok = prev_stok;
    }
  }
#endif // GCC_GE_7
#ifdef EXTRA_DEBUG_OUTPUT
  fprintf(stderr,
          "Fortran runtime warning on image %d: "
          "Could not find token to free %p",
          caf_this_image, *token);
#endif
}

void
PREFIX(sync_memory)(int *stat __attribute__((unused)),
                    char *errmsg __attribute__((unused)),
                    charlen_t errmsg_len __attribute__((unused)))
{
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
  explicit_flush();
#endif
}

void
PREFIX(sync_all)(int *stat, char *errmsg, charlen_t errmsg_len)
{
  int err = 0, ierr;

  dprint("Entering sync all.\n");
  if (unlikely(caf_is_finalized))
  {
    err = STAT_STOPPED_IMAGE;
  }
  else
  {
#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
    explicit_flush();
#endif

#ifdef WITH_FAILED_IMAGES
    ierr = MPI_Barrier(alive_comm);
    chk_err(ierr);
#else
    ierr = MPI_Barrier(CAF_COMM_WORLD);
    chk_err(ierr);
#endif
    dprint("MPI_Barrier = %d.\n", err);
    if (ierr == STAT_FAILED_IMAGE)
      err = STAT_FAILED_IMAGE;
    else if (ierr != 0)
      MPI_Error_class(ierr, &err);
  }

  if (stat != NULL)
    *stat = err;
#ifdef WITH_FAILED_IMAGES
  else if (err == STAT_FAILED_IMAGE)
    /* F2015 requests stat to be set for FAILED IMAGES, else error out. */
    terminate_internal(err, 0);
#endif

  if (err != 0 && err != STAT_FAILED_IMAGE)
  {
    char msg[80];
    strcpy(msg, "SYNC ALL failed");
    if (caf_is_finalized)
      strcat(msg, " - there are stopped images");

    if (errmsg_len > 0)
    {
      size_t len = (strlen(msg) > (size_t)errmsg_len) ? (size_t)errmsg_len
                                                      : strlen(msg);
      memcpy(errmsg, msg, len);
      if (errmsg_len > len)
        memset(&errmsg[len], ' ', errmsg_len - len);
    }
    else if (stat == NULL)
      caf_runtime_error(msg);
  }
  dprint("Leaving sync all.\n");
}

/* Convert kind 4 characters into kind 1 one.
 * Copied from the gcc:libgfortran/caf/single.c. */
static void
assign_char4_from_char1(size_t dst_size, size_t src_size, uint32_t *dst,
                        unsigned char *src)
{
  size_t i, n;
  n = (dst_size > src_size) ? src_size : dst_size;
  for (i = 0; i < n; ++i)
  {
    dst[i] = (int32_t)src[i];
  }
  for (; i < dst_size; ++i)
  {
    dst[i] = (int32_t)' ';
  }
}

/* Convert kind 1 characters into kind 4 one.
 * Copied from the gcc:libgfortran/caf/single.c. */
static void
assign_char1_from_char4(size_t dst_size, size_t src_size, unsigned char *dst,
                        uint32_t *src)
{
  size_t i, n;
  n = (dst_size > src_size) ? src_size : dst_size;
  for (i = 0; i < n; ++i)
  {
    dst[i] = src[i] > UINT8_MAX ? (unsigned char)'?' : (unsigned char)src[i];
  }
  if (dst_size > n)
    memset(&dst[n], ' ', dst_size - n);
}

/* Convert convertable types.
 * Copied from the gcc:libgfortran/caf/single.c. Can't say much about it. */
static void
convert_type(void *dst, int dst_type, int dst_kind, void *src, int src_type,
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
        int_val = *(int8_t *)src;
      else if (src_kind == 2)
        int_val = *(int16_t *)src;
      else if (src_kind == 4)
        int_val = *(int32_t *)src;
      else if (src_kind == 8)
        int_val = *(int64_t *)src;
#ifdef HAVE_GFC_INTEGER_16
      else if (src_kind == 16)
        int_val = *(int128t *)src;
#endif
      else
        goto error;
      break;
    case BT_REAL:
      if (src_kind == 4)
        real_val = *(float *)src;
      else if (src_kind == 8)
        real_val = *(double *)src;
#ifdef HAVE_GFC_REAL_10
      else if (src_kind == 10)
        real_val = *(long double *)src;
#endif
#ifdef HAVE_GFC_REAL_16
      else if (src_kind == 16)
        real_val = *(real128t *)src;
#endif
      else
        goto error;
      break;
    case BT_COMPLEX:
      if (src_kind == 4)
        cmpx_val = *(_Complex float *)src;
      else if (src_kind == 8)
        cmpx_val = *(_Complex double *)src;
#ifdef HAVE_GFC_REAL_10
      else if (src_kind == 10)
        cmpx_val = *(_Complex long double *)src;
#endif
#ifdef HAVE_GFC_REAL_16
      else if (src_kind == 16)
        cmpx_val = *(complex128t *)src;
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
          *(int8_t *)dst = (int8_t)int_val;
        else if (dst_kind == 2)
          *(int16_t *)dst = (int16_t)int_val;
        else if (dst_kind == 4)
          *(int32_t *)dst = (int32_t)int_val;
        else if (dst_kind == 8)
          *(int64_t *)dst = (int64_t)int_val;
#ifdef HAVE_GFC_INTEGER_16
        else if (dst_kind == 16)
          *(int128t *)dst = (int128t)int_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_REAL)
      {
        if (dst_kind == 1)
          *(int8_t *)dst = (int8_t)real_val;
        else if (dst_kind == 2)
          *(int16_t *)dst = (int16_t)real_val;
        else if (dst_kind == 4)
          *(int32_t *)dst = (int32_t)real_val;
        else if (dst_kind == 8)
          *(int64_t *)dst = (int64_t)real_val;
#ifdef HAVE_GFC_INTEGER_16
        else if (dst_kind == 16)
          *(int128t *)dst = (int128t)real_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_COMPLEX)
      {
        if (dst_kind == 1)
          *(int8_t *)dst = (int8_t)cmpx_val;
        else if (dst_kind == 2)
          *(int16_t *)dst = (int16_t)cmpx_val;
        else if (dst_kind == 4)
          *(int32_t *)dst = (int32_t)cmpx_val;
        else if (dst_kind == 8)
          *(int64_t *)dst = (int64_t)cmpx_val;
#ifdef HAVE_GFC_INTEGER_16
        else if (dst_kind == 16)
          *(int128t *)dst = (int128t)cmpx_val;
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
          *(float *)dst = (float)int_val;
        else if (dst_kind == 8)
          *(double *)dst = (double)int_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(long double *)dst = (long double)int_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(real128t *)dst = (real128t)int_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_REAL)
      {
        if (dst_kind == 4)
          *(float *)dst = (float)real_val;
        else if (dst_kind == 8)
          *(double *)dst = (double)real_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(long double *)dst = (long double)real_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(real128t *)dst = (real128t)real_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_COMPLEX)
      {
        if (dst_kind == 4)
          *(float *)dst = (float)cmpx_val;
        else if (dst_kind == 8)
          *(double *)dst = (double)cmpx_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(long double *)dst = (long double)cmpx_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(real128t *)dst = (real128t)cmpx_val;
#endif
        else
          goto error;
      }
      return;
    case BT_COMPLEX:
      if (src_type == BT_INTEGER)
      {
        if (dst_kind == 4)
          *(_Complex float *)dst = (_Complex float)int_val;
        else if (dst_kind == 8)
          *(_Complex double *)dst = (_Complex double)int_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(_Complex long double *)dst = (_Complex long double)int_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(complex128t *)dst = (complex128t)int_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_REAL)
      {
        if (dst_kind == 4)
          *(_Complex float *)dst = (_Complex float)real_val;
        else if (dst_kind == 8)
          *(_Complex double *)dst = (_Complex double)real_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(_Complex long double *)dst = (_Complex long double)real_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(complex128t *)dst = (complex128t)real_val;
#endif
        else
          goto error;
      }
      else if (src_type == BT_COMPLEX)
      {
        if (dst_kind == 4)
          *(_Complex float *)dst = (_Complex float)cmpx_val;
        else if (dst_kind == 8)
          *(_Complex double *)dst = (_Complex double)cmpx_val;
#ifdef HAVE_GFC_REAL_10
        else if (dst_kind == 10)
          *(_Complex long double *)dst = (_Complex long double)cmpx_val;
#endif
#ifdef HAVE_GFC_REAL_16
        else if (dst_kind == 16)
          *(complex128t *)dst = (complex128t)cmpx_val;
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
  fprintf(stderr,
          "libcaf_mpi RUNTIME ERROR: Cannot convert type %d kind %d "
          "to type %d kind %d\n",
          src_type, src_kind, dst_type, dst_kind);
  if (stat)
    *stat = 1;
  else
    abort();
}

static void
convert_with_strides(void *dst, int dst_type, int dst_kind,
                     ptrdiff_t byte_dst_stride, void *src, int src_type,
                     int src_kind, ptrdiff_t byte_src_stride, size_t num,
                     int *stat)
{
  /* Compute the step from one item to convert to the next in bytes. The stride
   * is expected to be the one or similar to the array.stride, i.e. *_stride is
   * expected to be >= 1 to progress from one item to the next. */
  for (size_t i = 0; i < num; ++i)
  {
    convert_type(dst, dst_type, dst_kind, src, src_type, src_kind, stat);
    dst += byte_dst_stride;
    src += byte_src_stride;
  }
}

static void
copy_char_to_self(void *src, int src_type, int src_size, int src_kind,
                  void *dst, int dst_type, int dst_size, int dst_kind,
                  size_t size, bool src_is_scalar)
{
#ifdef GFC_CAF_CHECK
  if (dst_type != BT_CHARACTER || src_type != BT_CHARACTER)
    caf_runtime_error("internal error: copy_char_to_self() "
                      "for non-char types called.");
#endif
  const size_t dst_len = dst_size / dst_kind, src_len = src_size / src_kind;
  const size_t min_len = (src_len < dst_len) ? src_len : dst_len;
  /* The address of dest passed by the compiler points on the right  memory
   * location. No offset summation is needed. */
  if (dst_kind == src_kind)
  {
    for (size_t c = 0; c < size; ++c)
    {
      memmove(dst, src, min_len * dst_kind);
      /* Fill dest when source is too short. */
      if (dst_len > src_len)
      {
        int32_t *dest_addr = (int32_t *)(dst + dst_kind * src_len);
        const size_t pad_num = dst_len - src_len;
        if (dst_kind == 1)
          memset(dest_addr, ' ', pad_num);
        else if (dst_kind == 4)
        {
          const void *end_addr = &(dest_addr[pad_num]);
          while (dest_addr != end_addr)
            *(dest_addr++) = (int32_t)' ';
        }
        else
          caf_runtime_error(unreachable);
      }
      dst = (void *)((ptrdiff_t)(dst) + dst_size);
      if (!src_is_scalar)
        src = (void *)((ptrdiff_t)(src) + src_size);
    }
  }
  else
  {
    /* Assign using kind-conversion. */
    if (dst_kind == 1 && src_kind == 4)
      for (size_t c = 0; c < size; ++c)
      {
        assign_char1_from_char4(dst_len, src_len, dst, src);
        dst = (void *)((ptrdiff_t)(dst) + dst_size);
        if (!src_is_scalar)
          src = (void *)((ptrdiff_t)(src) + src_size);
      }
    else if (dst_kind == 4 && src_kind == 1)
      for (size_t c = 0; c < size; ++c)
      {
        assign_char4_from_char1(dst_len, src_len, dst, src);
        dst = (void *)((ptrdiff_t)(dst) + dst_size);
        if (!src_is_scalar)
          src = (void *)((ptrdiff_t)(src) + src_size);
      }
    else
      caf_runtime_error("_caf_send(): Unsupported char kinds in same image "
                        "assignment (kind(lhs)= %d, kind(rhs) = %d)",
                        dst_kind, src_kind);
  }
}

static void
copy_to_self(gfc_descriptor_t *src, int src_kind, gfc_descriptor_t *dst,
             int dst_kind, size_t elem_size, int *stat)
{
  const int src_size = GFC_DESCRIPTOR_SIZE(src),
            dst_size = GFC_DESCRIPTOR_SIZE(dst);
  const int src_type = GFC_DESCRIPTOR_TYPE(src),
            dst_type = GFC_DESCRIPTOR_TYPE(dst);
  const int src_rank = GFC_DESCRIPTOR_RANK(src),
            dst_rank = GFC_DESCRIPTOR_RANK(dst);
#ifdef GFC_CAF_CHECK
  if (dst_type == BT_CHARACTER || src_type == BT_CHARACTER)
    caf_runtime_error("internal error: copy_to_self() for char types called.");
#endif
  /* The address of dest passed by the compiler points on the right
   * memory location. No offset summation is needed. Use the convert with
   * strides when src is a scalar. */
  if (dst_kind == src_kind && dst_size == src_size && dst_type == src_type
      && src_rank == dst_rank)
    memmove(dst->base_addr, src->base_addr, elem_size * dst_size);
  else
    /* When the rank is 0 then a scalar is copied to a vector and the stride
     * is zero. */
    convert_with_strides(dst->base_addr, dst_type, dst_kind, dst_size,
                         src->base_addr, src_type, src_kind,
                         src_rank > 0 ? src_size : 0, elem_size, stat);
}

/* token: The token of the array to be written to.
 * offset: Difference between the coarray base address and the actual data,
 * used for caf(3)[2] = 8 or caf[4]%a(4)%b = 7.
 * image_index: Index of the coarray (typically remote,
 * though it can also be on this_image).
 * data: Pointer to the to-be-transferred data.
 * size: The number of bytes to be transferred.
 * asynchronous: Return before the data transfer has been complete */

void
selectType(int size, MPI_Datatype *dt)
{
  int t_s;

#define SELTYPE(type)                                                          \
  MPI_Type_size(type, &t_s);                                                   \
  if (t_s == size)                                                             \
  {                                                                            \
    *dt = type;                                                                \
    return;                                                                    \
  }

  SELTYPE(MPI_BYTE)
  SELTYPE(MPI_SHORT)
  SELTYPE(MPI_INT)
  SELTYPE(MPI_DOUBLE)
  SELTYPE(MPI_COMPLEX)
  SELTYPE(MPI_DOUBLE_COMPLEX)

#undef SELTYPE
}

void
PREFIX(sendget)(caf_token_t token_s, size_t offset_s, int image_index_s,
                gfc_descriptor_t *dest, caf_vector_t *dst_vector,
                caf_token_t token_g, size_t offset_g, int image_index_g,
                gfc_descriptor_t *src, caf_vector_t *src_vector, int dst_kind,
                int src_kind, bool mrt, int *pstat)
{
  int j, ierr = 0;
  size_t i, size;
  ptrdiff_t dimextent;
  const int src_rank = GFC_DESCRIPTOR_RANK(src),
            dst_rank = GFC_DESCRIPTOR_RANK(dest);
  const size_t src_size = GFC_DESCRIPTOR_SIZE(src),
               dst_size = GFC_DESCRIPTOR_SIZE(dest);
  const int src_type = GFC_DESCRIPTOR_TYPE(src),
            dst_type = GFC_DESCRIPTOR_TYPE(dest);
  const bool src_contiguous = PREFIX(is_contiguous)(src),
             dst_contiguous = PREFIX(is_contiguous)(dest);
  const bool src_same_image = caf_this_image == image_index_g,
             dst_same_image = caf_this_image == image_index_s,
             same_type_and_kind = dst_type == src_type && dst_kind == src_kind;

  MPI_Win *p = TOKEN(token_g);
  ptrdiff_t src_offset = 0, dst_offset = 0;
  void *pad_str = NULL;
  bool free_pad_str = false;
  void *src_t_buff = NULL, *dst_t_buff = NULL;
  bool free_src_t_buff = false, free_dst_t_buff = false;
  const bool dest_char_array_is_longer
      = dst_type == BT_CHARACTER && dst_size > src_size;
  int src_remote_image = image_index_g - 1,
      dst_remote_image = image_index_s - 1;

  if (!src_same_image)
  {
    MPI_Group current_team_group, win_group;
    ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
    chk_err(ierr);
    ierr = MPI_Win_get_group(*p, &win_group);
    chk_err(ierr);
    ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                     (int[]){src_remote_image}, win_group,
                                     &src_remote_image);
    chk_err(ierr);
    ierr = MPI_Group_free(&current_team_group);
    chk_err(ierr);
    ierr = MPI_Group_free(&win_group);
    chk_err(ierr);
  }
  if (!dst_same_image)
  {
    MPI_Group current_team_group, win_group;
    ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
    chk_err(ierr);
    ierr = MPI_Win_get_group(*p, &win_group);
    chk_err(ierr);
    ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                     (int[]){dst_remote_image}, win_group,
                                     &dst_remote_image);
    chk_err(ierr);
    ierr = MPI_Group_free(&current_team_group);
    chk_err(ierr);
    ierr = MPI_Group_free(&win_group);
    chk_err(ierr);
  }

  /* Ensure stat is always set. */
#ifdef GCC_GE_7
  int *stat = pstat;
  if (stat)
    *stat = 0;
#else
  /* Gcc prior to 7.0 does not have stat here. */
  int *stat = NULL;
#endif

  size = 1;
  for (j = 0; j < dst_rank; ++j)
  {
    dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
    if (dimextent < 0)
      dimextent = 0;
    size *= dimextent;
  }

  if (size == 0)
    return;

  dprint("src_vector = %p, dst_vector = %p, src_image_index = %d, "
         "dst_image_index = %d, offset_src = %zd, offset_dst = %zd.\n",
         src_vector, dst_vector, image_index_g, image_index_s, offset_g,
         offset_s);
  check_image_health(image_index_g, stat);
  check_image_health(image_index_s, stat);

  /* For char arrays: create the padding array, when dst is longer than src. */
  if (dest_char_array_is_longer)
  {
    const size_t pad_num = (dst_size / dst_kind) - (src_size / src_kind);
    const size_t pad_sz = pad_num * dst_kind;
    /* For big arrays alloca() may not be able to get the memory on the stack.
     * Use a regular malloc then. */
    if ((free_pad_str = ((pad_str = alloca(pad_sz)) == NULL)))
    {
      pad_str = malloc(pad_sz);
      if (src_t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in sendget().");
    }
    if (dst_kind == 1)
    {
      memset(pad_str, ' ', pad_num);
    }
    else /* dst_kind == 4. */
    {
      for (int32_t *it = (int32_t *)pad_str,
                   *itEnd = ((int32_t *)pad_str) + pad_num;
           it < itEnd; ++it)
      {
        *it = (int32_t)' ';
      }
    }
  }

  if (src_contiguous && src_vector == NULL)
  {
    if (src_same_image)
    {
      dprint("in caf_this == image_index, size = %zd, "
             "dst_kind = %d, src_kind = %d, dst_size = %zd, src_size = %zd\n",
             size, dst_kind, src_kind, dst_size, src_size);
      src_t_buff = src->base_addr;
      if (same_type_and_kind && !dest_char_array_is_longer)
      {
        dst_t_buff = src_t_buff;
      }
      else
      {
        dprint("allocating %zd bytes for dst_t_buff.\n", dst_size * size);
        if ((free_dst_t_buff
             = ((dst_t_buff = alloca(dst_size * size)) == NULL)))
        {
          dst_t_buff = malloc(dst_size * size);
          if (dst_t_buff == NULL)
            caf_runtime_error("Unable to allocate memory "
                              "for internal buffer in sendget().");
        }
        if (dst_type == BT_CHARACTER)
        {
          /* The size is encoded in the descriptor's type for char arrays. */
          copy_char_to_self(src->base_addr, src_type, src_size, src_kind,
                            dst_t_buff, dst_type, dst_size, dst_kind, size,
                            src_rank == 0);
        }
        else
        {
          convert_with_strides(dst_t_buff, dst_type, dst_kind, dst_size,
                               src->base_addr, src_type, src_kind,
                               (src_rank > 0) ? src_size : 0, size, stat);
        }
      }
    }
    else
    {
      /* When replication is needed, only access the scalar on the remote. */
      const size_t src_real_size = src_rank > 0 ? (src_size * size) : src_size;
      if ((free_dst_t_buff = ((dst_t_buff = alloca(dst_size * size)) == NULL)))
      {
        dst_t_buff = malloc(dst_size * size);
        if (dst_t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in sendget().");
      }

      if (dst_kind != src_kind || src_rank == 0 || dest_char_array_is_longer)
      {
        if ((free_src_t_buff
             = ((src_t_buff = alloca(src_size * size)) == NULL)))
        {
          src_t_buff = malloc(src_size * size);
          if (src_t_buff == NULL)
            caf_runtime_error("Unable to allocate memory "
                              "for internal buffer in sendget().");
        }
      }
      else
        src_t_buff = dst_t_buff;

      CAF_Win_lock(MPI_LOCK_SHARED, src_remote_image, *p);
      if ((same_type_and_kind && dst_rank == src_rank)
          || dst_type == BT_CHARACTER)
      {
        if (!dest_char_array_is_longer
            && (dst_kind == src_kind || dst_type != BT_CHARACTER))
        {
          const size_t trans_size
              = ((dst_size > src_size) ? src_size : dst_size) * size;
          ierr = MPI_Get(dst_t_buff, trans_size, MPI_BYTE, src_remote_image,
                         offset_g, trans_size, MPI_BYTE, *p);
          chk_err(ierr);
        }
        else
        {
          ierr = MPI_Get(src_t_buff, src_real_size, MPI_BYTE, src_remote_image,
                         offset_g, src_real_size, MPI_BYTE, *p);
          chk_err(ierr);
          dprint("copy_char_to_self(src_size = %zd, src_kind = %d, "
                 "dst_size = %zd, dst_kind = %d, size = %zd)\n",
                 src_size, src_kind, dst_size, dst_kind, size);
          copy_char_to_self(src_t_buff, src_type, src_size, src_kind,
                            dst_t_buff, dst_type, dst_size, dst_kind, size,
                            src_rank == 0);
          dprint("|%s|\n", (char *)dst_t_buff);
        }
      }
      else
      {
        ierr = MPI_Get(src_t_buff, src_real_size, MPI_BYTE, src_remote_image,
                       offset_g, src_real_size, MPI_BYTE, *p);
        chk_err(ierr);
        convert_with_strides(dst_t_buff, dst_type, dst_kind, dst_size,
                             src_t_buff, src_type, src_kind,
                             (src_rank > 0) ? src_size : 0, size, stat);
      }
      CAF_Win_unlock(src_remote_image, *p);
    }
  }
#ifdef STRIDED
  else if (!src_same_image && same_type_and_kind && dst_type != BT_CHARACTER)
  {
    /* For strided copy, no type and kind conversion, copy to self or
     * character arrays are supported. */
    MPI_Datatype dt_s, dt_d, base_type_src, base_type_dst;
    int *arr_bl;
    int *arr_dsp_s;

    if ((free_dst_t_buff = ((dst_t_buff = alloca(dst_size * size)) == NULL)))
    {
      dst_t_buff = malloc(dst_size * size);
      if (dst_t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in sendget().");
    }

    selectType(src_size, &base_type_src);
    selectType(dst_size, &base_type_dst);

    if (src_rank == 1)
    {
      if (src_vector == NULL)
      {
        ierr = MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src,
                               &dt_s);
        chk_err(ierr);
      }
      else
      {
        arr_bl = calloc(size, sizeof(int));
        arr_dsp_s = calloc(size, sizeof(int));

        dprint("Setting up strided vector index.\n");
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    for (i = 0; i < size; ++i)                                                 \
    {                                                                          \
      arr_dsp_s[i] = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]           \
                      - src->dim[0].lower_bound);                              \
      arr_bl[i] = 1;                                                           \
    }                                                                          \
    break

        switch (src_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
#undef KINDCASE
        ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
        chk_err(ierr);
        free(arr_bl);
        free(arr_dsp_s);
      }
      ierr = MPI_Type_vector(size, 1, 1, base_type_dst, &dt_d);
      chk_err(ierr);
    }
    else
    {
      arr_bl = calloc(size, sizeof(int));
      arr_dsp_s = calloc(size, sizeof(int));

      for (i = 0; i < size; ++i)
      {
        arr_bl[i] = 1;
      }

      for (i = 0; i < size; ++i)
      {
        ptrdiff_t array_offset_sr = 0, extent = 1, tot_ext = 1;
        if (src_vector == NULL)
        {
          for (j = 0; j < src_rank - 1; ++j)
          {
            extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
            array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_sr += (i / tot_ext) * src->dim[src_rank - 1]._stride;
        }
        else
        {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_sr = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]          \
                       - src->dim[0].lower_bound);                             \
    break
          switch (src_vector->u.v.kind)
          {
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
#undef KINDCASE
        }
        arr_dsp_s[i] = array_offset_sr;
      }

      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
      chk_err(ierr);
      ierr = MPI_Type_vector(size, 1, 1, base_type_dst, &dt_d);
      chk_err(ierr);

      free(arr_bl);
      free(arr_dsp_s);
    }

    ierr = MPI_Type_commit(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_commit(&dt_d);
    chk_err(ierr);

    CAF_Win_lock(MPI_LOCK_SHARED, src_remote_image, *p);
    ierr
        = MPI_Get(dst_t_buff, 1, dt_d, src_remote_image, offset_g, 1, dt_s, *p);
    chk_err(ierr);
    CAF_Win_unlock(src_remote_image, *p);

#ifdef WITH_FAILED_IMAGES
    check_image_health(image_index_g, stat);

    if (!stat && ierr == STAT_FAILED_IMAGE)
      terminate_internal(ierr, 1);

    if (stat)
      *stat = ierr;
#else
    if (ierr != 0)
    {
      terminate_internal(ierr, 1);
      return;
    }
#endif
    ierr = MPI_Type_free(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_free(&dt_d);
    chk_err(ierr);
  }
#endif // STRIDED
  else
  {
    if ((free_dst_t_buff = ((dst_t_buff = alloca(dst_size * size)) == NULL)))
    {
      dst_t_buff = malloc(dst_size * size);
      if (dst_t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in sendget().");
    }

    if (src_same_image)
      src_t_buff = src->base_addr;
    else if (!same_type_and_kind)
    {
      if ((free_src_t_buff = (((src_t_buff = alloca(src_size))) == NULL)))
      {
        src_t_buff = malloc(src_size);
        if (src_t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in sendget().");
      }
    }

    if (!src_same_image)
      CAF_Win_lock(MPI_LOCK_SHARED, src_remote_image, *p);
    for (i = 0; i < size; ++i)
    {
      ptrdiff_t array_offset_sr = 0, extent = 1, tot_ext = 1;
      if (src_vector == NULL)
      {
        for (j = 0; j < src_rank - 1; ++j)
        {
          extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
          array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
          tot_ext *= extent;
        }

        array_offset_sr += (i / tot_ext) * src->dim[src_rank - 1]._stride;
      }
      else
      {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_sr = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]          \
                       - src->dim[0].lower_bound);                             \
    break
        switch (src_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
      }
#undef KINDCASE

      src_offset = array_offset_sr * src_size;
      void *dst = (void *)((char *)dst_t_buff + i * dst_size);

      if (!src_same_image)
      {
        // Do the more likely first.
        dprint("kind(dst) = %d, el_sz(dst) = %zd, "
               "kind(src) = %d, el_sz(src) = %zd, lb(dst) = %zd.\n",
               dst_kind, dst_size, src_kind, src_size, src->dim[0].lower_bound);
        if (same_type_and_kind)
        {
          const size_t trans_size = (src_size < dst_size) ? src_size : dst_size;
          ierr = MPI_Get(dst, trans_size, MPI_BYTE, src_remote_image,
                         offset_g + src_offset, trans_size, MPI_BYTE, *p);
          chk_err(ierr);
          if (pad_str)
            memcpy((void *)((char *)dst + src_size), pad_str,
                   dst_size - src_size);
        }
        else if (dst_type == BT_CHARACTER)
        {
          ierr = MPI_Get(src_t_buff, src_size, MPI_BYTE, src_remote_image,
                         offset_g + src_offset, src_size, MPI_BYTE, *p);
          chk_err(ierr);
          copy_char_to_self(src_t_buff, src_type, src_size, src_kind, dst,
                            dst_type, dst_size, dst_kind, 1, true);
        }
        else
        {
          ierr = MPI_Get(src_t_buff, src_size, MPI_BYTE, src_remote_image,
                         offset_g + src_offset, src_size, MPI_BYTE, *p);
          chk_err(ierr);
          convert_type(dst, dst_type, dst_kind, src_t_buff, src_type, src_kind,
                       stat);
        }
      }
      else
      {
        if (!mrt)
        {
          dprint("strided same_image, no temp,  for i = %zd, "
                 "src_offset = %zd, offset = %zd.\n",
                 i, src_offset, offset_g);
          if (same_type_and_kind)
            memmove(dst, src->base_addr + src_offset, src_size);
          else
            convert_type(dst, dst_type, dst_kind, src->base_addr + src_offset,
                         src_type, src_kind, stat);
        }
        else
        {
          dprint("strided same_image, *WITH* temp, for i = %zd.\n", i);
          if (same_type_and_kind)
            memmove(dst, src->base_addr + src_offset, src_size);
          else
            convert_type(dst, dst_type, dst_kind, src->base_addr + src_offset,
                         src_type, src_kind, stat);
        }
      }

#ifndef WITH_FAILED_IMAGES
      if (ierr != 0)
      {
        caf_runtime_error("MPI Error: %d", ierr);
        return;
      }
#endif
    }
    if (!src_same_image)
      CAF_Win_unlock(src_remote_image, *p);
  }

  p = TOKEN(token_s);
  /* Now transfer data to the remote dest. */
  if (dst_contiguous && dst_vector == NULL)
  {
    if (dst_same_image)
      memmove(dest->base_addr, dst_t_buff, dst_size * size);
    else
    {
      CAF_Win_lock(MPI_LOCK_EXCLUSIVE, dst_remote_image, *p);
      const size_t trans_size = size * dst_size;
      ierr = MPI_Put(dst_t_buff, trans_size, MPI_BYTE, dst_remote_image,
                     offset_s, trans_size, MPI_BYTE, *p);
      chk_err(ierr);
      ierr = CAF_Win_unlock(dst_remote_image, *p);
      chk_err(ierr);
#if NONBLOCKING_PUT
      /* Pending puts init */
      if (pending_puts == NULL)
      {
        pending_puts = calloc(1, sizeof(win_sync));
        pending_puts->next = NULL;
        pending_puts->win = token_s;
        pending_puts->img = dst_remote_image;
        last_elem = pending_puts;
        last_elem->next = NULL;
      }
      else
      {
        last_elem->next = calloc(1, sizeof(win_sync));
        last_elem = last_elem->next;
        last_elem->win = token_s;
        last_elem->img = dst_remote_image;
        last_elem->next = NULL;
      }
#endif // CAF_MPI_LOCK_UNLOCK
    }
  }
#ifdef STRIDED
  else if (!dst_same_image && same_type_and_kind && dst_type != BT_CHARACTER)
  {
    /* For strided copy, no type and kind conversion, copy to self or
     * character arrays are supported. */
    MPI_Datatype dt_s, dt_d, base_type_dst;
    int *arr_bl, *arr_dsp_d;

    selectType(dst_size, &base_type_dst);

    if (dst_rank == 1)
    {
      if (dst_vector == NULL)
      {
        ierr = MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst,
                               &dt_d);
        chk_err(ierr);
      }
      else
      {
        arr_bl = calloc(size, sizeof(int));
        arr_dsp_d = calloc(size, sizeof(int));

        dprint("Setting up strided vector index.\n");
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    for (i = 0; i < size; ++i)                                                 \
    {                                                                          \
      arr_dsp_d[i] = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]           \
                      - dest->dim[0].lower_bound);                             \
      arr_bl[i] = 1;                                                           \
    }                                                                          \
    break
        switch (dst_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
#undef KINDCASE
        ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
        chk_err(ierr);
        free(arr_bl);
        free(arr_dsp_d);
      }
      ierr = MPI_Type_vector(size, 1, 1, base_type_dst, &dt_s);
      chk_err(ierr);
    }
    else
    {
      arr_bl = calloc(size, sizeof(int));
      arr_dsp_d = calloc(size, sizeof(int));

      for (i = 0; i < size; ++i)
      {
        arr_bl[i] = 1;
      }

      for (i = 0; i < size; ++i)
      {
        ptrdiff_t array_offset_dst = 0, extent = 1, tot_ext = 1;
        if (dst_vector == NULL)
        {
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
        }
        else
        {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]         \
                        - dest->dim[0].lower_bound);                           \
    break
          switch (dst_vector->u.v.kind)
          {
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
#undef KINDCASE
        }
        arr_dsp_d[i] = array_offset_dst;
      }

      ierr = MPI_Type_vector(size, 1, 1, base_type_dst, &dt_s);
      chk_err(ierr);
      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
      chk_err(ierr);

      free(arr_bl);
      free(arr_dsp_d);
    }

    ierr = MPI_Type_commit(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_commit(&dt_d);
    chk_err(ierr);

    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, dst_remote_image, *p);
    ierr
        = MPI_Put(dst_t_buff, 1, dt_s, dst_remote_image, offset_s, 1, dt_d, *p);
    chk_err(ierr);
    CAF_Win_unlock(dst_remote_image, *p);

#ifdef WITH_FAILED_IMAGES
    check_image_health(image_index_s, stat);

    if (!stat && ierr == STAT_FAILED_IMAGE)
      terminate_internal(ierr, 1);

    if (stat)
      *stat = ierr;
#else
    if (ierr != 0)
    {
      terminate_internal(ierr, 1);
      return;
    }
#endif
    ierr = MPI_Type_free(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_free(&dt_d);
    chk_err(ierr);
  }
#endif // STRIDED
  else
  {
    if (!dst_same_image)
      CAF_Win_lock(MPI_LOCK_EXCLUSIVE, dst_remote_image, *p);
    for (i = 0; i < size; ++i)
    {
      ptrdiff_t array_offset_dst = 0, extent = 1, tot_ext = 1;
      if (dst_vector == NULL)
      {
        for (j = 0; j < dst_rank - 1; ++j)
        {
          extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
          array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
          tot_ext *= extent;
        }

        array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
      }
      else
      {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]         \
                        - dest->dim[0].lower_bound);                           \
    break
        switch (dst_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
      }
#undef KINDCASE
      dst_offset = array_offset_dst * dst_size;

      void *sr = (void *)((char *)dst_t_buff + i * dst_size);

      if (!dst_same_image)
      {
        // Do the more likely first.
        ierr = MPI_Put(sr, dst_size, MPI_BYTE, dst_remote_image,
                       offset_s + dst_offset, dst_size, MPI_BYTE, *p);
        chk_err(ierr);
      }
      else
        memmove(dest->base_addr + dst_offset, sr, dst_size);

#ifndef WITH_FAILED_IMAGES
      if (ierr != 0)
      {
        caf_runtime_error("MPI Error: %d", ierr);
        return;
      }
#endif
    } /* for */
    if (!dst_same_image)
      CAF_Win_unlock(dst_remote_image, *p);
  }

  /* Free memory, when not allocated on stack. */
  if (free_src_t_buff)
    free(src_t_buff);
  if (free_dst_t_buff)
    free(dst_t_buff);
  if (free_pad_str)
    free(pad_str);

#ifdef WITH_FAILED_IMAGES
  /* Catch failed images, when failed image support is active. */
  check_image_health(image_index_g, stat);
  check_image_health(image_index_s, stat);
#endif

  if (ierr != MPI_SUCCESS)
  {
    int mpi_error;
    MPI_Error_class(ierr, &mpi_error);
    if (stat)
      *stat = mpi_error;
    else
    {
      int error_len = 2048;
      char error_str[error_len];
      strcpy(error_str, "MPI-error: ");
      MPI_Error_string(mpi_error, &error_str[11], &error_len);
      error_stop_str(error_str, error_len + 11, false);
    }
  }
}

/* Send array data from src to dest on a remote image.
 * The argument mrt means may_require_temporary */

void
PREFIX(send)(caf_token_t token, size_t offset, int image_index,
             gfc_descriptor_t *dest, caf_vector_t *dst_vector,
             gfc_descriptor_t *src, int dst_kind, int src_kind, bool mrt,
             int *pstat)
{
  int j, ierr = 0;
  size_t i, size;
  ptrdiff_t dimextent;
  const int src_rank = GFC_DESCRIPTOR_RANK(src),
            dst_rank = GFC_DESCRIPTOR_RANK(dest);
  const size_t src_size = GFC_DESCRIPTOR_SIZE(src),
               dst_size = GFC_DESCRIPTOR_SIZE(dest);
  const int src_type = GFC_DESCRIPTOR_TYPE(src),
            dst_type = GFC_DESCRIPTOR_TYPE(dest);
  const bool src_contiguous = PREFIX(is_contiguous)(src),
             dst_contiguous = PREFIX(is_contiguous)(dest);
  const bool same_image = caf_this_image == image_index,
             same_type_and_kind = dst_type == src_type && dst_kind == src_kind;

  MPI_Win *p = TOKEN(token);
  ptrdiff_t dst_offset = 0;
  void *pad_str = NULL, *t_buff = NULL;
  bool free_pad_str = false, free_t_buff = false;
  const bool dest_char_array_is_longer
      = dst_type == BT_CHARACTER && dst_size > src_size && !same_image;
  int remote_image = image_index - 1;
  if (!same_image)
  {
    MPI_Group current_team_group, win_group;
    ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
    chk_err(ierr);
    ierr = MPI_Win_get_group(*p, &win_group);
    chk_err(ierr);
    ierr = MPI_Group_translate_ranks(
        current_team_group, 1, (int[]){remote_image}, win_group, &remote_image);
    chk_err(ierr);
    ierr = MPI_Group_free(&current_team_group);
    chk_err(ierr);
    ierr = MPI_Group_free(&win_group);
    chk_err(ierr);
  }

  /* Ensure stat is always set. */
#ifdef GCC_GE_7
  int *stat = pstat;
  if (stat)
    *stat = 0;
#else
  /* Gcc prior to 7.0 does not have stat here. */
  int *stat = NULL;
#endif

  size = 1;
  for (j = 0; j < dst_rank; ++j)
  {
    dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
    if (dimextent < 0)
      dimextent = 0;
    size *= dimextent;
  }

  if (size == 0)
    return;

  dprint("dst_vector = %p, image_index = %d, offset = %zd.\n", dst_vector,
         image_index, offset);
  check_image_health(image_index, stat);

  /* For char arrays: create the padding array, when dst is longer than src. */
  if (dest_char_array_is_longer)
  {
    const size_t pad_num = (dst_size / dst_kind) - (src_size / src_kind);
    const size_t pad_sz = pad_num * dst_kind;
    /* For big arrays alloca() may not be able to get the memory on the stack.
     * Use a regular malloc then. */
    if ((free_pad_str = ((pad_str = alloca(pad_sz)) == NULL)))
    {
      pad_str = malloc(pad_sz);
      if (t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in send().");
    }
    if (dst_kind == 1)
      memset(pad_str, ' ', pad_num);
    else /* dst_kind == 4. */
    {
      for (int32_t *it = (int32_t *)pad_str,
                   *itEnd = ((int32_t *)pad_str) + pad_num;
           it < itEnd; ++it)
      {
        *it = (int32_t)' ';
      }
    }
  }

  if (src_contiguous && dst_contiguous && dst_vector == NULL)
  {
    if (same_image)
    {
      dprint("in caf_this == image_index, size = %zd, dst_kind = %d, "
             "src_kind = %d\n",
             size, dst_kind, src_kind);
      if (dst_type == BT_CHARACTER)
        /* The size is encoded in the descriptor's type for char arrays. */
        copy_char_to_self(src->base_addr, src_type, src_size, src_kind,
                          dest->base_addr, dst_type, dst_size, dst_kind, size,
                          src_rank == 0);
      else
        copy_to_self(src, src_kind, dest, dst_kind, size, stat);
      return;
    }
    else
    {
      if (dst_kind != src_kind || dest_char_array_is_longer || src_rank == 0)
      {
        if ((free_t_buff = ((t_buff = alloca(dst_size * size)) == NULL)))
        {
          t_buff = malloc(dst_size * size);
          if (t_buff == NULL)
            caf_runtime_error("Unable to allocate memory "
                              "for internal buffer in send().");
        }
      }

      if ((same_type_and_kind && dst_rank == src_rank)
          || dst_type == BT_CHARACTER)
      {
        if (dest_char_array_is_longer
            || (dst_kind != src_kind && dst_type == BT_CHARACTER))
        {
          copy_char_to_self(src->base_addr, src_type, src_size, src_kind,
                            t_buff, dst_type, dst_size, dst_kind, size,
                            src_rank == 0);
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
          ierr = MPI_Put(t_buff, dst_size, MPI_BYTE, remote_image, offset,
                         dst_size, MPI_BYTE, *p);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, *p);
        }
        else
        {
          const size_t trans_size
              = ((dst_size > src_size) ? src_size : dst_size) * size;
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
          ierr = MPI_Put(src->base_addr, trans_size, MPI_BYTE, remote_image,
                         offset, trans_size, MPI_BYTE, *p);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, *p);
        }
      }
      else
      {
        convert_with_strides(t_buff, dst_type, dst_kind, dst_size,
                             src->base_addr, src_type, src_kind,
                             (src_rank > 0) ? src_size : 0, size, stat);
        CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
        ierr = MPI_Put(t_buff, dst_size * size, MPI_BYTE, remote_image, offset,
                       dst_size * size, MPI_BYTE, *p);
        chk_err(ierr);
        ierr = CAF_Win_unlock(remote_image, *p);
        chk_err(ierr);
      }
#if NONBLOCKING_PUT
      /* Pending puts init */
      if (pending_puts == NULL)
      {
        pending_puts = calloc(1, sizeof(win_sync));
        pending_puts->next = NULL;
        pending_puts->win = token;
        pending_puts->img = remote_image;
        last_elem = pending_puts;
        last_elem->next = NULL;
      }
      else
      {
        last_elem->next = calloc(1, sizeof(win_sync));
        last_elem = last_elem->next;
        last_elem->win = token;
        last_elem->img = remote_image;
        last_elem->next = NULL;
      }
#endif // CAF_MPI_LOCK_UNLOCK
    }
  }

#ifdef STRIDED
  else if (!same_image && same_type_and_kind && dst_type != BT_CHARACTER)
  {
    /* For strided copy, no type and kind conversion, copy to self or
     * character arrays are supported. */
    MPI_Datatype dt_s, dt_d, base_type_src, base_type_dst;
    int *arr_bl, *arr_dsp_s, *arr_dsp_d;

    selectType(src_size, &base_type_src);
    selectType(dst_size, &base_type_dst);

    if (dst_rank == 1)
    {
      if (dst_vector == NULL)
      {
        dprint("Setting up mpi datatype vector with "
               "stride %d, size %d and offset %d.\n",
               dest->dim[0]._stride, size, offset);
        ierr = MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst,
                               &dt_d);
        chk_err(ierr);
      }
      else
      {
        arr_bl = calloc(size, sizeof(int));
        arr_dsp_d = calloc(size, sizeof(int));

        dprint("Setting up strided vector index.\n");
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    for (i = 0; i < size; ++i)                                                 \
    {                                                                          \
      arr_dsp_d[i] = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]           \
                      - dest->dim[0].lower_bound);                             \
      arr_bl[i] = 1;                                                           \
    }                                                                          \
    break
        switch (dst_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
#undef KINDCASE
        ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
        chk_err(ierr);
        free(arr_bl);
        free(arr_dsp_d);
      }
      MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src, &dt_s);
    }
    else
    {
      arr_bl = calloc(size, sizeof(int));
      arr_dsp_s = calloc(size, sizeof(int));
      arr_dsp_d = calloc(size, sizeof(int));

      for (i = 0; i < size; ++i)
      {
        arr_bl[i] = 1;
      }

      for (i = 0; i < size; ++i)
      {
        ptrdiff_t array_offset_dst = 0, extent = 1, tot_ext = 1;
        if (dst_vector == NULL)
        {
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
        }
        else
        {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]         \
                        - dest->dim[0].lower_bound);                           \
    break
          switch (dst_vector->u.v.kind)
          {
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
#undef KINDCASE
        }
        arr_dsp_d[i] = array_offset_dst;

        if (src_rank != 0)
        {
          ptrdiff_t array_offset_sr = 0;
          extent = 1;
          tot_ext = 1;
          for (j = 0; j < src_rank - 1; ++j)
          {
            extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
            array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_sr += (i / tot_ext) * src->dim[src_rank - 1]._stride;
          arr_dsp_s[i] = array_offset_sr;
        }
        else
          arr_dsp_s[i] = 0;
      }

      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
      chk_err(ierr);
      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
      chk_err(ierr);

      free(arr_bl);
      free(arr_dsp_s);
      free(arr_dsp_d);
    }

    ierr = MPI_Type_commit(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_commit(&dt_d);
    chk_err(ierr);

    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
    ierr = MPI_Put(src->base_addr, 1, dt_s, remote_image, offset, 1, dt_d, *p);
    chk_err(ierr);
    CAF_Win_unlock(remote_image, *p);

#ifdef WITH_FAILED_IMAGES
    check_image_health(image_index, stat);

    if (!stat && ierr == STAT_FAILED_IMAGE)
      terminate_internal(ierr, 1);

    if (stat)
      *stat = ierr;
#else
    if (ierr != 0)
    {
      terminate_internal(ierr, 1);
      return;
    }
#endif
    ierr = MPI_Type_free(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_free(&dt_d);
    chk_err(ierr);
  }
#endif // STRIDED
  else
  {
    if (same_image && mrt)
    {
      if ((free_t_buff = (((t_buff = alloca(dst_size * size))) == NULL)))
      {
        t_buff = malloc(dst_size * size);
        if (t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in send().");
      }
    }
    else if (!same_type_and_kind && !same_image)
    {
      if ((free_t_buff = (((t_buff = alloca(dst_size))) == NULL)))
      {
        t_buff = malloc(dst_size);
        if (t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in send().");
      }
    }

    for (i = 0; i < size; ++i)
    {
      ptrdiff_t array_offset_dst = 0, extent = 1, tot_ext = 1;
      if (!same_image || !mrt)
      {
        /* For same image and may require temp, the dst_offset is
         * computed on storage. */
        if (dst_vector == NULL)
        {
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }
          array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
        }
        else
        {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst = ((ptrdiff_t)((type *)dst_vector->u.v.vector)[i]         \
                        - dest->dim[0].lower_bound);                           \
    break
          switch (dst_vector->u.v.kind)
          {
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
        }
        dst_offset = array_offset_dst * dst_size;
      }

      void *sr;
      if (src_rank != 0)
      {
        ptrdiff_t array_offset_sr = 0;
        extent = 1;
        tot_ext = 1;
        for (j = 0; j < src_rank - 1; ++j)
        {
          extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
          array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
          tot_ext *= extent;
        }

        array_offset_sr += (i / tot_ext) * src->dim[dst_rank - 1]._stride;
        sr = (void *)((char *)src->base_addr + array_offset_sr * src_size);
      }
      else
        sr = src->base_addr;

      if (!same_image)
      {
        // Do the more likely first.
        dprint("kind(dst) = %d, el_sz(dst) = %zd, "
               "kind(src) = %d, el_sz(src) = %zd, lb(dst) = %zd.\n",
               dst_kind, dst_size, src_kind, src_size,
               dest->dim[0].lower_bound);
        if (same_type_and_kind)
        {
          const size_t trans_size = (src_size < dst_size) ? src_size : dst_size;
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
          ierr = MPI_Put(sr, trans_size, MPI_BYTE, remote_image,
                         offset + dst_offset, trans_size, MPI_BYTE, *p);
          chk_err(ierr);
          if (pad_str)
          {
            ierr = MPI_Put(pad_str, dst_size - src_size, MPI_BYTE, remote_image,
                           offset + dst_offset + src_size, dst_size - src_size,
                           MPI_BYTE, *p);
            chk_err(ierr);
          }
          CAF_Win_unlock(remote_image, *p);
        }
        else if (dst_type == BT_CHARACTER)
        {
          copy_char_to_self(sr, src_type, src_size, src_kind, t_buff, dst_type,
                            dst_size, dst_kind, 1, true);
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
          ierr = MPI_Put(t_buff, dst_size, MPI_BYTE, remote_image,
                         offset + dst_offset, dst_size, MPI_BYTE, *p);
          CAF_Win_unlock(remote_image, *p);
          chk_err(ierr);
        }
        else
        {
          convert_type(t_buff, dst_type, dst_kind, sr, src_type, src_kind,
                       stat);
          CAF_Win_lock(MPI_LOCK_EXCLUSIVE, remote_image, *p);
          ierr = MPI_Put(t_buff, dst_size, MPI_BYTE, remote_image,
                         offset + dst_offset, dst_size, MPI_BYTE, *p);
          CAF_Win_unlock(remote_image, *p);
          chk_err(ierr);
        }
      }
      else
      {
        if (!mrt)
        {
          dprint("strided same_image, no temp, for i = %zd, "
                 "dst_offset = %zd, offset = %zd.\n",
                 i, dst_offset, offset);
          if (same_type_and_kind)
            memmove(dest->base_addr + dst_offset, sr, src_size);
          else
            convert_type(dest->base_addr + dst_offset, dst_type, dst_kind, sr,
                         src_type, src_kind, stat);
        }
        else
        {
          dprint("strided same_image, *WITH* temp, for i = %zd.\n", i);
          if (same_type_and_kind)
            memmove(t_buff + i * dst_size, sr, src_size);
          else
            convert_type(t_buff + i * dst_size, dst_type, dst_kind, sr,
                         src_type, src_kind, stat);
        }
      }

#ifndef WITH_FAILED_IMAGES
      if (ierr != 0)
      {
        caf_runtime_error("MPI Error: %d", ierr);
        return;
      }
#endif
    }

    if (same_image && mrt)
    {
      for (i = 0; i < size; ++i)
      {
        ptrdiff_t array_offset_dst = 0, extent = 1, tot_ext = 1;
        if (dst_vector == NULL)
        {
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
        }
        else
        {
          switch (dst_vector->u.v.kind)
          {
            // KINDCASE is defined above.
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
#undef KINDCASE
        }
        dst_offset = array_offset_dst * dst_size;
        memmove(dest->base_addr + dst_offset, t_buff + i * dst_size, dst_size);
      }
    }
  }

  /* Free memory, when not allocated on stack. */
  if (free_t_buff)
    free(t_buff);
  if (free_pad_str)
    free(pad_str);

#ifdef WITH_FAILED_IMAGES
  /* Catch failed images, when failed image support is active. */
  check_image_health(image_index, stat);
#endif

  if (ierr != MPI_SUCCESS)
  {
    int mpi_error;
    MPI_Error_class(ierr, &mpi_error);
    if (stat)
      *stat = mpi_error;
    else
    {
      int error_len = 2048;
      char error_str[error_len];
      strcpy(error_str, "MPI-error: ");
      MPI_Error_string(mpi_error, &error_str[11], &error_len);
      error_stop_str(error_str, error_len + 11, false);
    }
  }
}

/* Get array data from a remote src to a local dest. */

void
PREFIX(get)(caf_token_t token, size_t offset, int image_index,
            gfc_descriptor_t *src, caf_vector_t *src_vector,
            gfc_descriptor_t *dest, int src_kind, int dst_kind, bool mrt,
            int *pstat)
{
  int j, ierr = 0;
  size_t i, size;
  const int src_rank = GFC_DESCRIPTOR_RANK(src),
            dst_rank = GFC_DESCRIPTOR_RANK(dest);
  const size_t src_size = GFC_DESCRIPTOR_SIZE(src),
               dst_size = GFC_DESCRIPTOR_SIZE(dest);
  const int src_type = GFC_DESCRIPTOR_TYPE(src),
            dst_type = GFC_DESCRIPTOR_TYPE(dest);
  const bool src_contiguous = PREFIX(is_contiguous)(src),
             dst_contiguous = PREFIX(is_contiguous)(dest);
  const bool same_image = caf_this_image == image_index,
             same_type_and_kind = dst_type == src_type && dst_kind == src_kind;

  MPI_Win *p = TOKEN(token);
  ptrdiff_t dimextent, src_offset = 0;
  void *pad_str = NULL, *t_buff = NULL;
  bool free_pad_str = false, free_t_buff = false;
  const bool dest_char_array_is_longer
      = dst_type == BT_CHARACTER && dst_size > src_size && !same_image;
  int remote_image = image_index - 1, this_image = mpi_this_image;

  if (!same_image)
  {
    MPI_Group current_team_group, win_group;
    int trans_ranks[2];
    ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
    chk_err(ierr);
    ierr = MPI_Win_get_group(*p, &win_group);
    chk_err(ierr);
    ierr = MPI_Group_translate_ranks(current_team_group, 2,
                                     (int[]){remote_image, this_image},
                                     win_group, trans_ranks);
    dprint("rank translation: remote: %d -> %d, this: %d -> %d.\n",
           remote_image, trans_ranks[0], this_image, trans_ranks[1]);
    remote_image = trans_ranks[0];
    this_image = trans_ranks[1];
    chk_err(ierr);
    ierr = MPI_Group_free(&current_team_group);
    chk_err(ierr);
    ierr = MPI_Group_free(&win_group);
    chk_err(ierr);
  }

  /* Ensure stat is always set. */
#ifdef GCC_GE_7
  int *stat = pstat;
  if (stat)
    *stat = 0;
#else
  /* Gcc prior to 7.0 does not have stat here. */
  int *stat = NULL;
#endif

  size = 1;
  for (j = 0; j < dst_rank; ++j)
  {
    dimextent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
    if (dimextent < 0)
      dimextent = 0;
    size *= dimextent;
  }

  if (size == 0)
    return;

  dprint("src_vector = %p, image_index = %d (remote = %d), offset = %zd.\n",
         src_vector, image_index, remote_image, offset);
  check_image_health(image_index, stat);

  /* For char arrays: create the padding array, when dst is longer than src. */
  if (dest_char_array_is_longer)
  {
    const size_t pad_num = (dst_size / dst_kind) - (src_size / src_kind);
    const size_t pad_sz = pad_num * dst_kind;
    /* For big arrays alloca() may not be able to get the memory on the stack.
     * Use a regular malloc then. */
    if ((free_pad_str = ((pad_str = alloca(pad_sz)) == NULL)))
    {
      pad_str = malloc(pad_sz);
      if (t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in get().");
    }
    if (dst_kind == 1)
      memset(pad_str, ' ', pad_num);
    else /* dst_kind == 4. */
    {
      for (int32_t *it = (int32_t *)pad_str,
                   *itEnd = ((int32_t *)pad_str) + pad_num;
           it < itEnd; ++it)
      {
        *it = (int32_t)' ';
      }
    }
  }

  if (src_contiguous && dst_contiguous && src_vector == NULL)
  {
    if (same_image)
    {
      dprint("in caf_this == image_index, size = %zd, "
             "dst_kind = %d, src_kind = %d\n",
             size, dst_kind, src_kind);
      if (dst_type == BT_CHARACTER)
        /* The size is encoded in the descriptor's type for char arrays. */
        copy_char_to_self(src->base_addr, src_type, src_size, src_kind,
                          dest->base_addr, dst_type, dst_size, dst_kind, size,
                          src_rank == 0);
      else
        copy_to_self(src, src_kind, dest, dst_kind, size, stat);
      return;
    }
    else
    {
      if (dst_kind != src_kind || dest_char_array_is_longer || src_rank == 0)
      {
        if ((free_t_buff = ((t_buff = alloca(src_size * size)) == NULL)))
        {
          t_buff = malloc(src_size * size);
          if (t_buff == NULL)
            caf_runtime_error("Unable to allocate memory "
                              "for internal buffer in get().");
        }
      }

      if ((same_type_and_kind && dst_rank == src_rank)
          || dst_type == BT_CHARACTER)
      {
        if (!dest_char_array_is_longer
            && (dst_kind == src_kind || dst_type != BT_CHARACTER))
        {
          const size_t trans_size
              = ((dst_size > src_size) ? src_size : dst_size) * size;
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
          ierr = MPI_Get(dest->base_addr, trans_size, MPI_BYTE, remote_image,
                         offset, trans_size, MPI_BYTE, *p);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, *p);
        }
        else
        {
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
          ierr = MPI_Get(t_buff, src_size, MPI_BYTE, remote_image, offset,
                         src_size, MPI_BYTE, *p);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, *p);
          copy_char_to_self(t_buff, src_type, src_size, src_kind,
                            dest->base_addr, dst_type, dst_size, dst_kind, size,
                            src_rank == 0);
        }
      }
      else
      {
        CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
        ierr = MPI_Get(t_buff, src_size * size, MPI_BYTE, remote_image, offset,
                       src_size * size, MPI_BYTE, *p);
        chk_err(ierr);
        CAF_Win_unlock(remote_image, *p);
        convert_with_strides(dest->base_addr, dst_type, dst_kind, dst_size,
                             t_buff, src_type, src_kind,
                             (src_rank > 0) ? src_size : 0, size, stat);
      }
    }
  }
#ifdef STRIDED
  else if (!same_image && same_type_and_kind && dst_type != BT_CHARACTER)
  {
    /* For strided copy, no type and kind conversion, copy to self or
     * character arrays are supported. */
    MPI_Datatype dt_s, dt_d, base_type_src, base_type_dst;
    int *arr_bl;
    int *arr_dsp_s, *arr_dsp_d;

    selectType(src_size, &base_type_src);
    selectType(dst_size, &base_type_dst);

    if (src_rank == 1)
    {
      if (src_vector == NULL)
      {
        dprint("Setting up mpi datatype vector with stride %d, "
               "size %d and offset %d.\n",
               src->dim[0]._stride, size, offset);
        ierr = MPI_Type_vector(size, 1, src->dim[0]._stride, base_type_src,
                               &dt_s);
        chk_err(ierr);
      }
      else
      {
        arr_bl = calloc(size, sizeof(int));
        arr_dsp_s = calloc(size, sizeof(int));

        dprint("Setting up strided vector index.\n", caf_this_image,
               caf_num_images, __FUNCTION__);
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    for (i = 0; i < size; ++i)                                                 \
    {                                                                          \
      arr_dsp_s[i] = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]           \
                      - src->dim[0].lower_bound);                              \
      arr_bl[i] = 1;                                                           \
    }                                                                          \
    break
        switch (src_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
#undef KINDCASE
        ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
        chk_err(ierr);
        free(arr_bl);
        free(arr_dsp_s);
      }
      ierr = MPI_Type_vector(size, 1, dest->dim[0]._stride, base_type_dst,
                             &dt_d);
      chk_err(ierr);
    }
    else
    {
      arr_bl = calloc(size, sizeof(int));
      arr_dsp_s = calloc(size, sizeof(int));
      arr_dsp_d = calloc(size, sizeof(int));

      for (i = 0; i < size; ++i)
      {
        arr_bl[i] = 1;
      }

      for (i = 0; i < size; ++i)
      {
        ptrdiff_t array_offset_sr = 0, extent = 1, tot_ext = 1;
        if (src_vector == NULL)
        {
          for (j = 0; j < src_rank - 1; ++j)
          {
            extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
            array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_sr += (i / tot_ext) * src->dim[src_rank - 1]._stride;
        }
        else
        {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_sr = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]          \
                       - src->dim[0].lower_bound);                             \
    break
          switch (src_vector->u.v.kind)
          {
            KINDCASE(1, int8_t);
            KINDCASE(2, int16_t);
            KINDCASE(4, int32_t);
            KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
            KINDCASE(16, __int128);
#endif
            default:
              caf_runtime_error(unreachable);
              return;
          }
#undef KINDCASE
        }
        arr_dsp_s[i] = array_offset_sr;

        if (dst_rank != 0)
        {
          ptrdiff_t array_offset_dst = 0;
          extent = 1;
          tot_ext = 1;
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_dst += (i / tot_ext) * dest->dim[src_rank - 1]._stride;
          arr_dsp_d[i] = array_offset_dst;
        }
        else
          arr_dsp_d[i] = 0;
      }

      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_s, base_type_src, &dt_s);
      chk_err(ierr);
      ierr = MPI_Type_indexed(size, arr_bl, arr_dsp_d, base_type_dst, &dt_d);
      chk_err(ierr);

      free(arr_bl);
      free(arr_dsp_s);
      free(arr_dsp_d);
    }

    ierr = MPI_Type_commit(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_commit(&dt_d);
    chk_err(ierr);

    CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
    ierr = MPI_Get(dest->base_addr, 1, dt_d, remote_image, offset, 1, dt_s, *p);
    chk_err(ierr);
    CAF_Win_unlock(remote_image, *p);

#ifdef WITH_FAILED_IMAGES
    check_image_health(image_index, stat);

    if (!stat && ierr == STAT_FAILED_IMAGE)
      terminate_internal(ierr, 1);

    if (stat)
      *stat = ierr;
#else
    if (ierr != 0)
    {
      terminate_internal(ierr, 1);
      return;
    }
#endif
    ierr = MPI_Type_free(&dt_s);
    chk_err(ierr);
    ierr = MPI_Type_free(&dt_d);
    chk_err(ierr);
  }
#endif // STRIDED
  else
  {
    if (same_image && mrt)
    {
      if ((free_t_buff = (((t_buff = alloca(src_size * size))) == NULL)))
      {
        t_buff = malloc(src_size * size);
        if (t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in get().");
      }
    }
    else if (!same_type_and_kind && !same_image)
    {
      if ((free_t_buff = (((t_buff = alloca(src_size))) == NULL)))
      {
        t_buff = malloc(src_size);
        if (t_buff == NULL)
          caf_runtime_error("Unable to allocate memory "
                            "for internal buffer in get().");
      }
    }

    for (i = 0; i < size; ++i)
    {
      ptrdiff_t array_offset_sr = 0, extent = 1, tot_ext = 1;
      if (src_vector == NULL)
      {
        for (j = 0; j < src_rank - 1; ++j)
        {
          extent = src->dim[j]._ubound - src->dim[j].lower_bound + 1;
          array_offset_sr += ((i / tot_ext) % extent) * src->dim[j]._stride;
          tot_ext *= extent;
        }

        array_offset_sr += (i / tot_ext) * src->dim[src_rank - 1]._stride;
      }
      else
      {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_sr = ((ptrdiff_t)((type *)src_vector->u.v.vector)[i]          \
                       - src->dim[0].lower_bound);                             \
    break
        switch (src_vector->u.v.kind)
        {
          KINDCASE(1, int8_t);
          KINDCASE(2, int16_t);
          KINDCASE(4, int32_t);
          KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
          KINDCASE(16, __int128);
#endif
          default:
            caf_runtime_error(unreachable);
            return;
        }
      }
      src_offset = array_offset_sr * src_size;
#undef KINDCASE

      void *dst;
      if (!same_image || !mrt)
      {
        if (dst_rank != 0)
        {
          ptrdiff_t array_offset_dst = 0;
          extent = 1;
          tot_ext = 1;
          for (j = 0; j < dst_rank - 1; ++j)
          {
            extent = dest->dim[j]._ubound - dest->dim[j].lower_bound + 1;
            array_offset_dst += ((i / tot_ext) % extent) * dest->dim[j]._stride;
            tot_ext *= extent;
          }

          array_offset_dst += (i / tot_ext) * dest->dim[dst_rank - 1]._stride;
          dst = (void *)((char *)dest->base_addr + array_offset_dst * dst_size);
        }
        else
          dst = dest->base_addr;
      }

      if (!same_image)
      {
        // Do the more likely first.
        dprint("kind(dst) = %d, el_sz(dst) = %zd, "
               "kind(src) = %d, el_sz(src) = %zd, lb(dst) = %zd.\n",
               dst_kind, dst_size, src_kind, src_size, src->dim[0].lower_bound);
        if (same_type_and_kind)
        {
          const size_t trans_size = (src_size < dst_size) ? src_size : dst_size;
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
          ierr = MPI_Get(dst, trans_size, MPI_BYTE, remote_image,
                         offset + src_offset, trans_size, MPI_BYTE, *p);
          CAF_Win_unlock(remote_image, *p);
          chk_err(ierr);
          if (pad_str)
            memcpy((void *)((char *)dst + src_size), pad_str,
                   dst_size - src_size);
        }
        else if (dst_type == BT_CHARACTER)
        {
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
          ierr = MPI_Get(t_buff, src_size, MPI_BYTE, remote_image,
                         offset + src_offset, src_size, MPI_BYTE, *p);
          CAF_Win_unlock(remote_image, *p);
          chk_err(ierr);
          copy_char_to_self(t_buff, src_type, src_size, src_kind, dst, dst_type,
                            dst_size, dst_kind, 1, true);
        }
        else
        {
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, *p);
          ierr = MPI_Get(t_buff, src_size, MPI_BYTE, remote_image,
                         offset + src_offset, src_size, MPI_BYTE, *p);
          CAF_Win_unlock(remote_image, *p);
          chk_err(ierr);
          convert_type(dst, dst_type, dst_kind, t_buff, src_type, src_kind,
                       stat);
        }
      }
      else
      {
        if (!mrt)
        {
          dprint("strided same_image, no temp, for i = %zd, "
                 "src_offset = %zd, offset = %zd.\n",
                 i, src_offset, offset);
          if (same_type_and_kind)
            memmove(dst, src->base_addr + src_offset, src_size);
          else
            convert_type(dst, dst_type, dst_kind, src->base_addr + src_offset,
                         src_type, src_kind, stat);
        }
        else
        {
          dprint("strided same_image, *WITH* temp, for i = %zd.\n", i);
          if (same_type_and_kind)
            memmove(t_buff + i * dst_size, src->base_addr + src_offset,
                    src_size);
          else
            convert_type(t_buff + i * dst_size, dst_type, dst_kind,
                         src->base_addr + src_offset, src_type, src_kind, stat);
        }
      }

#ifndef WITH_FAILED_IMAGES
      if (ierr != 0)
      {
        caf_runtime_error("MPI Error: %d", ierr);
        return;
      }
#endif
    }

    if (same_image && mrt)
    {
      dprint("Same image temporary move.\n");
      memmove(dest->base_addr, t_buff, size * dst_size);
    }
  }

  /* Free memory, when not allocated on stack. */
  if (free_t_buff)
    free(t_buff);
  if (free_pad_str)
    free(pad_str);

#ifdef WITH_FAILED_IMAGES
  /* Catch failed images, when failed image support is active. */
  check_image_health(image_index, stat);
#endif

  if (ierr != MPI_SUCCESS)
  {
    int mpi_error;
    MPI_Error_class(ierr, &mpi_error);
    if (stat)
      *stat = mpi_error;
    else
    {
      int error_len = 2048 - 11;
      char error_str[error_len + 11];
      strcpy(error_str, "MPI-error: ");
      MPI_Error_string(mpi_error, &error_str[11], &error_len);
      error_stop_str(error_str, error_len + 11, false);
    }
  }
}

#ifdef GCC_GE_7
/* Get a chunk of data from one image to the current one, with type conversion.
 *
 * Copied from the gcc:libgfortran/caf/single.c. Can't say much about it. */
static void
get_data(void *ds, mpi_caf_token_t *token, MPI_Aint offset, int dst_type,
         int src_type, int dst_kind, int src_kind, size_t dst_size,
         size_t src_size, size_t num, int *stat, int image_index)
{
  size_t k;
  int ierr;
  MPI_Win win = (token == NULL) ? global_dynamic_win : token->memptr_win;
#ifdef EXTRA_DEBUG_OUTPUT
  if (token)
    dprint("%p = win(%d): %d -> offset: %zd of size %zd -> %zd, "
           "dst type %d(%d), src type %d(%d)\n",
           ds, win, image_index + 1, offset, src_size, dst_size, dst_type,
           dst_kind, src_type, src_kind);
  else
    dprint("%p = global_win(%d) offset: %zd (0x%lx) of size %zd -> %zd, "
           "dst type %d(%d), src type %d(%d)\n",
           ds, image_index + 1, offset, offset, src_size, dst_size, dst_type,
           dst_kind, src_type, src_kind);
#endif
  if (dst_type == src_type && dst_kind == src_kind)
  {
    size_t sz = ((dst_size > src_size) ? src_size : dst_size) * num;
    CAF_Win_lock(MPI_LOCK_SHARED, image_index, win);
    ierr = MPI_Get(ds, sz, MPI_BYTE, image_index, offset, sz, MPI_BYTE, win);
    CAF_Win_unlock(image_index, win);
    chk_err(ierr);
    if ((dst_type == BT_CHARACTER || src_type == BT_CHARACTER)
        && dst_size > src_size)
    {
      if (dst_kind == 1)
      {
        memset((void *)((char *)ds + src_size), ' ', dst_size - src_size);
      }
      else /* dst_kind == 4. */
      {
        for (k = src_size / 4; k < dst_size / 4; k++)
          ((int32_t *)ds)[k] = (int32_t)' ';
      }
    }
  }
  else if (dst_type == BT_CHARACTER && dst_kind == 1)
  {
    /* Get the required amount of memory on the stack. */
    void *srh = alloca(src_size);
    CAF_Win_lock(MPI_LOCK_SHARED, image_index, win);
    ierr = MPI_Get(srh, src_size, MPI_BYTE, image_index, offset, src_size,
                   MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
    assign_char1_from_char4(dst_size, src_size, ds, srh);
  }
  else if (dst_type == BT_CHARACTER)
  {
    /* Get the required amount of memory on the stack. */
    void *srh = alloca(src_size);
    CAF_Win_lock(MPI_LOCK_SHARED, image_index, win);
    ierr = MPI_Get(srh, src_size, MPI_BYTE, image_index, offset, src_size,
                   MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
    assign_char4_from_char1(dst_size, src_size, ds, srh);
  }
  else
  {
    /* Get the required amount of memory on the stack. */
    void *srh = alloca(src_size * num);
    dprint("type/kind convert %zd items: "
           "type %d(%d) -> type %d(%d), local buffer: %p\n",
           num, src_type, src_kind, dst_type, dst_kind, srh);
    CAF_Win_lock(MPI_LOCK_SHARED, image_index, win);
    ierr = MPI_Get(srh, src_size * num, MPI_BYTE, image_index, offset,
                   src_size * num, MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
    dprint("srh[0] = %d, ierr = %d\n", (int)((char *)srh)[0], ierr);
    for (k = 0; k < num; ++k)
    {
      convert_type(ds, dst_type, dst_kind, srh, src_type, src_kind, stat);
      ds += dst_size;
      srh += src_size;
    }
  }
}

/* Compute the number of items referenced.
 *
 * Computes the number of items between lower bound (lb) and upper bound (ub)
 * with respect to the stride taking corner cases into account. */
#define COMPUTE_NUM_ITEMS(num, stride, lb, ub)                                 \
  do                                                                           \
  {                                                                            \
    ptrdiff_t abs_stride = (stride) > 0 ? (stride) : -(stride);                \
    num = (stride) > 0 ? (ub) + 1 - (lb) : (lb) + 1 - (ub);                    \
    if (num <= 0 || abs_stride < 1)                                            \
      return;                                                                  \
    num = (abs_stride > 1) ? (1 + (num - 1) / abs_stride) : num;               \
  } while (0)

typedef struct gfc_dim1_descriptor_t
{
  gfc_descriptor_t base;
  descriptor_dimension dim[1];
} gfc_dim1_descriptor_t;

static void
get_for_ref(caf_reference_t *ref, size_t *i, size_t dst_index,
            mpi_caf_token_t *mpi_token, gfc_descriptor_t *dst,
            gfc_descriptor_t *src, void *ds, void *sr, ptrdiff_t sr_byte_offset,
            void *rdesc, ptrdiff_t desc_byte_offset, int dst_kind, int src_kind,
            size_t dst_dim, size_t src_dim, size_t num, int *stat,
            int global_dynamic_win_rank, int memptr_win_rank,
            bool sr_global,  /* access sr through global_dynamic_win */
            bool desc_global /* access desc through global_dynamic_win */
#ifdef GCC_GE_8
            ,
            int src_type)
{
#else
)
{
  int src_type = -1;
#endif
  ptrdiff_t extent_src = 1, array_offset_src = 0, stride_src;
  size_t next_dst_dim, ref_rank;
  gfc_max_dim_descriptor_t src_desc_data;
  int ierr;

  if (unlikely(ref == NULL))
  {
    /* May be we should issue an error here, because this case should not
     * occur. */
    return;
  }

  dprint("caf_ref = %p (type = %d), sr_offset = %zd, sr = %p, rdesc = %p, "
         "desc_offset = %zd, src = %p, sr_glb = %d, desc_glb = %d, src_dim = "
         "%zd\n",
         ref, ref->type, sr_byte_offset, sr, rdesc, desc_byte_offset, src,
         sr_global, desc_global, src_dim);

  if (ref->next == NULL)
  {
    size_t dst_size = GFC_DESCRIPTOR_SIZE(dst);

    switch (ref->type)
    {
      case CAF_REF_COMPONENT:
        dprint("caf_offset = %zd\n", ref->u.c.caf_token_offset);
        if (ref->u.c.caf_token_offset > 0)
        {
          sr_byte_offset += ref->u.c.offset;
          if (sr_global)
          {
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&sr, stdptr_size, MPI_BYTE, global_dynamic_win_rank,
                           MPI_Aint_add((MPI_Aint)sr, sr_byte_offset),
                           stdptr_size, MPI_BYTE, global_dynamic_win);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
            chk_err(ierr);
            desc_global = true;
          }
          else
          {
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&sr, stdptr_size, MPI_BYTE, memptr_win_rank,
                           sr_byte_offset, stdptr_size, MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            sr_global = true;
          }
          sr_byte_offset = 0;
        }
        else
          sr_byte_offset += ref->u.c.offset;
        if (sr_global)
        {
          get_data(ds, NULL, MPI_Aint_add((MPI_Aint)sr, sr_byte_offset),
                   GFC_DESCRIPTOR_TYPE(dst),
#ifdef GCC_GE_8
                   (src_type != -1) ? src_type : GFC_DESCRIPTOR_TYPE(dst),
#else
                   GFC_DESCRIPTOR_TYPE(dst),
#endif
                   dst_kind, src_kind, dst_size, ref->item_size, 1, stat,
                   global_dynamic_win_rank);
        }
        else
        {
          get_data(ds, mpi_token, sr_byte_offset, GFC_DESCRIPTOR_TYPE(dst),
#ifdef GCC_GE_8
                   src_type,
#else
                   GFC_DESCRIPTOR_TYPE(src),
#endif
                   dst_kind, src_kind, dst_size, ref->item_size, 1, stat,
                   memptr_win_rank);
        }
        ++(*i);
        return;
      case CAF_REF_STATIC_ARRAY:
        src_type = ref->u.a.static_array_type;
        /* Intentionally fall through. */
      case CAF_REF_ARRAY:
        if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
        {
          if (sr_global)
          {
            get_data(ds + dst_index * dst_size, NULL,
                     MPI_Aint_add((MPI_Aint)sr, sr_byte_offset),
                     GFC_DESCRIPTOR_TYPE(dst),
#ifdef GCC_GE_8
                     (src_type != -1) ? src_type : GFC_DESCRIPTOR_TYPE(src),
#else
                     (src_type == -1) ? GFC_DESCRIPTOR_TYPE(src) : src_type,
#endif
                     dst_kind, src_kind, dst_size, ref->item_size, num, stat,
                     global_dynamic_win_rank);
          }
          else
          {
            get_data(ds + dst_index * dst_size, mpi_token, sr_byte_offset,
                     GFC_DESCRIPTOR_TYPE(dst),
#ifdef GCC_GE_8
                     (src_type != -1) ? src_type : GFC_DESCRIPTOR_TYPE(src),
#else
                     (src_type == -1) ? GFC_DESCRIPTOR_TYPE(src) : src_type,
#endif
                     dst_kind, src_kind, dst_size, ref->item_size, num, stat,
                     memptr_win_rank);
          }
          *i += num;
          return;
        }
        break;
      default:
        caf_runtime_error(unreachable);
    }
  }

  switch (ref->type)
  {
    case CAF_REF_COMPONENT:
      sr_byte_offset += ref->u.c.offset;
      if (ref->u.c.caf_token_offset > 0)
      {
        desc_byte_offset = sr_byte_offset;
        rdesc = sr;
        if (sr_global)
        {
          CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                       global_dynamic_win);
          ierr = MPI_Get(&sr, stdptr_size, MPI_BYTE, global_dynamic_win_rank,
                         MPI_Aint_add((MPI_Aint)sr, sr_byte_offset),
                         stdptr_size, MPI_BYTE, global_dynamic_win);
          CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
          chk_err(ierr);
          desc_global = true;
        }
        else
        {
          CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank, mpi_token->memptr_win);
          ierr = MPI_Get(&sr, stdptr_size, MPI_BYTE, memptr_win_rank,
                         sr_byte_offset, stdptr_size, MPI_BYTE,
                         mpi_token->memptr_win);
          chk_err(ierr);
          CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
          sr_global = true;
        }
        sr_byte_offset = 0;
      }
      else
      {
        desc_byte_offset += ref->u.c.offset;
      }
      get_for_ref(ref->next, i, dst_index, mpi_token, dst, NULL, ds, sr,
                  sr_byte_offset, rdesc, desc_byte_offset, dst_kind, src_kind,
                  dst_dim, 0, 1, stat, global_dynamic_win_rank, memptr_win_rank,
                  sr_global, desc_global
#ifdef GCC_GE_8
                  ,
                  src_type
#endif
      );
      return;
    case CAF_REF_ARRAY:
      if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
      {
        get_for_ref(ref->next, i, dst_index, mpi_token, dst, src, ds, sr,
                    sr_byte_offset, rdesc, desc_byte_offset, dst_kind, src_kind,
                    dst_dim, 0, 1, stat, global_dynamic_win_rank,
                    memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                    ,
                    src_type
#endif
        );
        return;
      }
      /* Only when on the left most index switch the data pointer to the
       * array's data pointer. */
      if (src_dim == 0)
      {
        if (sr_global)
        {
          for (ref_rank = 0; ref->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          /* Get the remote descriptor. */
          if (desc_global)
          {
            MPI_Aint disp = MPI_Aint_add((MPI_Aint)rdesc, desc_byte_offset);
            dprint("Fetching remote descriptor from %lx.\n", disp);
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&src_desc_data, sizeof_desc_for_rank(ref_rank),
                           MPI_BYTE, global_dynamic_win_rank, disp,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           global_dynamic_win);
            chk_err(ierr);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
            sr = src_desc_data.base.base_addr;
          }
          else
          {
            dprint("Fetching remote data.\n");
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&src_desc_data, sizeof_desc_for_rank(ref_rank),
                           MPI_BYTE, memptr_win_rank, desc_byte_offset,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            desc_global = true;
          }
          src = (gfc_descriptor_t *)&src_desc_data;
        }
        else
          src = mpi_token->desc;

        sr_byte_offset = 0;
        desc_byte_offset = 0;
#ifdef EXTRA_DEBUG_OUTPUT
        dprint("remote desc rank: %d, base: %p\n", GFC_DESCRIPTOR_RANK(src),
               src->base_addr);
        for (int r = 0; r < GFC_DESCRIPTOR_RANK(src); ++r)
        {
          dprint("remote desc dim[%d] = (lb=%zd, ub=%zd, stride=%zd)\n", r,
                 src->dim[r].lower_bound, src->dim[r]._ubound,
                 src->dim[r]._stride);
        }
#endif
      }
      switch (ref->u.a.mode[src_dim])
      {
        case CAF_ARR_REF_VECTOR:
          extent_src = GFC_DESCRIPTOR_EXTENT(src, src_dim);
          array_offset_src = 0;
          for (size_t idx = 0; idx < ref->u.a.dim[src_dim].v.nvec; ++idx)
          {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_src                                                           \
        = (((ptrdiff_t)((type *)ref->u.a.dim[src_dim].v.vector)[idx])          \
           - src->dim[src_dim].lower_bound * src->dim[src_dim]._stride);       \
    break

            switch (ref->u.a.dim[src_dim].v.kind)
            {
              KINDCASE(1, int8_t);
              KINDCASE(2, int16_t);
              KINDCASE(4, int32_t);
              KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
              KINDCASE(16, __int128);
#endif
              default:
                caf_runtime_error(unreachable);
                return;
            }
#undef KINDCASE

            dprint("vector-index computed to: %zd\n", array_offset_src);
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, src, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
          }
          return;
        case CAF_ARR_REF_FULL:
          COMPUTE_NUM_ITEMS(extent_src, ref->u.a.dim[src_dim].s.stride,
                            src->dim[src_dim].lower_bound,
                            src->dim[src_dim]._ubound);
          stride_src
              = src->dim[src_dim]._stride * ref->u.a.dim[src_dim].s.stride;
          array_offset_src = 0;
          for (ptrdiff_t idx = 0; idx < extent_src;
               ++idx, array_offset_src += stride_src)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, src, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
          }
          return;
        case CAF_ARR_REF_RANGE:
          COMPUTE_NUM_ITEMS(extent_src, ref->u.a.dim[src_dim].s.stride,
                            ref->u.a.dim[src_dim].s.start,
                            ref->u.a.dim[src_dim].s.end);
          array_offset_src
              = (ref->u.a.dim[src_dim].s.start - src->dim[src_dim].lower_bound)
                * src->dim[src_dim]._stride;
          stride_src
              = src->dim[src_dim]._stride * ref->u.a.dim[src_dim].s.stride;
          /* Increase the dst_dim only, when the src_extent is greater than one
           * or src and dst extent are both one. Don't increase when the scalar
           * source is not present in the dst. */
          next_dst_dim = ((extent_src > 1)
                          || (GFC_DESCRIPTOR_EXTENT(dst, dst_dim) == 1
                              && extent_src == 1))
                             ? (dst_dim + 1)
                             : dst_dim;
          for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, src, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, next_dst_dim, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
            array_offset_src += stride_src;
          }
          return;
        case CAF_ARR_REF_SINGLE:
          array_offset_src
              = (ref->u.a.dim[src_dim].s.start - src->dim[src_dim].lower_bound)
                * src->dim[src_dim]._stride;
          get_for_ref(ref, i, dst_index, mpi_token, dst, src, ds, sr,
                      sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                      desc_byte_offset + array_offset_src * ref->item_size,
                      dst_kind, src_kind, dst_dim, src_dim + 1, 1, stat,
                      global_dynamic_win_rank, memptr_win_rank, sr_global,
                      desc_global
#ifdef GCC_GE_8
                      ,
                      src_type
#endif
          );
          return;
        case CAF_ARR_REF_OPEN_END:
          COMPUTE_NUM_ITEMS(extent_src, ref->u.a.dim[src_dim].s.stride,
                            ref->u.a.dim[src_dim].s.start,
                            src->dim[src_dim]._ubound);
          stride_src
              = src->dim[src_dim]._stride * ref->u.a.dim[src_dim].s.stride;
          array_offset_src
              = (ref->u.a.dim[src_dim].s.start - src->dim[src_dim].lower_bound)
                * src->dim[src_dim]._stride;
          for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, src, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
            array_offset_src += stride_src;
          }
          return;
        case CAF_ARR_REF_OPEN_START:
          COMPUTE_NUM_ITEMS(extent_src, ref->u.a.dim[src_dim].s.stride,
                            src->dim[src_dim].lower_bound,
                            ref->u.a.dim[src_dim].s.end);
          stride_src
              = src->dim[src_dim]._stride * ref->u.a.dim[src_dim].s.stride;
          array_offset_src = 0;
          for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, src, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
            array_offset_src += stride_src;
          }
          return;
        default:
          caf_runtime_error(unreachable);
      }
      return;
    case CAF_REF_STATIC_ARRAY:
      if (ref->u.a.mode[src_dim] == CAF_ARR_REF_NONE)
      {
        get_for_ref(ref->next, i, dst_index, mpi_token, dst, NULL, ds, sr,
                    sr_byte_offset, rdesc, desc_byte_offset, dst_kind, src_kind,
                    dst_dim, 0, 1, stat, global_dynamic_win_rank,
                    memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                    ,
                    src_type
#endif
        );
        return;
      }
      switch (ref->u.a.mode[src_dim])
      {
        case CAF_ARR_REF_VECTOR:
          array_offset_src = 0;
          for (size_t idx = 0; idx < ref->u.a.dim[src_dim].v.nvec; ++idx)
          {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_src = ((type *)ref->u.a.dim[src_dim].v.vector)[idx];          \
    break

            switch (ref->u.a.dim[src_dim].v.kind)
            {
              KINDCASE(1, int8_t);
              KINDCASE(2, int16_t);
              KINDCASE(4, int32_t);
              KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
              KINDCASE(16, __int128);
#endif
              default:
                caf_runtime_error(unreachable);
                return;
            }
#undef KINDCASE

            get_for_ref(
                ref, i, dst_index, mpi_token, dst, NULL, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
          }
          return;
        case CAF_ARR_REF_FULL:
          for (array_offset_src = 0;
               array_offset_src <= ref->u.a.dim[src_dim].s.end;
               array_offset_src += ref->u.a.dim[src_dim].s.stride)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, NULL, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
          }
          return;
        case CAF_ARR_REF_RANGE:
          COMPUTE_NUM_ITEMS(extent_src, ref->u.a.dim[src_dim].s.stride,
                            ref->u.a.dim[src_dim].s.start,
                            ref->u.a.dim[src_dim].s.end);
          array_offset_src = ref->u.a.dim[src_dim].s.start;
          for (ptrdiff_t idx = 0; idx < extent_src; ++idx)
          {
            get_for_ref(
                ref, i, dst_index, mpi_token, dst, NULL, ds, sr,
                sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                desc_byte_offset + array_offset_src * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, sr_global, desc_global
#ifdef GCC_GE_8
                ,
                src_type
#endif
            );
            dst_index += dst->dim[dst_dim]._stride;
            array_offset_src += ref->u.a.dim[src_dim].s.stride;
          }
          return;
        case CAF_ARR_REF_SINGLE:
          array_offset_src = ref->u.a.dim[src_dim].s.start;
          get_for_ref(ref, i, dst_index, mpi_token, dst, NULL, ds, sr,
                      sr_byte_offset + array_offset_src * ref->item_size, rdesc,
                      desc_byte_offset + array_offset_src * ref->item_size,
                      dst_kind, src_kind, dst_dim, src_dim + 1, 1, stat,
                      global_dynamic_win_rank, memptr_win_rank, sr_global,
                      desc_global
#ifdef GCC_GE_8
                      ,
                      src_type
#endif
          );
          return;
          /* The OPEN_* are mapped to a RANGE and therefore can not occur. */
        case CAF_ARR_REF_OPEN_END:
        case CAF_ARR_REF_OPEN_START:
        default:
          caf_runtime_error(unreachable);
      }
      return;
    default:
      caf_runtime_error(unreachable);
  }
}

#ifdef GCC_GE_15
void
PREFIX(register_accessor)(const int hash, getter_t accessor)
{
  if (accessor_hash_table_state == AHT_UNINITIALIZED)
  {
    aht_cap = 16;
    accessor_hash_table = calloc(aht_cap, sizeof(struct accessor_hash_t));
    accessor_hash_table_state = AHT_OPEN;
  }
  if (aht_size == aht_cap)
  {
    aht_cap += 16;
    accessor_hash_table = realloc(accessor_hash_table,
                                  aht_cap * sizeof(struct accessor_hash_t));
  }
  if (accessor_hash_table_state == AHT_PREPARED)
  {
    accessor_hash_table_state = AHT_OPEN;
  }
  dprint("adding function %p with hash %x.\n", accessor, hash);
  accessor_hash_table[aht_size].hash = hash;
  accessor_hash_table[aht_size].u.getter = accessor;
  ++aht_size;
}

static int
hash_compare(const struct accessor_hash_t *lhs,
             const struct accessor_hash_t *rhs)
{
  return lhs->hash < rhs->hash ? -1 : (lhs->hash > rhs->hash ? 1 : 0);
}

void
PREFIX(register_accessors_finish)()
{
  if (accessor_hash_table_state == AHT_PREPARED
      || accessor_hash_table_state == AHT_UNINITIALIZED)
    return;

  qsort(accessor_hash_table, aht_size, sizeof(struct accessor_hash_t),
        (int (*)(const void *, const void *))hash_compare);
  accessor_hash_table_state = AHT_PREPARED;
  dprint("finished accessor hash table.\n");
}

int
PREFIX(get_remote_function_index)(const int hash)
{
  if (accessor_hash_table_state != AHT_PREPARED)
  {
    caf_runtime_error("the accessor hash table is not prepared.");
  }

  struct accessor_hash_t cand;
  cand.hash = hash;
  struct accessor_hash_t *f = bsearch(
      &cand, accessor_hash_table, aht_size, sizeof(struct accessor_hash_t),
      (int (*)(const void *, const void *))hash_compare);

  int index = f ? f - accessor_hash_table : -1;
  dprint("the index for accessor hash %x is %d.\n", hash, index);
  return index;
}

static void
get_from_self(caf_token_t token, const gfc_descriptor_t *opt_src_desc,
              const size_t *opt_src_charlen,
              const int image_index __attribute__((unused)), void **dst_data,
              size_t *opt_dst_charlen, gfc_descriptor_t *opt_dst_desc,
              const bool may_realloc_dst, const int getter_index,
              void *get_data, const int this_image)
{
  const bool dst_incl_desc = opt_dst_desc && may_realloc_dst,
             has_src_desc = opt_src_desc;
  int32_t ignore;
  gfc_max_dim_descriptor_t tmp_desc;
  void *dst_ptr = opt_dst_desc
                      ? (dst_incl_desc ? opt_dst_desc : (void *)&tmp_desc)
                      : dst_data;
  const bool needs_copy_back = opt_dst_desc && !may_realloc_dst;
  mpi_caf_token_t src_token = {get_data, MPI_WIN_NULL, NULL};
  void *src_ptr = has_src_desc ? (void *)opt_src_desc
                               : ((mpi_caf_token_t *)token)->memptr;

  if (needs_copy_back)
  {
    memcpy(&tmp_desc, opt_dst_desc,
           sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_dst_desc)));
    tmp_desc.base.base_addr = NULL;
  }

  dprint("Shortcutting due to self access on image %d.\n", image_index);
  accessor_hash_table[getter_index].u.getter(get_data, &this_image, dst_ptr,
                                             &ignore, src_ptr, &src_token, 0,
                                             opt_dst_charlen, opt_src_charlen);

  if (needs_copy_back)
  {
    const size_t sz = compute_arr_data_size(opt_dst_desc);

    memcpy(opt_dst_desc->base_addr, tmp_desc.base.base_addr, sz);
    free(tmp_desc.base.base_addr);
  }
}

bool
team_translate(int *remote_image, int *this_image, caf_token_t token,
               int image_index, caf_team_t *team, int *team_number, int *stat)
{
  MPI_Group current_team_group, win_group;
  int ierr;
  int trans_ranks[2];
  MPI_Comm comm = CAF_COMM_WORLD;

#ifdef GCC_GE_16
  if (team)
    comm = (*(caf_teams_list_t **)team)->communicator;
  else if (team_number)
  {
    bool found = false;
    for (caf_team_stack_node_t *cur = current_team; cur; cur = cur->parent)
      if (cur->team_list_elem->team_id == *team_number)
      {
        comm = cur->team_list_elem->communicator;
        found = true;
        break;
      }
    if (!found)
    {
      caf_internal_error("Team number %d not found", stat, NULL, 0,
                         *team_number);
      return false;
    }
  }
#endif

  ierr = MPI_Comm_group(comm, &current_team_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(*TOKEN(token), &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 2,
                                   (int[]){image_index - 1, mpi_this_image},
                                   win_group, trans_ranks);
  chk_err(ierr);
  *remote_image = trans_ranks[0];
  *this_image = trans_ranks[1];
  ierr = MPI_Group_free(&current_team_group);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);
  return true;
}

/* Get data from a remote image's memory pointed to by `token`.  The image is
 * given by `image_index`.  When the source is descriptor array, then
 * `opt_src_desc` gives its dimension as of the source image.  On the remote
 * image the base address will be replaced.  `opt_src_charlen` gives the length
 * of the source string on the remote image when that is a character array.
 * `dst_size` then gives the number of bytes of each character.
 * `opt_src_charlen` is null, when this is no character array.
 * `*dst_size` gives the expected number of bytes to be stored in `*dst_data`.
 * `*dst_data` gives the memory where the data is stored.  This address may be
 * changed, when reallocation is necessary.
 * `opt_dst_charlen` is NULL when dst is not a character array, or stores the
 * number a characters in `*dst_data`.
 * 'opt_dst_desc' is an optional descriptor. Its address in memory is fixed,
 * but its data may be changed. `getter_index` is the index in the hashtable as
 * returned by `get_remote_function_index()`. `get_data` is optional data to be
 * passed to the getter function. `get_data_size` is the size of the former
 * data.  `*stat` will be set to non-zero on error, when `stat` is not null.
 * `team` and `team_number` will be used for team and number of the team in the
 * future. At the moment these are only placeholders.
 */
void
PREFIX(get_from_remote)(caf_token_t token, const gfc_descriptor_t *opt_src_desc,
                        const size_t *opt_src_charlen, const int image_index,
                        const size_t dst_size, void **dst_data,
                        size_t *opt_dst_charlen, gfc_descriptor_t *opt_dst_desc,
                        const bool may_realloc_dst, const int getter_index,
                        void *get_data, const size_t get_data_size, int *stat,
                        caf_team_t *team, int *team_number)
{
  int ierr, this_image, remote_image;
  bool free_t_buff, free_msg;
  void *t_buff;
  ct_msg_t *msg;
  const bool dst_incl_desc = opt_dst_desc && may_realloc_dst,
             has_src_desc = opt_src_desc,
             external_call = *TOKEN(token) != MPI_WIN_NULL;
  const size_t dst_desc_size
      = opt_dst_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_dst_desc))
                     : 0,
      src_desc_size
      = has_src_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_src_desc))
                     : 0,
      msg_size
      = sizeof(ct_msg_t) + dst_desc_size + src_desc_size + get_data_size;
  struct running_accesses_t *rat;

  if (stat)
    *stat = 0;

  // Get mapped remote image
  if (external_call)
  {
    if (unlikely(!team_translate(&remote_image, &this_image, token, image_index,
                                 team, team_number, stat)))
      return;
  }
  else
  {
    remote_image = image_index - 1;
    this_image = mpi_this_image;
  }

  check_image_health(remote_image, stat);

  dprint(
      "Entering get_from_remote(), token = %p, win_rank = %d, this_rank = %d, "
      "getter index = %d, sizeof(src_desc) = %zd, sizeof(dst_desc) = %zd.\n",
      token, remote_image, this_image, getter_index, src_desc_size,
      dst_desc_size);

  if (this_image == remote_image)
  {
    get_from_self(token, opt_src_desc, opt_src_charlen, image_index, dst_data,
                  opt_dst_charlen, opt_dst_desc, may_realloc_dst, getter_index,
                  get_data, this_image);
    return;
  }
  // create get msg
  if ((free_msg = (((msg = alloca(msg_size))) == NULL)))
  {
    msg = malloc(msg_size);
    if (msg == NULL)
      caf_runtime_error("Unable to allocate memory "
                        "for internal message in get_from_remote().");
  }
  msg->cmd = remote_command_get;
  msg->transfer_size = dst_size;
  msg->opt_charlen = opt_src_charlen ? *opt_src_charlen : 0;
  msg->win = *TOKEN(token);
  msg->dest_image = mpi_this_image;
  msg->dest_tag = CAF_CT_TAG + 1;
  msg->dest_opt_charlen = opt_dst_charlen ? *opt_dst_charlen : 1;
  msg->flags = (opt_dst_desc ? CT_DST_HAS_DESC : 0)
               | (has_src_desc ? CT_SRC_HAS_DESC : 0)
               | (opt_src_charlen ? CT_CHAR_ARRAY : 0)
               | (dst_incl_desc ? CT_INCLUDE_DESCRIPTOR : 0);
  dprint("message flags: %x.\n", msg->flags);
  msg->accessor_index = getter_index;
  if (opt_dst_desc)
    memcpy(msg->data, opt_dst_desc, dst_desc_size);
  if (has_src_desc)
    memcpy(msg->data + dst_desc_size, opt_src_desc, src_desc_size);

  memcpy(msg->data + dst_desc_size + src_desc_size, get_data, get_data_size);

  if (external_call)
  {
    msg->ra_id = running_accesses_id_cnt++;
    rat = (struct running_accesses_t *)malloc(
        sizeof(struct running_accesses_t));
    rat->id = msg->ra_id;
    rat->memptr = msg->data + dst_desc_size + src_desc_size;
    rat->next = running_accesses;
    running_accesses = rat;
  }
  else
    msg->ra_id = (rat_id_t)((struct mpi_caf_token_t *)token)->memptr;

  // call get on remote
  ierr = MPI_Send(msg, msg_size, MPI_BYTE, remote_image, CAF_CT_TAG, ct_COMM);
  chk_err(ierr);

  if (!opt_dst_charlen && !dst_incl_desc)
  {
    // allocate local buffer
    if ((free_t_buff = (((t_buff = alloca(dst_size))) == NULL)))
    {
      t_buff = malloc(dst_size);
      if (t_buff == NULL)
        caf_runtime_error("Unable to allocate memory "
                          "for internal buffer in get_from_remote().");
    }
    dprint("waiting to receive %zd bytes from %d.\n", dst_size,
           image_index - 1);
    ierr = MPI_Recv(t_buff, dst_size, MPI_BYTE, image_index - 1, msg->dest_tag,
                    CAF_COMM_WORLD, MPI_STATUS_IGNORE);
    chk_err(ierr);
    dprint("received %zd bytes as requested from %d.\n", dst_size,
           image_index - 1);
    // dump_mem("get_from_remote", t_buff, dst_size);
    memcpy(*dst_data, t_buff, dst_size);

    if (free_t_buff)
      free(t_buff);
  }
  else
  {
    MPI_Status status;
    MPI_Message msg_han;
    int cnt;

    dprint("probing for incoming message from %d, tag %d.\n", image_index - 1,
           msg->dest_tag);
    ierr = MPI_Mprobe(image_index - 1, msg->dest_tag, CAF_COMM_WORLD, &msg_han,
                      &status);
    chk_err(ierr);
    if (ierr == MPI_SUCCESS)
    {
      MPI_Get_count(&status, MPI_BYTE, &cnt);
      dprint("get message of %d bytes from image %d, tag %d, dest_addr %p.\n",
             cnt, image_index - 1, msg->dest_tag, *dst_data);
      if (may_realloc_dst)
        *dst_data = realloc(*dst_data, cnt);
      // else // max cnt
      ierr = MPI_Mrecv(*dst_data, cnt, MPI_BYTE, &msg_han, &status);
      chk_err(ierr);
      if (opt_dst_charlen)
        *opt_dst_charlen = cnt / dst_size;
      if (dst_incl_desc)
      {
        const size_t desc_size = sizeof_desc_for_rank(
            GFC_DESCRIPTOR_RANK((gfc_descriptor_t *)(*dst_data)));
        dprint("refitting dst descriptor of size %zd at %p with data %zd at %p "
               "from %d bytes transfered.\n",
               desc_size, opt_dst_desc, cnt - desc_size, *dst_data, cnt);
        memcpy(opt_dst_desc, *dst_data, desc_size);
        memmove(*dst_data, (*dst_data) + desc_size, cnt - desc_size);
        opt_dst_desc->base_addr = *dst_data
            = realloc(*dst_data, cnt - desc_size);
        dump_mem("ret data", opt_dst_desc->base_addr, cnt - desc_size);
      }
    }
    else
    {
      int err_len;
      char err_str[MPI_MAX_ERROR_STRING];
      MPI_Error_string(status.MPI_ERROR, err_str, &err_len);
      caf_runtime_error("Got MPI error %d retrieving result: %s",
                        status.MPI_ERROR, err_str);
    }
  }

  if (running_accesses == rat)
    running_accesses = rat->next;
  else
  {
    struct running_accesses_t *pra = running_accesses;
    for (; pra && pra->next != rat; pra = pra->next)
      ;
    pra->next = rat->next;
  }
  free(rat);

  if (free_msg)
    free(msg);

  dprint("done with get_from_remote.\n");
}

int32_t
PREFIX(is_present_on_remote)(caf_token_t token, const int image_index,
                             const int is_present_index, void *add_data,
                             const size_t add_data_size)
{
  /* Unregistered tokens are always not present.  */
  if (!token)
    return 0;

  MPI_Group current_team_group, win_group;
  int ierr, this_image, remote_image;
  int trans_ranks[2];
  bool free_msg;
  int32_t result = 0;
  ct_msg_t *msg;
  const size_t msg_size = sizeof(ct_msg_t) + add_data_size;
  struct running_accesses_t *rat;

  // Get mapped remote image
  ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(*TOKEN(token), &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 2,
                                   (int[]){image_index - 1, mpi_this_image},
                                   win_group, trans_ranks);
  chk_err(ierr);
  remote_image = trans_ranks[0];
  this_image = trans_ranks[1];
  ierr = MPI_Group_free(&current_team_group);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);

  check_image_health(remote_image, stat);

  dprint("Entering is_present_on_remote(), token = %p, memptr = %p, win_rank = "
         "%d, this_rank = %d, is_present index = %d, sizeof(msg) = %ld.\n",
         token, ((mpi_caf_token_t *)token)->memptr, remote_image, this_image,
         is_present_index, msg_size);

  if (this_image == remote_image)
  {
    int32_t result = 0;
    mpi_caf_token_t src_token = {get_data, MPI_WIN_NULL, NULL};
    void *src_ptr = ((mpi_caf_token_t *)token)->memptr;

    dprint("Shortcutting due to self access on image %d.\n", image_index);
    accessor_hash_table[is_present_index].u.is_present(
        add_data, &this_image, &result, &src_ptr, &src_token, 0);

    return result;
  }

  // create get msg
  if ((free_msg = (((msg = alloca(msg_size))) == NULL)))
  {
    msg = malloc(msg_size);
    if (msg == NULL)
      caf_runtime_error("Unable to allocate memory "
                        "for internal message in get_from_remote().");
  }
  msg->cmd = remote_command_present;
  msg->transfer_size = 1;
  msg->opt_charlen = 0;
  msg->win = *TOKEN(token);
  msg->dest_image = mpi_this_image;
  msg->dest_tag = CAF_CT_TAG + 1;
  msg->dest_opt_charlen = 0;
  msg->flags = 0;
  dprint("message flags: %x.\n", msg->flags);
  msg->accessor_index = is_present_index;

  memcpy(msg->data, add_data, add_data_size);

  msg->ra_id = running_accesses_id_cnt++;
  rat = (struct running_accesses_t *)malloc(sizeof(struct running_accesses_t));
  rat->id = msg->ra_id;
  rat->memptr = msg->data;
  rat->next = running_accesses;
  running_accesses = rat;

  // call get on remote
  ierr = MPI_Send(msg, msg_size, MPI_BYTE, remote_image, CAF_CT_TAG, ct_COMM);
  chk_err(ierr);

  dprint("waiting to receive %d bytes from %d.\n", 1, image_index - 1);
  ierr = MPI_Recv(&result, 1, MPI_BYTE, image_index - 1, msg->dest_tag,
                  CAF_COMM_WORLD, MPI_STATUS_IGNORE);
  chk_err(ierr);
  dprint("received %d bytes as requested from %d.\n", 1, image_index - 1);

  if (running_accesses == rat)
    running_accesses = rat->next;
  else
  {
    struct running_accesses_t *pra = running_accesses;
    for (; pra && pra->next != rat; pra = pra->next)
      ;
    pra->next = rat->next;
  }
  free(rat);
  if (free_msg)
    free(msg);

  dprint("done with is_present_on_remote.\n");
  return result;
}

static void
send_to_self(caf_token_t token, gfc_descriptor_t *opt_dst_desc,
             const size_t *opt_dst_charlen,
             const int image_index __attribute__((unused)),
             const size_t src_size, const void *src_data,
             size_t *opt_src_charlen, const gfc_descriptor_t *opt_src_desc,
             const int setter_index, void *add_data, const int this_image)
{
  const bool requires_temp
      = (opt_src_desc
         && ((mpi_caf_token_t *)token)->memptr == opt_src_desc->base_addr)
        || (!opt_src_desc && ((mpi_caf_token_t *)token)->memptr == src_data);
  void *dst_ptr
      = opt_dst_desc ? opt_dst_desc : ((mpi_caf_token_t *)token)->memptr;
  mpi_caf_token_t src_token = {add_data, MPI_WIN_NULL, NULL};
  const void *src_ptr = opt_src_desc ? opt_src_desc : src_data,
             *orig_src_ptr = src_ptr;
  const size_t sz
      = requires_temp
            ? opt_src_desc ? compute_arr_data_size(opt_src_desc) : src_size
            : 0;
  bool free_tmp = false;
  if (requires_temp)
  {
    void *tmp_ptr;
    if ((free_tmp = (tmp_ptr = alloca(sz)) == NULL))
    {
      tmp_ptr = malloc(sz);
      if (!tmp_ptr)
        caf_runtime_error("can not allocate %zd bytes for temp buffer in send",
                          sz);
    }
    memcpy(tmp_ptr, opt_src_desc ? opt_src_desc->base_addr : src_ptr, sz);
    if (opt_src_desc)
    {
      orig_src_ptr = opt_src_desc->base_addr;
      ((gfc_descriptor_t *)opt_src_desc)->base_addr = tmp_ptr;
    }
    else
      src_ptr = tmp_ptr;
  }

  dprint("Shortcutting due to self access on image %d %s temporary on %s.\n",
         image_index, requires_temp ? "w/ " : "w/o",
         opt_src_desc ? "array " : "scalar");
  accessor_hash_table[setter_index].u.receiver(
      add_data, &this_image, dst_ptr, src_ptr, &src_token, 0, opt_dst_charlen,
      opt_src_charlen);

  if (requires_temp)
  {
    if (opt_src_desc)
    {
      if (free_tmp)
        free(opt_src_desc->base_addr);
      ((gfc_descriptor_t *)opt_src_desc)->base_addr = (void *)orig_src_ptr;
    }
    else if (free_tmp)
      free((void *)src_ptr);
  }
}

/* Send data to a remote image's memory pointed to by `token`.  The image
 * is given by `image_index`. When the data is a descriptor array, then
 * `opt_dst_desc` gives the descriptor. Its data pointer is replaced by the
 * memory of the remote image. `opt_dst_charlen` gives the length of the
 * destination string on the remote image when that is a character array.
 * `in_src_size` then gives the number of bytes of each character, else
 * `in_src_size` gives the bytes to transfer from `src_data`. If `src_data` is
 * a character array, then `opt_src_charlen` gives its number of characters.
 * When source is a descriptor array, then `opt_src_desc` gives the descriptor.
 * `setter_index` is the index in the hashtable as returned by
 * `get_remote_function_index()`. `add_data` is optional data to be
 * passed to the getter function. `add_data_size` is the size of the former
 * data. `*stat` will be set to non-zero on error, when `stat` is not null.
 * `team` and `team_number` will be used for team and number of the team in the
 * future. At the moment these are only placeholders.
 */
void
PREFIX(send_to_remote)(caf_token_t token, gfc_descriptor_t *opt_dst_desc,
                       const size_t *opt_dst_charlen, const int image_index,
                       const size_t in_src_size, const void *src_data,
                       size_t *opt_src_charlen,
                       const gfc_descriptor_t *opt_src_desc,
                       const int setter_index, void *add_data,
                       const size_t add_data_size, int *stat, caf_team_t *team,
                       int *team_number)
{
  int ierr, this_image, remote_image;
  bool free_msg;
  ct_msg_t *msg;
  const bool dst_incl_desc = opt_dst_desc, has_src_desc = opt_src_desc,
             external_call = *TOKEN(token) != MPI_WIN_NULL;
  const size_t dst_desc_size
      = opt_dst_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_dst_desc))
                     : 0,
      src_desc_size
      = has_src_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_src_desc))
                     : 0;
  size_t src_size
      = opt_src_charlen ? in_src_size * *opt_src_charlen : in_src_size,
      msg_size = sizeof(ct_msg_t) + src_size + dst_desc_size + src_desc_size
                 + add_data_size;
  struct running_accesses_t *rat;

  if (stat)
    *stat = 0;

  // Get mapped remote image
  if (external_call)
  {
    if (unlikely(!team_translate(&remote_image, &this_image, token, image_index,
                                 team, team_number, stat)))
      return;
  }
  else
  {
    remote_image = image_index - 1;
    this_image = mpi_this_image;
  }

  check_image_health(remote_image, stat);
  if (opt_src_charlen && opt_src_desc)
  {
    const size_t sz = compute_arr_data_size_sz(opt_src_desc, 1);
    msg_size -= src_size;
    src_size *= sz;
    msg_size += src_size;
  }

  dprint("Entering send_to_remote(), token = %p, memptr = %p, win_rank = %d, "
         "this_rank = %d, setter index = %d, sizeof(data = %p) = %zd, "
         "sizeof(src_desc) = %zd, sizeof(dst_desc) = %zd, sizeof(msg) = %zd.\n",
         token, ((mpi_caf_token_t *)token)->memptr, remote_image, this_image,
         setter_index, opt_src_desc ? opt_src_desc->base_addr : src_data,
         src_size, src_desc_size, dst_desc_size, msg_size);

  /* Shortcut for copy to self.  */
  if (this_image == remote_image)
  {
    send_to_self(token, opt_dst_desc, opt_dst_charlen, image_index, src_size,
                 src_data, opt_src_charlen, opt_src_desc, setter_index,
                 add_data, this_image);
    return;
  }

  // create get msg
  if ((free_msg = (((msg = alloca(msg_size))) == NULL)))
  {
    msg = malloc(msg_size);
    if (msg == NULL)
      caf_runtime_error("Unable to allocate memory "
                        "for internal message in send_to_remote().");
  }
  msg->cmd = remote_command_send;
  msg->transfer_size = src_size;
  msg->opt_charlen = opt_src_charlen ? *opt_src_charlen : 0;
  msg->win = *TOKEN(token);
  msg->dest_image = mpi_this_image;
  msg->dest_tag = CAF_CT_TAG + 1;
  msg->dest_opt_charlen = opt_dst_charlen ? *opt_dst_charlen : 1;
  msg->flags = (opt_dst_desc ? CT_DST_HAS_DESC : 0)
               | (has_src_desc ? CT_SRC_HAS_DESC : 0)
               | (opt_src_charlen ? CT_CHAR_ARRAY : 0)
               | (dst_incl_desc ? CT_INCLUDE_DESCRIPTOR : 0);
  dprint("message flags: %x.\n", msg->flags);
  msg->accessor_index = setter_index;
  if (has_src_desc)
  {
    memcpy(msg->data, opt_src_desc->base_addr, src_size);
    memcpy(msg->data + src_size, opt_src_desc, src_desc_size);
  }
  else
    memcpy(msg->data, src_data, src_size);
  if (opt_dst_desc)
    memcpy(msg->data + src_size + src_desc_size, opt_dst_desc, dst_desc_size);

  memcpy(msg->data + src_size + src_desc_size + dst_desc_size, add_data,
         add_data_size);

  if (external_call)
  {
    msg->ra_id = running_accesses_id_cnt++;
    rat = (struct running_accesses_t *)malloc(
        sizeof(struct running_accesses_t));
    rat->id = msg->ra_id;
    rat->memptr = msg->data + src_size + dst_desc_size + src_desc_size;
    rat->next = running_accesses;
    running_accesses = rat;
  }
  else
    msg->ra_id = (rat_id_t)((struct mpi_caf_token_t *)token)->memptr;

  // call get on remote
  ierr = MPI_Send(msg, msg_size, MPI_BYTE, remote_image, CAF_CT_TAG, ct_COMM);
  chk_err(ierr);

  {
    char c;
    dprint("waiting to receive %d bytes from %d on tag %d.\n", 1, image_index,
           msg->dest_tag);
    ierr = MPI_Recv(&c, 1, MPI_BYTE, image_index - 1, msg->dest_tag,
                    CAF_COMM_WORLD, MPI_STATUS_IGNORE);
    chk_err(ierr);
    dprint("received %d bytes as requested from %d on tag %d.\n", 1,
           image_index, msg->dest_tag);
  }

  if (running_accesses == rat)
    running_accesses = rat->next;
  else
  {
    struct running_accesses_t *pra = running_accesses;
    for (; pra && pra->next != rat; pra = pra->next)
      ;
    pra->next = rat->next;
  }
  free(rat);

  if (free_msg)
    free(msg);

  dprint("done with send_to_remote.\n");
}

/* Transfer data from one remote image's memory to a different remote image.
 * The memory on the destination image is given by `dst_token`.  If that memory
 * is a descriptor array, then `opt_dst_desc` gives the descriptor of the array
 * on the initiating image. When the destination data is a string, then
 * `opt_dst_charlen` gives the number of characters. The destination images
 * index is given by `dst_image_index`. The accessor to use is given by
 * `dst_access_index`. Addititional data to provide to the accessor on the
 * destination is given by `dst_add_data` and that size in `dst_add_data_size`.
 * The source for the transfer is given by `src_token`.  If the memory is an
 * array, then `opt_src_desc` gives it descriptor. When the source is a string,
 * then `opt_src_charlen` gives its the length. `in_src_size` then
 * gives the number of bytes of each character. `opt_src_charlen` is null, when
 * this is no character array.
 * The index of the source image is given by `src_image_index`. The index for
 * the getter is specified in `src_access_index`. Additional data to provide to
 * this getter is given in `*src_add_data` and its size in `src_add_data_size`.
 * The parameter `in_src_size` specifies the size of data to transfer from on
 * to the other image, when the data to transfer is not an array. In the latter
 * case it is ignored. When the data to transfer is a character array, then
 * `in_src_size` gives the size of one character. The `scalar_transfer`
 * indicates that the data between the two images is not a descriptor array.
 * `dst_stat` if set, gets set to zero on success. Should there be an error,
 * then this is set to non-zero.  When `src_stat` is set, it is set to zero.
 * `dst_team`, `dst_team_number`, `src_team` and `src_team_number` will be used
 * for team and number of the team in the future. At the moment these are only
 * placeholders.
 */
void
PREFIX(transfer_between_remotes)(
    caf_token_t dst_token, gfc_descriptor_t *opt_dst_desc,
    size_t *opt_dst_charlen, const int dst_image_index,
    const int dst_access_index, void *dst_add_data,
    const size_t dst_add_data_size, caf_token_t src_token,
    const gfc_descriptor_t *opt_src_desc, const size_t *opt_src_charlen,
    const int src_image_index, const int src_access_index, void *src_add_data,
    const size_t src_add_data_size, const size_t in_src_size,
    const bool scalar_transfer, int *dst_stat, int *src_stat,
    caf_team_t *dst_team, int *dst_team_number, caf_team_t *src_team,
    int *src_team_number)
{
  MPI_Group current_team_group, win_group;
  int ierr, this_image, src_remote_image, dst_remote_image;
  int trans_ranks[3];
  bool free_msg;
  ct_msg_t *full_msg, *dst_msg;
  struct transfer_msg_data_t *tmd;
  const bool has_src_desc = opt_src_desc;
  const size_t dst_desc_size
      = opt_dst_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_dst_desc))
                     : 0,
      src_desc_size
      = has_src_desc ? sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(opt_src_desc))
                     : 0;
  size_t src_size
      = opt_src_charlen ? in_src_size * *opt_src_charlen : in_src_size,
      dst_msg_size = sizeof(ct_msg_t) + sizeof(struct transfer_msg_data_t)
                     + dst_desc_size + dst_add_data_size,
      full_msg_size
      = sizeof(ct_msg_t) + dst_msg_size + src_desc_size + src_add_data_size;
  struct running_accesses_t *rat;

  if (dst_stat)
    *dst_stat = 0;
  if (src_stat)
    *src_stat = 0;

  // Get mapped remote image
  if (unlikely(!team_translate(&src_remote_image, &this_image, src_token,
                               src_image_index, src_team, src_team_number,
                               src_stat)))
    return;
  if (unlikely(!team_translate(&dst_remote_image, &this_image, dst_token,
                               dst_image_index, dst_team, dst_team_number,
                               dst_stat)))
    return;

  dprint("team-map: dst(in) %d -> %d, src(in) %d -> %d, this %d -> %d.\n",
         dst_image_index, dst_remote_image, src_image_index, src_remote_image,
         caf_this_image, this_image);
  check_image_health(src_remote_image, src_stat);
  check_image_health(dst_remote_image, dst_stat);

  if (opt_src_charlen && opt_src_desc)
  {
    const size_t sz = compute_arr_data_size_sz(opt_src_desc, 1);
    full_msg_size -= src_size;
    dst_msg_size -= src_size;
    src_size *= sz;
    full_msg_size += src_size;
    dst_msg_size += src_size;
  }

  dprint("Entering transfer_between_remotes(), dst_token = %p, dst_rank = %d, "
         "this_rank = %d, setter index = %d, src_token = %p, src_rank = %d, "
         "getter index = %d, sizeof(src_desc) = %zd, sizeof(dst_desc) = %zd, "
         "sizeof(msg) = %zd, scalar_transfer = %d, in_src_size = %zd, src_size "
         "= %zd.\n",
         dst_token, dst_remote_image, this_image, dst_access_index, src_token,
         src_remote_image, src_access_index, src_desc_size, dst_desc_size,
         full_msg_size, scalar_transfer, in_src_size, src_size);

  /* Shortcut for copy to self.  */
  if (this_image == src_remote_image && this_image == dst_remote_image)
  {
    void *dptr = NULL;
    gfc_max_dim_descriptor_t max_desc;
    gfc_descriptor_t *trans_desc
        = scalar_transfer ? NULL : (gfc_descriptor_t *)&max_desc;

    if (!scalar_transfer)
      trans_desc->base_addr = NULL;

    get_from_self(src_token, opt_src_desc, opt_src_charlen, src_image_index,
                  &dptr, opt_dst_charlen, trans_desc, true, src_access_index,
                  src_add_data, this_image);
    send_to_self(dst_token, opt_dst_desc, opt_dst_charlen, dst_image_index,
                 src_size, dptr, (size_t *)opt_src_charlen, trans_desc,
                 dst_access_index, dst_add_data, this_image);

    if (trans_desc)
      free(trans_desc->base_addr);

    return;
  }
  else if (this_image == src_remote_image)
  {
    void *dptr = NULL;
    gfc_max_dim_descriptor_t max_desc;
    gfc_descriptor_t *trans_desc
        = scalar_transfer ? NULL : (gfc_descriptor_t *)&max_desc;

    if (!scalar_transfer)
      trans_desc->base_addr = NULL;

    // Essentially a send_to_remote
    get_from_self(src_token, opt_src_desc, opt_src_charlen, src_image_index,
                  &dptr, opt_dst_charlen, trans_desc, true, src_access_index,
                  src_add_data, this_image);
    _gfortran_caf_send_to_remote(
        dst_token, opt_dst_desc, opt_dst_charlen, dst_image_index, src_size,
        dptr, (size_t *)opt_src_charlen, trans_desc, dst_access_index,
        dst_add_data, dst_add_data_size, dst_stat, dst_team, dst_team_number);
    if (trans_desc)
      free(trans_desc->base_addr);
    return;
  }
  else if (this_image == dst_remote_image)
  {
    // Essentially a get_from_remote
    void *dptr = NULL;
    gfc_max_dim_descriptor_t max_desc;
    gfc_descriptor_t *trans_desc
        = scalar_transfer ? NULL : (gfc_descriptor_t *)&max_desc;

    if (!scalar_transfer)
      trans_desc->base_addr = NULL;
    if (scalar_transfer)
      dptr = malloc(src_size);

    _gfortran_caf_get_from_remote(
        src_token, opt_src_desc, opt_src_charlen, src_image_index, src_size,
        &dptr, opt_dst_charlen, trans_desc, true, src_access_index,
        src_add_data, src_add_data_size, src_stat, src_team, src_team_number);
    send_to_self(dst_token, opt_dst_desc, opt_dst_charlen, dst_image_index,
                 src_size, dptr, (size_t *)opt_src_charlen, trans_desc,
                 dst_access_index, dst_add_data, this_image);

    if (trans_desc)
      free(trans_desc->base_addr);
    if (scalar_transfer)
      free(dptr);
    return;
  }

  // create get msg
  if ((free_msg = (((full_msg = alloca(full_msg_size))) == NULL)))
  {
    full_msg = malloc(full_msg_size);
    if (full_msg == NULL)
      caf_runtime_error("Unable to allocate memory "
                        "for internal message in transfer_between_remotes().");
  }
  full_msg->cmd = remote_command_transfer;
  full_msg->transfer_size = src_size;
  full_msg->opt_charlen = opt_src_charlen ? *opt_src_charlen : 0;
  full_msg->win = *TOKEN(src_token);
  full_msg->dest_image = dst_remote_image;
  full_msg->dest_tag = CAF_CT_TAG;
  full_msg->dest_opt_charlen = opt_dst_charlen ? *opt_dst_charlen : 1;
  full_msg->flags = (opt_dst_desc ? CT_DST_HAS_DESC : 0)
                    | (has_src_desc ? CT_SRC_HAS_DESC : 0)
                    | (opt_src_charlen ? CT_CHAR_ARRAY : 0)
                    | (scalar_transfer ? 0 : CT_TRANSFER_DESC);
  dprint("get message flags: %x.\n", full_msg->flags);
  full_msg->accessor_index = src_access_index;

  /* The message to hand to the reciever of the data.  */
  dst_msg = (ct_msg_t *)full_msg->data;
  dst_msg->cmd = remote_command_send;
  dst_msg->transfer_size = src_size;
  dst_msg->opt_charlen = opt_src_charlen ? *opt_src_charlen : 0;
  dst_msg->win = *TOKEN(dst_token);
  dst_msg->dest_image = mpi_this_image;
  dst_msg->dest_tag = CAF_CT_TAG + 1;
  dst_msg->dest_opt_charlen = opt_dst_charlen ? *opt_dst_charlen : 1;
  dst_msg->flags
      = (opt_dst_desc ? (CT_DST_HAS_DESC | CT_INCLUDE_DESCRIPTOR) : 0)
        | (scalar_transfer ? 0 : CT_SRC_HAS_DESC)
        | (opt_src_charlen ? CT_CHAR_ARRAY : 0);
  dprint("send message flags: %x.\n", dst_msg->flags);
  dst_msg->accessor_index = dst_access_index;

  tmd = (struct transfer_msg_data_t *)dst_msg->data;
  tmd->dst_msg_size = dst_msg_size;
  tmd->dst_desc_size = dst_desc_size;
  tmd->dst_add_data_size = dst_add_data_size;

  /* Data for forwarding result to receiver.  */
  if (opt_dst_desc)
    memcpy(tmd->data, opt_dst_desc, dst_desc_size);
  memcpy(tmd->data + dst_desc_size, dst_add_data, dst_add_data_size);

  /* Data the getter needs.  */
  if (has_src_desc)
    memcpy(tmd->data + dst_desc_size + dst_add_data_size, opt_src_desc,
           src_desc_size);
  memcpy(tmd->data + dst_desc_size + dst_add_data_size + src_desc_size,
         src_add_data, src_add_data_size);

  full_msg->ra_id = running_accesses_id_cnt++;
  rat = (struct running_accesses_t *)malloc(sizeof(struct running_accesses_t));
  rat->id = full_msg->ra_id;
  rat->memptr = full_msg->data + src_size + dst_desc_size + src_desc_size;
  rat->next = running_accesses;
  running_accesses = rat;

  // initiate transfer on getter
  dprint("message size is %zd, dst_desc_size: %zd, src_desc_size: %zd.\n",
         full_msg_size, dst_desc_size, src_desc_size);
  ierr = MPI_Send(full_msg, full_msg_size, MPI_BYTE, src_remote_image,
                  CAF_CT_TAG, ct_COMM);
  chk_err(ierr);

  {
    char c;
    dprint("waiting to receive %d bytes from %d on tag %d.\n", 1,
           dst_image_index, dst_msg->dest_tag);
    ierr = MPI_Recv(&c, 1, MPI_BYTE, dst_image_index - 1, dst_msg->dest_tag,
                    CAF_COMM_WORLD, MPI_STATUS_IGNORE);
    chk_err(ierr);
    if (dst_stat)
      *dst_stat = c;
    dprint("received %d bytes as requested from %d on tag %d.\n", 1,
           dst_image_index, dst_msg->dest_tag);
  }

  if (running_accesses == rat)
    running_accesses = rat->next;
  else
  {
    struct running_accesses_t *pra = running_accesses;
    for (; pra && pra->next != rat; pra = pra->next)
      ;
    pra->next = rat->next;
  }
  free(rat);

  if (free_msg)
    free(full_msg);

  dprint("done with transfer_between_remotes.\n");
}
#endif

void
PREFIX(get_by_ref)(caf_token_t token, int image_index, gfc_descriptor_t *dst,
                   caf_reference_t *refs, int dst_kind, int src_kind,
                   bool may_require_tmp __attribute__((unused)),
                   bool dst_reallocatable, int *stat
#ifdef GCC_GE_8
                   ,
                   int src_type
#endif
)
{
  const char vecrefunknownkind[]
      = "libcaf_mpi::caf_get_by_ref(): unknown kind in vector-ref.\n";
  const char unknownreftype[]
      = "libcaf_mpi::caf_get_by_ref(): unknown reference type.\n";
  const char unknownarrreftype[]
      = "libcaf_mpi::caf_get_by_ref(): unknown array reference type.\n";
  const char rankoutofrange[]
      = "libcaf_mpi::caf_get_by_ref(): rank out of range.\n";
  const char extentoutofrange[]
      = "libcaf_mpi::caf_get_by_ref(): extent out of range.\n";
  const char cannotallocdst[]
      = "libcaf_mpi::caf_get_by_ref(): can not allocate %d bytes of memory.\n";
  const char nonallocextentmismatch[]
      = "libcaf_mpi::caf_get_by_ref(): extent of non-allocatable arrays "
        "mismatch (%lu != %lu).\n";
  const char doublearrayref[]
      = "libcaf_mpi::caf_get_by_ref(): two or more array part references "
        "are not supported.\n";
  size_t size, i, ref_rank, dst_index, src_size;
  int ierr, dst_rank = GFC_DESCRIPTOR_RANK(dst), dst_cur_dim = 0;
  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *)token;
  void *remote_memptr = mpi_token->memptr, *remote_base_memptr = NULL;
  gfc_max_dim_descriptor_t src_desc;
  gfc_descriptor_t *src = (gfc_descriptor_t *)&src_desc;
  caf_reference_t *riter = refs;
  long delta;
  ptrdiff_t data_offset = 0, desc_offset = 0;
  /* Reallocation of dst.data is needed (e.g., array to small). */
  bool realloc_needed;
  /* Reallocation of dst.data is required, because data is not alloced at
   * all. */
  bool realloc_required, extent_mismatch = false;
  /* Set when the first non-scalar array reference is encountered. */
  bool in_array_ref = false, array_extent_fixed = false;
  /* Set when a non-scalar result is expected in the array-refs. */
  bool non_scalar_array_ref_expected = false;
  /* Set when remote data is to be accessed through the
   * global dynamic window. */
  bool access_data_through_global_win = false;
  /* Set when the remote descriptor is to accessed through the global window. */
  bool access_desc_through_global_win = false;
  caf_array_ref_t array_ref;

  realloc_needed = realloc_required = dst->base_addr == NULL;

  if (stat)
    *stat = 0;

  MPI_Group current_team_group, win_group;
  int global_dynamic_win_rank, memptr_win_rank;
  ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(global_dynamic_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){image_index - 1}, win_group,
                                   &global_dynamic_win_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(mpi_token->memptr_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){image_index - 1}, win_group,
                                   &memptr_win_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&current_team_group);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);

  check_image_health(global_dynamic_win_rank, stat);

  dprint("Entering get_by_ref(may_require_tmp = %d), win_rank = %d, "
         "global_rank = %d.\n",
         may_require_tmp, memptr_win_rank, global_dynamic_win_rank);

  /* Compute the size of the result.  In the beginning size just counts the
   * number of elements. */
  size = 1;
  while (riter)
  {
    dprint("caf_ref = %p, type = %d,  offset = %zd, remote_mem = %p, "
           "global_win(data, desc)) = (%d, %d)\n",
           riter, riter->type, data_offset, remote_memptr,
           access_data_through_global_win, access_desc_through_global_win);
    switch (riter->type)
    {
      case CAF_REF_COMPONENT:
        data_offset += riter->u.c.offset;
        if (riter->u.c.caf_token_offset > 0)
        {
          remote_base_memptr = remote_memptr;
          if (access_data_through_global_win)
          {
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           global_dynamic_win_rank,
                           MPI_Aint_add((MPI_Aint)remote_memptr, data_offset),
                           stdptr_size, MPI_BYTE, global_dynamic_win);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
            chk_err(ierr);
            dprint("global_win access: remote_memptr(old) = %p, "
                   "remote_memptr(new) = %p, offset = %zd.\n",
                   remote_base_memptr, remote_memptr, data_offset);
            /* On the second indirection access also the remote descriptor
             * using the global window. */
            access_desc_through_global_win = true;
          }
          else
          {
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           memptr_win_rank, data_offset, stdptr_size, MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            dprint("get(custom_token %d): remote_memptr(old) = %p, "
                   "remote_memptr(new) = %p, offset = %zd\n",
                   mpi_token->memptr_win, remote_base_memptr, remote_memptr,
                   data_offset);
            /* All future access is through the global dynamic window. */
            access_data_through_global_win = true;
          }
          desc_offset = data_offset;
          data_offset = 0;
        }
        else
        {
          desc_offset += riter->u.c.offset;
        }
        break;
      case CAF_REF_ARRAY:
        non_scalar_array_ref_expected = false;
        /* When there has been no CAF_REF_COMP before hand, then the
         * descriptor is stored in the token and the extends are the same on
         * all images, which is taken care of in the else part. */
        if (access_data_through_global_win)
        {
          /* Compute the ref_rank here and while doing so also figure whether
           * only a scalar result is expected (all refs are CAF_SINGLE). */
          for (ref_rank = 0; riter->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
          {
            non_scalar_array_ref_expected
                = non_scalar_array_ref_expected
                  || riter->u.a.mode[ref_rank] != CAF_ARR_REF_SINGLE;
          }
          /* Get the remote descriptor and use the stack to store it. Note,
           * src may be pointing to mpi_token->desc therefore it needs to be
           * reset here. */
          src = (gfc_descriptor_t *)&src_desc;
          if (access_desc_through_global_win)
          {
            size_t datasize = sizeof_desc_for_rank(ref_rank);
            dprint("remote desc fetch from %p, offset = %td, ref_rank = %zd, "
                   "get_size = %zd, rank = %d\n",
                   remote_base_memptr, desc_offset, ref_rank, datasize,
                   global_dynamic_win_rank);
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(
                src, datasize, MPI_BYTE, global_dynamic_win_rank,
                MPI_Aint_add((MPI_Aint)remote_base_memptr, desc_offset),
                datasize, MPI_BYTE, global_dynamic_win);
            chk_err(ierr);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
          }
          else
          {
            dprint(
                "remote desc fetch from win %d, offset = %td, ref_rank = %zd\n",
                mpi_token->memptr_win, desc_offset, ref_rank);
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(src, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           memptr_win_rank, desc_offset,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            access_desc_through_global_win = true;
          }
        }
        else
        {
          src = mpi_token->desc;
          /* Figure if a none scalar array ref is expected, which is important
           * to know beforehand, because else the descriptor of the destination
           * array may be errorneously constructed. */
          for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
          {
            if (riter->u.a.mode[i] != CAF_ARR_REF_SINGLE)
            {
              non_scalar_array_ref_expected = true;
              break;
            }
          }
        }

#ifdef EXTRA_DEBUG_OUTPUT
        dprint("remote desc rank: %d, base_addr: %p\n",
               GFC_DESCRIPTOR_RANK(src), src->base_addr);
        for (i = 0; i < GFC_DESCRIPTOR_RANK(src); ++i)
        {
          dprint("remote desc dim[%zd] = (lb=%zd, ub=%zd, stride=%zd)\n", i,
                 src->dim[i].lower_bound, src->dim[i]._ubound,
                 src->dim[i]._stride);
        }
#endif
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    remote_memptr += (((ptrdiff_t)((type *)riter->u.a.dim[i].v.vector)[0])     \
                      - src->dim[i].lower_bound)                               \
                     * src->dim[i]._stride * riter->item_size;                 \
    break

              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_internal_error(vecrefunknownkind, stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                src->dim[i].lower_bound, src->dim[i]._ubound);
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start, src->dim[i]._ubound);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_START:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                src->dim[i].lower_bound,
                                riter->u.a.dim[i].s.end);
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            default:
              caf_internal_error(unknownarrreftype, stat, NULL, 0);
              return;
          }
          dprint("i = %zd, array_ref = %s, delta = %ld, in_array_ref = %d, "
                 "arr_ext_fixed = %d, realloc_required = %d\n",
                 i, caf_array_ref_str[array_ref], delta, in_array_ref,
                 array_extent_fixed, realloc_required);
          if (delta <= 0)
            return;
          /* Check the various properties of the destination array.
           * Is an array expected and present? */
          if (delta > 1 && dst_rank == 0)
          {
            /* No, an array is required, but not provided. */
            caf_internal_error(extentoutofrange, stat, NULL, 0);
            return;
          }
          /* When dst is an array. */
          if (dst_rank > 0)
          {
            /* Check that dst_cur_dim is valid for dst. Can be superceeded
             * only by scalar data. */
            if (dst_cur_dim >= dst_rank && delta != 1)
            {
              caf_internal_error(rankoutofrange, stat, NULL, 0);
              return;
            }
            /* Do further checks, when the source is not scalar. */
            else if (non_scalar_array_ref_expected)
            {
              /* Check that the extent is not scalar and we are not in an array
               * ref for the dst side. */
              if (!in_array_ref)
              {
                /* Check that this is the non-scalar extent. */
                if (!array_extent_fixed)
                {
                  /* In an array extent now. */
                  in_array_ref = true;
                  /* Check that we haven't skipped any scalar  dimensions yet
                   * and that the dst is compatible. */
                  if (i > 0 && dst_rank == GFC_DESCRIPTOR_RANK(src))
                  {
                    if (dst_reallocatable)
                    {
                      /* Dst is reallocatable, which means that the bounds are
                       * not set. Set them. */
                      for (dst_cur_dim = 0; dst_cur_dim < (int)i; ++dst_cur_dim)
                      {
                        dst->dim[dst_cur_dim].lower_bound = 1;
                        dst->dim[dst_cur_dim]._ubound = 1;
                        dst->dim[dst_cur_dim]._stride = 1;
                      }
                    }
                    else
                      dst_cur_dim = i;
                  }
                  /* Else press thumbs, that there are enough dimensional refs
                   * to come. Checked below. */
                }
                else
                {
                  caf_internal_error(doublearrayref, stat, NULL, 0);
                  return;
                }
              }
              /* When the realloc is required, then no extent may have
               * been set. */
              extent_mismatch
                  = realloc_required
                    || (delta != 1
                        && GFC_DESCRIPTOR_EXTENT(dst, dst_cur_dim) != delta);
              /* When it already known, that a realloc is needed or the extent
               * does not match the needed one. */
              if (realloc_needed || extent_mismatch)
              {
                /* Check whether dst is reallocatable. */
                if (unlikely(!dst_reallocatable))
                {
                  caf_internal_error(nonallocextentmismatch, stat, NULL, 0,
                                     delta,
                                     GFC_DESCRIPTOR_EXTENT(dst, dst_cur_dim));
                  return;
                }
                /* Only report an error, when the extent needs to be modified,
                 * which is not allowed. */
                else if (!dst_reallocatable && extent_mismatch)
                {
                  caf_internal_error(extentoutofrange, stat, NULL, 0);
                  return;
                }
                realloc_needed = true;
              }
              /* Only change the extent when it does not match.  This is to
               * prevent resetting given array bounds. */
              if (extent_mismatch)
              {
                dst->dim[dst_cur_dim].lower_bound = 1;
                dst->dim[dst_cur_dim]._ubound = delta;
                dst->dim[dst_cur_dim]._stride = size;
                if (realloc_required)
                  dst->offset = -1;
              }
            }

            /* Only increase the dim counter, when in an array ref */
            if (in_array_ref && dst_cur_dim < dst_rank)
            {
              /* Mode != CAF_ARR_REF_SINGLE(delta == 1), and no rank
               * reduction */
              if (!(delta == 1 && dst_rank != GFC_DESCRIPTOR_RANK(src)))
                ++dst_cur_dim;
            }
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
        non_scalar_array_ref_expected = false;
        /* Figure if a none scalar array ref is expected, which is important
         * to know beforehand, because else the descriptor of the destination
         * array may be errorneously constructed. */
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          if (riter->u.a.mode[i] != CAF_ARR_REF_SINGLE)
          {
            non_scalar_array_ref_expected = true;
            break;
          }
        }

        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    data_offset += ((type *)riter->u.a.dim[i].v.vector)[0] * riter->item_size; \
    break

              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_internal_error(vecrefunknownkind, stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              delta = riter->u.a.dim[i].s.end / riter->u.a.dim[i].s.stride + 1;
              /* The memptr stays unchanged when ref'ing the first element in a
               * dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              data_offset += riter->u.a.dim[i].s.start
                             * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              data_offset += riter->u.a.dim[i].s.start * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              /* This and OPEN_START are mapped to a RANGE and therefore can
               * not occur here. */
            case CAF_ARR_REF_OPEN_START:
            default:
              caf_internal_error(unknownarrreftype, stat, NULL, 0);
              return;
          }
          dprint("i = %zd, array_ref = %s, delta = %ld\n", i,
                 caf_array_ref_str[array_ref], delta);
          if (delta <= 0)
            return;
          /* Check the various properties of the destination array.
           * Is an array expected and present? */
          if (delta > 1 && dst_rank == 0)
          {
            /* No, an array is required, but not provided. */
            caf_internal_error(extentoutofrange, stat, NULL, 0);
            return;
          }
          /* When dst is an array. */
          if (dst_rank > 0)
          {
            /* Check that dst_cur_dim is valid for dst. Can be superceeded
             * only by scalar data. */
            if (dst_cur_dim >= dst_rank && delta != 1)
            {
              caf_internal_error(rankoutofrange, stat, NULL, 0);
              return;
            }
            /* Do further checks, when the source is not scalar. */
            else if (non_scalar_array_ref_expected)
            {
              /* Check that the extent is not scalar and we are not in an array
               * ref for the dst side. */
              if (!in_array_ref)
              {
                /* Check that this is the non-scalar extent. */
                if (!array_extent_fixed)
                {
                  /* In an array extent now. */
                  in_array_ref = true;
                  /* The dst is not reallocatable, so nothing more to do,
                   * then correct the dim counter. */
                  dst_cur_dim = i;
                }
                else
                {
                  caf_internal_error(doublearrayref, stat, NULL, 0);
                  return;
                }
              }
              /* When the realloc is required, then no extent may have
               * been set. */
              extent_mismatch
                  = realloc_required
                    || (delta != 1
                        && GFC_DESCRIPTOR_EXTENT(dst, dst_cur_dim) != delta);
              /* When it is already known, that a realloc is needed or
               * the extent does not match the needed one. */
              if (realloc_needed || extent_mismatch)
              {
                /* Check whether dst is reallocatable. */
                if (unlikely(!dst_reallocatable))
                {
                  caf_internal_error(nonallocextentmismatch, stat, NULL, 0,
                                     delta,
                                     GFC_DESCRIPTOR_EXTENT(dst, dst_cur_dim));
                  return;
                }
                /* Only report an error, when the extent needs to be modified,
                 * which is not allowed. */
                else if (!dst_reallocatable && extent_mismatch)
                {
                  caf_internal_error(extentoutofrange, stat, NULL, 0);
                  return;
                }
                realloc_needed = true;
              }
              /* Only change the extent when it does not match.  This is to
               * prevent resetting given array bounds. */
              if (extent_mismatch)
              {
                dst->dim[dst_cur_dim].lower_bound = 1;
                dst->dim[dst_cur_dim]._ubound = delta;
                dst->dim[dst_cur_dim]._stride = size;
                if (realloc_required)
                  dst->offset = -1;
              }
            }
            /* Only increase the dim counter, when in an array ref */
            if (in_array_ref && dst_cur_dim < dst_rank)
            {
              /* Mode != CAF_ARR_REF_SINGLE(delta == 1), and no rank
               * reduction */
              if (!(delta == 1 && dst_rank != GFC_DESCRIPTOR_RANK(src)))
                ++dst_cur_dim;
            }
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
        caf_internal_error(unknownreftype, stat, NULL, 0);
        return;
    }
    src_size = riter->item_size;
    riter = riter->next;
  }
  if (size == 0 || src_size == 0)
    return;
  /* Postcondition:
   * - size contains the number of elements to store in the destination array,
   * - src_size gives the size in bytes of each item in the destination array.
   */

  if (realloc_needed)
  {
    if (!array_extent_fixed)
    {
      /* This can happen only, when the result is scalar. */
      for (dst_cur_dim = 0; dst_cur_dim < dst_rank; ++dst_cur_dim)
      {
        dst->dim[dst_cur_dim].lower_bound = 1;
        dst->dim[dst_cur_dim]._ubound = 1;
        dst->dim[dst_cur_dim]._stride = 1;
      }
    }
    dst->base_addr = malloc(size * GFC_DESCRIPTOR_SIZE(dst));
    if (unlikely(dst->base_addr == NULL))
    {
      caf_internal_error(cannotallocdst, stat, NULL, 0,
                         size * GFC_DESCRIPTOR_SIZE(dst));
      return;
    }
  }

  /* Reset the token. */
  mpi_token = (mpi_caf_token_t *)token;
  remote_memptr = mpi_token->memptr;
  dst_index = 0;
#ifdef EXTRA_DEBUG_OUTPUT
  dprint("dst_rank: %d\n", dst_rank);
  for (i = 0; i < dst_rank; ++i)
  {
    dprint("dst_dim[%zd] = (%zd, %zd)\n", i, dst->dim[i].lower_bound,
           dst->dim[i]._ubound);
  }
#endif
  i = 0;
  dprint("get_by_ref() calling get_for_ref.\n");
  get_for_ref(refs, &i, dst_index, mpi_token, dst, mpi_token->desc,
              dst->base_addr, remote_memptr, 0, NULL, 0, dst_kind, src_kind, 0,
              0, 1, stat, global_dynamic_win_rank, memptr_win_rank, false, false
#ifdef GCC_GE_8
              ,
              src_type
#endif
  );
}

static void
put_data(mpi_caf_token_t *token, MPI_Aint offset, void *sr, int dst_type,
         int src_type, int dst_kind, int src_kind, size_t dst_size,
         size_t src_size, size_t num, int *stat, int image_index)
{
  size_t k;
  int ierr;
  MPI_Win win = (token == NULL) ? global_dynamic_win : token->memptr_win;
#ifdef EXTRA_DEBUG_OUTPUT
  if (token)
    dprint("(win: %d, image: %d, offset: %zd) <- %p, "
           "num: %zd, size %zd -> %zd, dst type %d(%d), src type %d(%d)\n",
           win, image_index + 1, offset, sr, num, src_size, dst_size, dst_type,
           dst_kind, src_type, src_kind);
  else
    dprint("(global_win: %x, image: %d, offset: %zd (%zd)) <- %p, "
           "num: %zd, size %zd -> %zd, dst type %d(%d), src type %d(%d)\n",
           win, image_index + 1, offset, offset, sr, num, src_size, dst_size,
           dst_type, dst_kind, src_type, src_kind);
#endif
  if (dst_type == src_type && dst_kind == src_kind)
  {
    size_t sz = (dst_size > src_size ? src_size : dst_size) * num;
    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index, win);
    ierr = MPI_Put(sr, sz, MPI_BYTE, image_index, offset, sz, MPI_BYTE, win);
    CAF_Win_unlock(image_index, win);
    chk_err(ierr);
    dprint("sr[] = %d, num = %zd, num bytes = %zd\n", (int)((char *)sr)[0], num,
           sz);
    if ((dst_type == BT_CHARACTER || src_type == BT_CHARACTER)
        && dst_size > src_size)
    {
      const size_t trans_size = dst_size / dst_kind - src_size / src_kind;
      void *pad = alloca(trans_size * dst_kind);
      if (dst_kind == 1)
      {
        memset((void *)(char *)pad, ' ', trans_size);
      }
      else /* dst_kind == 4. */
      {
        for (k = 0; k < trans_size; ++k)
        {
          ((int32_t *)pad)[k] = (int32_t)' ';
        }
      }
      CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index, win);
      ierr = MPI_Put(pad, trans_size * dst_kind, MPI_BYTE, image_index,
                     offset + (src_size / src_kind) * dst_kind,
                     trans_size * dst_kind, MPI_BYTE, win);
      chk_err(ierr);
      CAF_Win_unlock(image_index, win);
    }
  }
  else if (dst_type == BT_CHARACTER && dst_kind == 1)
  {
    /* Get the required amount of memory on the stack. */
    void *dsh = alloca(dst_size);
    assign_char1_from_char4(dst_size, src_size, dsh, sr);
    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index, win);
    ierr = MPI_Put(dsh, dst_size, MPI_BYTE, image_index, offset, dst_size,
                   MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
  }
  else if (dst_type == BT_CHARACTER)
  {
    /* Get the required amount of memory on the stack. */
    void *dsh = alloca(dst_size);
    assign_char4_from_char1(dst_size, src_size, dsh, sr);
    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index, win);
    ierr = MPI_Put(dsh, dst_size, MPI_BYTE, image_index, offset, dst_size,
                   MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
  }
  else
  {
    /* Get the required amount of memory on the stack. */
    void *dsh = alloca(dst_size * num), *dsh_iter = dsh;
    dprint("type/kind convert %zd items: "
           "type %d(%d) -> type %d(%d), local buffer: %p\n",
           num, src_type, src_kind, dst_type, dst_kind, dsh);
    for (k = 0; k < num; ++k)
    {
      convert_type(dsh_iter, dst_type, dst_kind, sr, src_type, src_kind, stat);
      dsh_iter += dst_size;
      sr += src_size;
    }
    // dprint("dsh[0] = %d\n", ((int *)dsh)[0]);
    CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image_index, win);
    ierr = MPI_Put(dsh, dst_size * num, MPI_BYTE, image_index, offset,
                   dst_size * num, MPI_BYTE, win);
    chk_err(ierr);
    CAF_Win_unlock(image_index, win);
  }
}

static void
send_for_ref(caf_reference_t *ref, size_t *i, size_t src_index,
             mpi_caf_token_t *mpi_token, gfc_descriptor_t *dst,
             gfc_descriptor_t *src, void *ds, void *sr,
             ptrdiff_t dst_byte_offset, void *rdesc, ptrdiff_t desc_byte_offset,
             int dst_kind, int src_kind, size_t dst_dim, size_t src_dim,
             size_t num, int *stat, int global_dynamic_win_rank,
             int memptr_win_rank,
             bool ds_global,  /* access ds through global_dynamic_win */
             bool desc_global /* access desc through global_dynamic_win */
#ifdef GCC_GE_8
             ,
             int dst_type)
{
#else
)
{
  int dst_type = -1;
#endif
  ptrdiff_t extent_dst = 1, array_offset_dst = 0, dst_stride, src_stride;
  size_t next_dst_dim, ref_rank;
  gfc_max_dim_descriptor_t dst_desc_data;
  caf_ref_type_t ref_type = ref->type;
  caf_array_ref_t array_ref_dst = ref_type != CAF_REF_COMPONENT
                                      ? ref->u.a.mode[dst_dim]
                                      : CAF_ARR_REF_NONE;
  int ierr;

  if (unlikely(ref == NULL))
  {
    /* May be we should issue an error here, because this case should not
     * occur. */
    return;
  }

  dprint("Entering send_for_ref: [i = %zd], %s, arr_ref_type: %s, src_index "
         "= %zd, dst_offset = %zd, rdesc = %p, desc_offset = %zd, ds_glb = %d, "
         "desc_glb "
         "= %d, src_desc = %p, dst_desc = %p, ds = %p\n",
         *i, caf_ref_type_str[ref_type], caf_array_ref_str[array_ref_dst],
         src_index, dst_byte_offset, rdesc, desc_byte_offset, ds_global,
         desc_global, src, dst, ds);

  if (ref->next == NULL)
  {
    size_t src_size = GFC_DESCRIPTOR_SIZE(src);
    dprint("[next == NULL]: src_size = %zd, ref_type = %s\n", src_size,
           caf_ref_type_str[ref_type]);

    switch (ref_type)
    {
      case CAF_REF_COMPONENT:
        dst_byte_offset += ref->u.c.offset;
        if (ref->u.c.caf_token_offset > 0)
        {
          if (ds_global)
          {
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&ds, stdptr_size, MPI_BYTE, global_dynamic_win_rank,
                           MPI_Aint_add((MPI_Aint)ds, dst_byte_offset),
                           stdptr_size, MPI_BYTE, global_dynamic_win);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
            chk_err(ierr);
            desc_global = true;
          }
          else
          {
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&ds, stdptr_size, MPI_BYTE, memptr_win_rank,
                           dst_byte_offset, stdptr_size, MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            ds_global = true;
          }
          dst_byte_offset = 0;
        }

        if (ds_global)
        {
          put_data(NULL, MPI_Aint_add((MPI_Aint)ds, dst_byte_offset), sr,
#ifdef GCC_GE_8
                   dst_type,
#else
                   GFC_DESCRIPTOR_TYPE(src),
#endif
                   GFC_DESCRIPTOR_TYPE(src), dst_kind, src_kind, ref->item_size,
                   src_size, 1, stat, global_dynamic_win_rank);
        }
        else
        {
          put_data(mpi_token, dst_byte_offset, sr,
#ifdef GCC_GE_8
                   dst_type,
#else
                   GFC_DESCRIPTOR_TYPE(dst),
#endif
                   GFC_DESCRIPTOR_TYPE(src), dst_kind, src_kind, ref->item_size,
                   src_size, 1, stat, memptr_win_rank);
        }
        ++(*i);
        return;
      case CAF_REF_STATIC_ARRAY:
        dst_type = ref->u.a.static_array_type;
        /* Intentionally fall through. */
      case CAF_REF_ARRAY:
        if (array_ref_dst == CAF_ARR_REF_NONE)
        {
          if (ds_global)
          {
            put_data(NULL, MPI_Aint_add((MPI_Aint)ds, dst_byte_offset),
                     sr + src_index * src_size,
#ifdef GCC_GE_8
                     dst_type, GFC_DESCRIPTOR_TYPE(src),
#else
                     GFC_DESCRIPTOR_TYPE(dst),
                     (dst_type == -1) ? GFC_DESCRIPTOR_TYPE(src) : dst_type,
#endif
                     dst_kind, src_kind, ref->item_size, src_size, num, stat,
                     global_dynamic_win_rank);
          }
          else
          {
            put_data(mpi_token, dst_byte_offset, sr + src_index * src_size,
#ifdef GCC_GE_8
                     dst_type, GFC_DESCRIPTOR_TYPE(src),
#else
                     GFC_DESCRIPTOR_TYPE(dst),
                     (dst_type == -1) ? GFC_DESCRIPTOR_TYPE(src) : dst_type,
#endif
                     dst_kind, src_kind, ref->item_size, src_size, num, stat,
                     memptr_win_rank);
          }
          *i += num;
          return;
        }
        break;
      default:
        caf_runtime_error(unreachable);
    }
  }

  dprint("num = %zd, src_dim = %zd, dst_dim = %zd, "
         "ref_mode = %s, array_ref_type = %s, ds = %p\n",
         num, src_dim, dst_dim, caf_ref_type_str[ref_type],
         caf_array_ref_str[array_ref_dst], ds);

  switch (ref_type)
  {
    case CAF_REF_COMPONENT:
      dst_byte_offset += ref->u.c.offset;
      if (ref->u.c.caf_token_offset > 0)
      {
        desc_byte_offset = dst_byte_offset;
        rdesc = ds;
        if (ds_global)
        {
          CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                       global_dynamic_win);
          ierr = MPI_Get(&ds, stdptr_size, MPI_BYTE, global_dynamic_win_rank,
                         MPI_Aint_add((MPI_Aint)ds, dst_byte_offset),
                         stdptr_size, MPI_BYTE, global_dynamic_win);
          CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
          chk_err(ierr);
          desc_global = true;
        }
        else
        {
          CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank, mpi_token->memptr_win);
          ierr = MPI_Get(&ds, stdptr_size, MPI_BYTE, memptr_win_rank,
                         dst_byte_offset, stdptr_size, MPI_BYTE,
                         mpi_token->memptr_win);
          chk_err(ierr);
          CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
          ds_global = true;
        }
        dst_byte_offset = 0;
      }
      else
      {
        desc_byte_offset += ref->u.c.offset;
      }
      send_for_ref(ref->next, i, src_index, mpi_token, dst, src, ds, sr,
                   dst_byte_offset, rdesc, desc_byte_offset, dst_kind, src_kind,
                   0, 0, 1, stat, global_dynamic_win_rank, memptr_win_rank,
                   ds_global, desc_global
#ifdef GCC_GE_8
                   ,
                   dst_type
#endif
      );
      return;
    case CAF_REF_ARRAY:
      if (array_ref_dst == CAF_ARR_REF_NONE)
      {
        send_for_ref(ref->next, i, src_index, mpi_token, dst, src, ds, sr,
                     dst_byte_offset, rdesc, desc_byte_offset, dst_kind,
                     src_kind, dst_dim, 0, 1, stat, global_dynamic_win_rank,
                     memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                     ,
                     dst_type
#endif
        );
        return;
      }
      /* Only when on the left most index switch the data pointer to
       * the array's data pointer. */
      if (src_dim == 0)
      {
        if (ds_global)
        {
          for (ref_rank = 0; ref->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          /* Get the remote descriptor. */
          if (desc_global)
          {
            MPI_Aint disp = MPI_Aint_add((MPI_Aint)rdesc, desc_byte_offset);
            dprint("remote desc fetch from %p, offset = %td, aggreg. = %ld\n",
                   rdesc, desc_byte_offset, disp);
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&dst_desc_data, sizeof_desc_for_rank(ref_rank),
                           MPI_BYTE, global_dynamic_win_rank, disp,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           global_dynamic_win);
            chk_err(ierr);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
          }
          else
          {
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&dst_desc_data, sizeof_desc_for_rank(ref_rank),
                           MPI_BYTE, memptr_win_rank, desc_byte_offset,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            desc_global = true;
          }
          dst = (gfc_descriptor_t *)&dst_desc_data;
        }
        else
        {
          dst = mpi_token->desc;
        }
        dst_byte_offset = 0;
        desc_byte_offset = 0;
#ifdef EXTRA_DEBUG_OUTPUT
        dprint("remote desc rank: %d (ref_rank: %zd)\n",
               GFC_DESCRIPTOR_RANK(dst), ref_rank);
        for (int r = 0; r < GFC_DESCRIPTOR_RANK(dst); ++r)
        {
          dprint("remote desc dim[%d] = (lb=%zd, ub=%zd, stride=%zd), "
                 "ref-type: %s\n",
                 r, dst->dim[r].lower_bound, dst->dim[r]._ubound,
                 dst->dim[r]._stride, caf_array_ref_str[ref->u.a.mode[r]]);
        }
#endif
      }
      switch (array_ref_dst)
      {
        case CAF_ARR_REF_VECTOR:
          extent_dst = GFC_DESCRIPTOR_EXTENT(dst, dst_dim);
          array_offset_dst = 0;
          for (size_t idx = 0; idx < ref->u.a.dim[dst_dim].v.nvec; ++idx)
          {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst                                                           \
        = (((ptrdiff_t)((type *)ref->u.a.dim[dst_dim].v.vector)[idx])          \
           - dst->dim[dst_dim].lower_bound * dst->dim[dst_dim]._stride);       \
    break

            switch (ref->u.a.dim[dst_dim].v.kind)
            {
              KINDCASE(1, int8_t);
              KINDCASE(2, int16_t);
              KINDCASE(4, int32_t);
              KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
              KINDCASE(16, __int128);
#endif
              default:
                caf_runtime_error(unreachable);
                return;
            }
#undef KINDCASE

            dprint("vector-index computed to: %zd\n", array_offset_dst);
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += dst->dim[dst_dim]._stride;
          }
          return;
        case CAF_ARR_REF_FULL:
          COMPUTE_NUM_ITEMS(extent_dst, ref->u.a.dim[dst_dim].s.stride,
                            dst->dim[dst_dim].lower_bound,
                            dst->dim[dst_dim]._ubound);
          dst_stride
              = dst->dim[dst_dim]._stride * ref->u.a.dim[dst_dim].s.stride;
          array_offset_dst = 0;
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          dprint("CAF_ARR_REF_FULL: src_stride = %zd, dst_stride = %zd\n",
                 src_stride, dst_stride);
          for (ptrdiff_t idx = 0; idx < extent_dst;
               ++idx, array_offset_dst += dst_stride)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
          }
          return;

        case CAF_ARR_REF_RANGE:
          COMPUTE_NUM_ITEMS(extent_dst, ref->u.a.dim[dst_dim].s.stride,
                            ref->u.a.dim[dst_dim].s.start,
                            ref->u.a.dim[dst_dim].s.end);
          array_offset_dst
              = (ref->u.a.dim[dst_dim].s.start - dst->dim[dst_dim].lower_bound)
                * dst->dim[dst_dim]._stride;
          dst_stride
              = dst->dim[dst_dim]._stride * ref->u.a.dim[dst_dim].s.stride;
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          /* Increase the dst_dim only, when the src_extent is greater than one
           * or src and dst extent are both one. Don't increase when the
           * scalar source is not present in the dst. */
          next_dst_dim = ((extent_dst > 1)
                          || (GFC_DESCRIPTOR_EXTENT(src, src_dim) == 1
                              && extent_dst == 1))
                             ? (dst_dim + 1)
                             : dst_dim;
          for (ptrdiff_t idx = 0; idx < extent_dst; ++idx)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, next_dst_dim, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
            array_offset_dst += dst_stride;
          }
          return;

        case CAF_ARR_REF_SINGLE:
          array_offset_dst
              = (ref->u.a.dim[dst_dim].s.start - dst->dim[dst_dim].lower_bound)
                * dst->dim[dst_dim]._stride;
          send_for_ref(
              ref, i, src_index, mpi_token, dst, src, ds, sr,
              dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
              desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
              src_kind, dst_dim + 1, src_dim + 1, 1, stat,
              global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
              ,
              dst_type
#endif
          );

          return;
        case CAF_ARR_REF_OPEN_END:
          COMPUTE_NUM_ITEMS(extent_dst, ref->u.a.dim[dst_dim].s.stride,
                            ref->u.a.dim[dst_dim].s.start,
                            dst->dim[dst_dim]._ubound);
          dst_stride
              = dst->dim[dst_dim]._stride * ref->u.a.dim[dst_dim].s.stride;
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          array_offset_dst
              = (ref->u.a.dim[dst_dim].s.start - dst->dim[dst_dim].lower_bound)
                * dst->dim[dst_dim]._stride;
          for (ptrdiff_t idx = 0; idx < extent_dst; ++idx)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
            array_offset_dst += dst_stride;
          }
          return;
        case CAF_ARR_REF_OPEN_START:
          COMPUTE_NUM_ITEMS(extent_dst, ref->u.a.dim[dst_dim].s.stride,
                            dst->dim[dst_dim].lower_bound,
                            ref->u.a.dim[dst_dim].s.end);
          dst_stride
              = dst->dim[dst_dim]._stride * ref->u.a.dim[dst_dim].s.stride;
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          array_offset_dst = 0;
          for (ptrdiff_t idx = 0; idx < extent_dst; ++idx)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
            array_offset_dst += dst_stride;
          }
          return;
        default:
          caf_runtime_error(unreachable);
      }
      return;
    case CAF_REF_STATIC_ARRAY:
      if (array_ref_dst == CAF_ARR_REF_NONE)
      {
        send_for_ref(ref->next, i, src_index, mpi_token, dst, src, ds, sr,
                     dst_byte_offset, rdesc, desc_byte_offset, dst_kind,
                     src_kind, dst_dim, 0, 1, stat, global_dynamic_win_rank,
                     memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                     ,
                     dst_type
#endif
        );
        return;
      }
      switch (array_ref_dst)
      {
        case CAF_ARR_REF_VECTOR:
          array_offset_dst = 0;
          for (size_t idx = 0; idx < ref->u.a.dim[dst_dim].v.nvec; ++idx)
          {
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    array_offset_dst = ((type *)ref->u.a.dim[dst_dim].v.vector)[idx];          \
    break

            switch (ref->u.a.dim[dst_dim].v.kind)
            {
              KINDCASE(1, int8_t);
              KINDCASE(2, int16_t);
              KINDCASE(4, int32_t);
              KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
              KINDCASE(16, __int128);
#endif
              default:
                caf_runtime_error(unreachable);
                return;
            }
#undef KINDCASE

            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src->dim[src_dim]._stride;
          }
          return;
        case CAF_ARR_REF_FULL:
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          for (array_offset_dst = 0;
               array_offset_dst <= ref->u.a.dim[dst_dim].s.end;
               array_offset_dst += ref->u.a.dim[dst_dim].s.stride)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
          }
          return;
        case CAF_ARR_REF_RANGE:
          COMPUTE_NUM_ITEMS(extent_dst, ref->u.a.dim[dst_dim].s.stride,
                            ref->u.a.dim[dst_dim].s.start,
                            ref->u.a.dim[dst_dim].s.end);
          src_stride
              = (GFC_DESCRIPTOR_RANK(src) > 0) ? src->dim[src_dim]._stride : 0;
          array_offset_dst = ref->u.a.dim[dst_dim].s.start;
          for (ptrdiff_t idx = 0; idx < extent_dst; ++idx)
          {
            send_for_ref(
                ref, i, src_index, mpi_token, dst, src, ds, sr,
                dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
                desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
                src_kind, dst_dim + 1, src_dim + 1, 1, stat,
                global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
                ,
                dst_type
#endif
            );
            src_index += src_stride;
            array_offset_dst += ref->u.a.dim[dst_dim].s.stride;
          }
          return;
        case CAF_ARR_REF_SINGLE:
          array_offset_dst = ref->u.a.dim[dst_dim].s.start;
          send_for_ref(
              ref, i, src_index, mpi_token, dst, src, ds, sr,
              dst_byte_offset + array_offset_dst * ref->item_size, rdesc,
              desc_byte_offset + array_offset_dst * ref->item_size, dst_kind,
              src_kind, dst_dim + 1, src_dim + 1, 1, stat,
              global_dynamic_win_rank, memptr_win_rank, ds_global, desc_global
#ifdef GCC_GE_8
              ,
              dst_type
#endif
          );
          return;
          /* The OPEN_* are mapped to a RANGE and therefore can not occur. */
        case CAF_ARR_REF_OPEN_END:
        case CAF_ARR_REF_OPEN_START:
        default:
          caf_runtime_error(unreachable);
      }
      return;
    default:
      caf_runtime_error(unreachable);
  }
}

void
PREFIX(send_by_ref)(caf_token_t token, int image_index, gfc_descriptor_t *src,
                    caf_reference_t *refs, int dst_kind, int src_kind,
                    bool may_require_tmp, bool dst_reallocatable, int *stat
#ifdef GCC_GE_8
                    ,
                    int dst_type
#endif
)
{
  const char vecrefunknownkind[]
      = "libcaf_mpi::caf_send_by_ref(): unknown kind in vector-ref.\n";
  const char unknownreftype[]
      = "libcaf_mpi::caf_send_by_ref(): unknown reference type.\n";
  const char unknownarrreftype[]
      = "libcaf_mpi::caf_send_by_ref(): unknown array reference type.\n";
  const char rankoutofrange[]
      = "libcaf_mpi::caf_send_by_ref(): rank out of range.\n";
  const char extentoutofrange[]
      = "libcaf_mpi::caf_send_by_ref(): extent out of range.\n";
  const char cannotallocdst[]
      = "libcaf_mpi::caf_send_by_ref(): can not allocate %d bytes of memory.\n";
  const char unabletoallocdst[]
      = "libcaf_mpi::caf_send_by_ref(): "
        "unable to allocate memory on remote image.\n";
  const char nonallocextentmismatch[]
      = "libcaf_mpi::caf_send_by_ref(): "
        "extent of non-allocatable arrays mismatch (%lu != %lu).\n";

  size_t size, i, ref_rank = 0, src_index, dst_size;
  int dst_rank = -1, src_cur_dim = 0, ierr;
  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *)token;
  void *remote_memptr = mpi_token->memptr, *remote_base_memptr = NULL;
  gfc_max_dim_descriptor_t dst_desc, temp_src;
  gfc_descriptor_t *dst = (gfc_descriptor_t *)&dst_desc;
  caf_reference_t *riter = refs;
  long delta;
  ptrdiff_t data_offset = 0, desc_offset = 0;
  /* Reallocation of data on remote is needed (e.g., array to small).  This is
   * used for error tracking only.  It is not (yet) possible to allocate memory
   * on the remote image. */
  bool realloc_dst = false, extent_mismatch = false;
  /* Set when remote data is to be accessed through the
   * global dynamic window. */
  bool access_data_through_global_win = false;
  /* Set when the remote descriptor is to accessed through the global window. */
  bool access_desc_through_global_win = false;
  bool free_temp_src = false;
  caf_array_ref_t array_ref;
#ifdef EXTRA_DEBUG_OUTPUT
  bool desc_seen = false;
#endif

  if (stat)
    *stat = 0;

  MPI_Group current_team_group, win_group;
  int global_dynamic_win_rank, memptr_win_rank;
  ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(global_dynamic_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){image_index - 1}, win_group,
                                   &global_dynamic_win_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(mpi_token->memptr_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){image_index - 1}, win_group,
                                   &memptr_win_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&current_team_group);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);

  check_image_health(global_dynamic_win_rank, stat);

#ifdef GCC_GE_8
  dprint("Entering send_by_ref(may_require_tmp = %d, dst_type = %d)\n",
         may_require_tmp, dst_type);
#else
  dprint("Entering send_by_ref(may_require_tmp = %d)\n", may_require_tmp);
#endif

  /* Compute the size of the result.  In the beginning size just counts the
   * number of elements. */
  size = 1;
  while (riter)
  {
    dprint("remote_image = %d, offset = %zd, remote_mem = %p, ref: %s, "
           "global_win(data, desc) = (%d, %d)\n",
           global_dynamic_win_rank, data_offset, remote_memptr,
           caf_ref_type_str[riter->type], access_data_through_global_win,
           access_desc_through_global_win);
    switch (riter->type)
    {
      case CAF_REF_COMPONENT:
        data_offset += riter->u.c.offset;
        if (riter->u.c.caf_token_offset > 0)
        {
          remote_base_memptr = remote_memptr;
          if (access_data_through_global_win)
          {
            dprint("remote_memptr(old) = %p, offset = %zd\n",
                   remote_base_memptr, data_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           global_dynamic_win_rank,
                           MPI_Aint_add((MPI_Aint)remote_memptr, data_offset),
                           stdptr_size, MPI_BYTE, global_dynamic_win);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
            dprint("remote_memptr(new) = %p\n", remote_memptr);
            chk_err(ierr);
            /* On the second indirection access also the remote descriptor
             * using the global window. */
            access_desc_through_global_win = true;
          }
          else
          {
            dprint("remote_memptr(old) = %p, offset = %zd\n",
                   remote_base_memptr, data_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           memptr_win_rank, data_offset, stdptr_size, MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            /* All future access is through the global dynamic window. */
            access_data_through_global_win = true;
          }
          desc_offset = data_offset;
          data_offset = 0;
        }
        else
        {
          desc_offset += riter->u.c.offset;
        }
        dprint("comp-ref done.");
        break;
      case CAF_REF_ARRAY:
        /* When there has been no CAF_REF_COMP before hand, then the descriptor
         * is stored in the token and the extends are the same on all images,
         * which is taken care of in the else part. */
        if (access_data_through_global_win)
        {
          for (ref_rank = 0; riter->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          /* Get the remote descriptor and use the stack to store it
           * Note, dst may be pointing to mpi_token->desc therefore it
           * needs to be reset here. */
          dst = (gfc_descriptor_t *)&dst_desc;
          if (access_desc_through_global_win)
          {
            dprint("remote desc fetch from %p, offset = %zd, aggreg = %p\n",
                   remote_base_memptr, desc_offset,
                   remote_base_memptr + desc_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, global_dynamic_win_rank,
                         global_dynamic_win);
            ierr = MPI_Get(
                dst, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                global_dynamic_win_rank,
                MPI_Aint_add((MPI_Aint)remote_base_memptr, desc_offset),
                sizeof_desc_for_rank(ref_rank), MPI_BYTE, global_dynamic_win);
            chk_err(ierr);
            CAF_Win_unlock(global_dynamic_win_rank, global_dynamic_win);
          }
          else
          {
            dprint("remote desc fetch from win %d, offset = %zd\n",
                   mpi_token->memptr_win, desc_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_win_rank,
                         mpi_token->memptr_win);
            ierr = MPI_Get(dst, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           memptr_win_rank, desc_offset,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_win_rank, mpi_token->memptr_win);
            access_desc_through_global_win = true;
          }
        }
        else
          dst = mpi_token->desc;
#ifdef EXTRA_DEBUG_OUTPUT
        desc_seen = true;
        dprint("remote desc rank: %d (ref_rank: %zd)\n",
               GFC_DESCRIPTOR_RANK(dst), ref_rank);
        for (i = 0; i < GFC_DESCRIPTOR_RANK(dst); ++i)
        {
          dprint("remote desc dim[%zd] = (lb=%td, ub=%td, stride=%td)\n", i,
                 dst->dim[i].lower_bound, dst->dim[i]._ubound,
                 dst->dim[i]._stride);
        }
#endif
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    remote_memptr += (((ptrdiff_t)((type *)riter->u.a.dim[i].v.vector)[0])     \
                      - src->dim[i].lower_bound)                               \
                     * src->dim[i]._stride * riter->item_size;                 \
    break
              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_internal_error(vecrefunknownkind, stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                dst->dim[i].lower_bound, dst->dim[i]._ubound);
              /* The memptr stays unchanged when ref'ing the first element in
               * a dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - dst->dim[i].lower_bound)
                     * dst->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              remote_memptr
                  += (riter->u.a.dim[i].s.start - dst->dim[i].lower_bound)
                     * dst->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start, dst->dim[i]._ubound);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - dst->dim[i].lower_bound)
                     * dst->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_START:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                dst->dim[i].lower_bound,
                                riter->u.a.dim[i].s.end);
              /* The memptr stays unchanged when ref'ing the first element in
               * a dimension. */
              break;
            default:
              caf_internal_error(unknownarrreftype, stat, NULL, 0);
              return;
          } // switch
          dprint("i = %zd, array_ref = %s, delta = %ld\n", i,
                 caf_array_ref_str[array_ref], delta);
          if (delta <= 0)
            return;
          if (dst != NULL)
            dst_rank = GFC_DESCRIPTOR_RANK(dst);
          /* Check the various properties of the destination array.
           * Is an array expected and present? */
          if (delta > 1 && dst_rank == 0)
          {
            /* No, an array is required, but not provided. */
            caf_internal_error(extentoutofrange, stat, NULL, 0);
            return;
          }
          /* When dst is an array. */
          if (dst_rank > 0)
          {
            /* Check that src_cur_dim is valid for dst. Can be superceeded
             * only by scalar data. */
            if (src_cur_dim >= dst_rank && delta != 1)
            {
              caf_internal_error(rankoutofrange, stat, NULL, 0);
              return;
            }
            /* Do further checks, when the source is not scalar. */
            else if (delta != 1)
            {
              /* When the realloc is required, then no extent may have
               * been set. */
              extent_mismatch = GFC_DESCRIPTOR_EXTENT(dst, src_cur_dim) < delta;
              /* When it already known, that a realloc is needed or
               * the extent does not match the needed one. */
              if (realloc_dst || extent_mismatch)
              {
                /* Check whether dst is reallocatable. */
                if (unlikely(!dst_reallocatable))
                {
                  caf_internal_error(nonallocextentmismatch, stat, NULL, 0,
                                     delta,
                                     GFC_DESCRIPTOR_EXTENT(dst, src_cur_dim));
                  return;
                }
                /* Only report an error, when the extent needs to be
                 * modified, which is not allowed. */
                else if (!dst_reallocatable && extent_mismatch)
                {
                  caf_internal_error(extentoutofrange, stat, NULL, 0);
                  return;
                }
                dprint("extent(dst, %d): %zd != delta: %ld.\n", src_cur_dim,
                       GFC_DESCRIPTOR_EXTENT(dst, src_cur_dim), delta);
                realloc_dst = true;
              }
            }

            if (src_cur_dim < GFC_DESCRIPTOR_RANK(src))
              ++src_cur_dim;
          }
          size *= (ptrdiff_t)delta;
        }
        break;
      case CAF_REF_STATIC_ARRAY:
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    data_offset += ((type *)riter->u.a.dim[i].v.vector)[0] * riter->item_size; \
    break

              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_internal_error(vecrefunknownkind, stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              delta = riter->u.a.dim[i].s.end / riter->u.a.dim[i].s.stride + 1;
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              data_offset += riter->u.a.dim[i].s.start
                             * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              data_offset += riter->u.a.dim[i].s.start * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              /* This and OPEN_START are mapped to a RANGE and therefore
               * can not occur here. */
            case CAF_ARR_REF_OPEN_START:
            default:
              caf_internal_error(unknownarrreftype, stat, NULL, 0);
              return;
          } // switch
          if (delta <= 0)
            return;
          if (dst != NULL)
            dst_rank = GFC_DESCRIPTOR_RANK(dst);
          /* Check the various properties of the destination array.
           * Is an array expected and present? */
          if (delta > 1 && dst_rank == 0)
          {
            /* No, an array is required, but not provided. */
            caf_internal_error(extentoutofrange, stat, NULL, 0);
            return;
          }
          /* When dst is an array. */
          if (dst_rank > 0)
          {
            /* Check that src_cur_dim is valid for dst. Can be superceeded
             * only by scalar data. */
            if (src_cur_dim >= dst_rank && delta != 1)
            {
              caf_internal_error(rankoutofrange, stat, NULL, 0);
              return;
            }
            /* Do further checks, when the source is not scalar. */
            else if (delta != 1)
            {
              /* When the realloc is required, then no extent may have
               * been set. */
              extent_mismatch = GFC_DESCRIPTOR_EXTENT(dst, src_cur_dim) < delta;
              /* When it is already known, that a realloc is needed or
               * the extent does not match the needed one. */
              if (realloc_dst || extent_mismatch)
              {
                caf_internal_error(unabletoallocdst, stat, NULL, 0);
                return;
              }
            }
            if (src_cur_dim < GFC_DESCRIPTOR_RANK(src)
                && array_ref != CAF_ARR_REF_SINGLE)
              ++src_cur_dim;
          }
          size *= (ptrdiff_t)delta;
        }
        break;
      default:
        caf_internal_error(unknownreftype, stat, NULL, 0);
        return;
    }
    dst_size = riter->item_size;
    riter = riter->next;
  }
  if (size == 0 || dst_size == 0)
    return;
  /* Postcondition:
   * - size contains the number of elements to store in the destination array,
   * - dst_size gives the size in bytes of each item in the destination array.
   */

  if (realloc_dst)
  {
    caf_internal_error(unabletoallocdst, stat, NULL, 0);
    return;
  }

  /* Reset the token. */
  mpi_token = (mpi_caf_token_t *)token;
  remote_memptr = mpi_token->memptr;
  src_index = 0;
#ifdef EXTRA_DEBUG_OUTPUT
  if (desc_seen)
  {
    dprint("dst_rank: %d\n", GFC_DESCRIPTOR_RANK(dst));
    for (i = 0; i < GFC_DESCRIPTOR_RANK(dst); ++i)
    {
      dprint("dst_dim[%zd] = (%td, %td)\n", i, dst->dim[i].lower_bound,
             dst->dim[i]._ubound);
    }
  }
#endif
  /* When accessing myself and may_require_tmp is set, then copy the source
   * array. */
  if (caf_this_image == image_index && may_require_tmp)
  {
    dprint("preparing temporary source.\n");
    memcpy(&temp_src, src, sizeof_desc_for_rank(GFC_DESCRIPTOR_RANK(src)));
    size_t cap = 0;
    for (int r = 0; r < GFC_DESCRIPTOR_RANK(src); ++r)
    {
      cap += GFC_DESCRIPTOR_EXTENT(src, r);
    }

    cap *= GFC_DESCRIPTOR_SIZE(src);
    temp_src.base.base_addr = alloca(cap);
    if ((free_temp_src = (temp_src.base.base_addr == NULL)))
    {
      temp_src.base.base_addr = malloc(cap);
      if (temp_src.base.base_addr == NULL)
      {
        caf_internal_error(cannotallocdst, stat, NULL, cap);
        return;
      }
    }
    memcpy(temp_src.base.base_addr, src->base_addr, cap);
    src = (gfc_descriptor_t *)&temp_src;
  }

  i = 0;
  dprint("calling send_for_ref. num elems: size = %zd, elem size in bytes: "
         "dst_size = %zd\n",
         size, dst_size);
  send_for_ref(refs, &i, src_index, mpi_token, mpi_token->desc, src,
               remote_memptr, src->base_addr, 0, NULL, 0, dst_kind, src_kind, 0,
               0, 1, stat, global_dynamic_win_rank, memptr_win_rank, false,
               false
#ifdef GCC_GE_8
               ,
               dst_type
#endif
  );
  if (free_temp_src)
  {
    free(temp_src.base.base_addr);
  }
}

void
PREFIX(sendget_by_ref)(caf_token_t dst_token, int dst_image_index,
                       caf_reference_t *dst_refs, caf_token_t src_token,
                       int src_image_index, caf_reference_t *src_refs,
                       int dst_kind, int src_kind, bool may_require_tmp,
                       int *dst_stat, int *src_stat
#ifdef GCC_GE_8
                       ,
                       int dst_type, int src_type
#endif
)
{
  const char vecrefunknownkind[]
      = "libcaf_mpi::caf_sendget_by_ref(): unknown kind in vector-ref.\n";
  const char unknownreftype[]
      = "libcaf_mpi::caf_sendget_by_ref(): unknown reference type.\n";
  const char unknownarrreftype[]
      = "libcaf_mpi::caf_sendget_by_ref(): unknown array reference type.\n";
  const char cannotallocdst[] = "libcaf_mpi::caf_sendget_by_ref(): can not "
                                "allocate %d bytes of memory.\n";
  size_t size, i, ref_rank, dst_index, src_index = 0, src_size;
  int dst_rank, ierr;
  mpi_caf_token_t *src_mpi_token = (mpi_caf_token_t *)src_token,
                  *dst_mpi_token = (mpi_caf_token_t *)dst_token;
  void *remote_memptr = src_mpi_token->memptr, *remote_base_memptr = NULL;
  gfc_max_dim_descriptor_t src_desc;
  gfc_max_dim_descriptor_t temp_src_desc;
  gfc_descriptor_t *src = (gfc_descriptor_t *)&src_desc;
  caf_reference_t *riter = src_refs;
  long delta;
  ptrdiff_t data_offset = 0, desc_offset = 0;
  MPI_Group current_team_group, win_group;
  int global_dst_rank, global_src_rank, memptr_dst_rank, memptr_src_rank;
  /* Set when the first non-scalar array reference is encountered. */
  bool in_array_ref = false;
  /* Set when remote data is to be accessed through the
   * global dynamic window. */
  bool access_data_through_global_win = false;
  /* Set when the remote descriptor is to accessed through the global window. */
  bool access_desc_through_global_win = false;
  caf_array_ref_t array_ref;
#ifndef GCC_GE_8
  int dst_type = -1, src_type = -1;
#endif

  if (src_stat)
    *src_stat = 0;

  ierr = MPI_Comm_group(CAF_COMM_WORLD, &current_team_group);
  chk_err(ierr);

  ierr = MPI_Win_get_group(global_dynamic_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){src_image_index - 1}, win_group,
                                   &global_src_rank);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){dst_image_index - 1}, win_group,
                                   &global_dst_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);

  ierr = MPI_Win_get_group(src_mpi_token->memptr_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){src_image_index - 1}, win_group,
                                   &memptr_src_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);
  ierr = MPI_Win_get_group(dst_mpi_token->memptr_win, &win_group);
  chk_err(ierr);
  ierr = MPI_Group_translate_ranks(current_team_group, 1,
                                   (int[]){dst_image_index - 1}, win_group,
                                   &memptr_dst_rank);
  chk_err(ierr);
  ierr = MPI_Group_free(&win_group);
  chk_err(ierr);
  ierr = MPI_Group_free(&current_team_group);
  chk_err(ierr);

  check_image_health(global_src_rank, src_stat);

  dprint("Entering get_by_ref(may_require_tmp = %d, dst_type = %d(%d), "
         "src_type = %d(%d)).\n",
         may_require_tmp, dst_type, dst_kind, src_type, src_kind);

  /* Compute the size of the result.  In the beginning size just counts the
   * number of elements. */
  size = 1;
  while (riter)
  {
    dprint("offset = %zd, remote_mem = %p\n", data_offset, remote_memptr);
    switch (riter->type)
    {
      case CAF_REF_COMPONENT:
        data_offset += riter->u.c.offset;
        if (riter->u.c.caf_token_offset > 0)
        {
          remote_base_memptr = remote_memptr;
          if (access_data_through_global_win)
          {
            CAF_Win_lock(MPI_LOCK_SHARED, global_src_rank, global_dynamic_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           global_src_rank,
                           MPI_Aint_add((MPI_Aint)remote_memptr, data_offset),
                           stdptr_size, MPI_BYTE, global_dynamic_win);
            CAF_Win_unlock(global_src_rank, global_dynamic_win);
            chk_err(ierr);
            /* On the second indirection access also the remote descriptor
             * using the global window. */
            access_desc_through_global_win = true;
          }
          else
          {
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_src_rank,
                         src_mpi_token->memptr_win);
            ierr = MPI_Get(&remote_memptr, stdptr_size, MPI_BYTE,
                           memptr_src_rank, data_offset, stdptr_size, MPI_BYTE,
                           src_mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_src_rank, src_mpi_token->memptr_win);
            /* All future access is through the global dynamic window. */
            access_data_through_global_win = true;
          }
          desc_offset = data_offset;
          data_offset = 0;
        }
        else
        {
          desc_offset += riter->u.c.offset;
        }
        break;
      case CAF_REF_ARRAY:
        /* When there has been no CAF_REF_COMP before hand, then the
         * descriptor is stored in the token and the extends are the same on all
         * images, which is taken care of in the else part. */
        if (access_data_through_global_win)
        {
          for (ref_rank = 0; riter->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          /* Get the remote descriptor and use the stack to store it. Note,
           * src may be pointing to mpi_token->desc therefore it needs to be
           * reset here. */
          src = (gfc_descriptor_t *)&src_desc;
          if (access_desc_through_global_win)
          {
            dprint("remote desc fetch from %p, offset = %zd\n",
                   remote_base_memptr, desc_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, global_src_rank, global_dynamic_win);
            ierr = MPI_Get(
                src, sizeof_desc_for_rank(ref_rank), MPI_BYTE, global_src_rank,
                MPI_Aint_add((MPI_Aint)remote_base_memptr, desc_offset),
                sizeof_desc_for_rank(ref_rank), MPI_BYTE, global_dynamic_win);
            chk_err(ierr);
            CAF_Win_unlock(global_src_rank, global_dynamic_win);
          }
          else
          {
            dprint("remote desc fetch from win %d, offset = %zd\n",
                   src_mpi_token->memptr_win, desc_offset);
            CAF_Win_lock(MPI_LOCK_SHARED, memptr_src_rank,
                         src_mpi_token->memptr_win);
            ierr = MPI_Get(src, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           memptr_src_rank, desc_offset,
                           sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                           src_mpi_token->memptr_win);
            chk_err(ierr);
            CAF_Win_unlock(memptr_src_rank, src_mpi_token->memptr_win);
            access_desc_through_global_win = true;
          }
        }
        else
        {
          src = src_mpi_token->desc;
        }
#ifdef EXTRA_DEBUG_OUTPUT
        dprint("remote desc rank: %d (ref_rank: %zd)\n",
               GFC_DESCRIPTOR_RANK(src), ref_rank);
        for (i = 0; i < GFC_DESCRIPTOR_RANK(src); ++i)
        {
          dprint("remote desc dim[%zd] = (lb=%td, ub=%td, stride=%td)\n", i,
                 src->dim[i].lower_bound, src->dim[i]._ubound,
                 src->dim[i]._stride);
        }
#endif
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    remote_memptr += (((ptrdiff_t)((type *)riter->u.a.dim[i].v.vector)[0])     \
                      - src->dim[i].lower_bound)                               \
                     * src->dim[i]._stride * riter->item_size;                 \
    break
              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_runtime_error(vecrefunknownkind, src_stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                src->dim[i].lower_bound, src->dim[i]._ubound);
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start, src->dim[i]._ubound);
              remote_memptr
                  += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                     * src->dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_START:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                src->dim[i].lower_bound,
                                riter->u.a.dim[i].s.end);
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            default:
              caf_runtime_error(unknownarrreftype, src_stat, NULL, 0);
              return;
          } // switch
          dprint("i = %zd, array_ref = %s, delta = %ld\n", i,
                 caf_array_ref_str[array_ref], delta);
          if (delta <= 0)
            return;
          size *= (ptrdiff_t)delta;
        }
        if (in_array_ref)
          in_array_ref = false;
        break;
      case CAF_REF_STATIC_ARRAY:
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          switch (array_ref)
          {
            case CAF_ARR_REF_VECTOR:
              delta = riter->u.a.dim[i].v.nvec;
#define KINDCASE(kind, type)                                                   \
  case kind:                                                                   \
    remote_memptr                                                              \
        += ((type *)riter->u.a.dim[i].v.vector)[0] * riter->item_size;         \
    break
              switch (riter->u.a.dim[i].v.kind)
              {
                KINDCASE(1, int8_t);
                KINDCASE(2, int16_t);
                KINDCASE(4, int32_t);
                KINDCASE(8, int64_t);
#ifdef HAVE_GFC_INTEGER_16
                KINDCASE(16, __int128);
#endif
                default:
                  caf_runtime_error(vecrefunknownkind, src_stat, NULL, 0);
                  return;
              }
#undef KINDCASE
              break;
            case CAF_ARR_REF_FULL:
              delta = riter->u.a.dim[i].s.end / riter->u.a.dim[i].s.stride + 1;
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            case CAF_ARR_REF_RANGE:
              COMPUTE_NUM_ITEMS(delta, riter->u.a.dim[i].s.stride,
                                riter->u.a.dim[i].s.start,
                                riter->u.a.dim[i].s.end);
              remote_memptr += riter->u.a.dim[i].s.start
                               * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_SINGLE:
              delta = 1;
              remote_memptr += riter->u.a.dim[i].s.start
                               * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_OPEN_END:
              /* This and OPEN_START are mapped to a RANGE and therefore
               * can not occur here. */
            case CAF_ARR_REF_OPEN_START:
            default:
              caf_runtime_error(unknownarrreftype, src_stat, NULL, 0);
              return;
          } // switch
          dprint("i = %zd, array_ref = %s, delta = %ld\n", i,
                 caf_array_ref_str[array_ref], delta);
          if (delta <= 0)
            return;
          size *= (ptrdiff_t)delta;
        }
        if (in_array_ref)
          in_array_ref = false;
        break;
      default:
        caf_runtime_error(unknownreftype, src_stat, NULL, 0);
        return;
    } // switch
    src_size = riter->item_size;
    riter = riter->next;
  }
  if (size == 0 || src_size == 0)
    return;
  /* Postcondition:
   * - size contains the number of elements to store in the destination array,
   * - src_size gives the size in bytes of each item in the destination array.
   */

  dst_rank = (size > 1) ? 1 : 0;
  memset(&temp_src_desc, 0, sizeof(gfc_dim1_descriptor_t));
#ifdef GCC_GE_8
  temp_src_desc.base.dtype.elem_len
      = (dst_type != BT_COMPLEX) ? dst_kind : (2 * dst_kind);
  temp_src_desc.base.dtype.rank = 1;
  temp_src_desc.base.dtype.type = dst_type;
#else // GCC_GE_7
  temp_src_desc.base.dtype = GFC_DTYPE_INTEGER_4 | 1;
#endif
  temp_src_desc.base.offset = 0;
  temp_src_desc.dim[0]._ubound = size - 1;
  temp_src_desc.dim[0]._stride = 1;

  temp_src_desc.base.base_addr
      = malloc(size * GFC_DESCRIPTOR_SIZE((gfc_descriptor_t *)&temp_src_desc));
  if (unlikely(temp_src_desc.base.base_addr == NULL))
  {
    caf_runtime_error(
        cannotallocdst, src_stat,
        size * GFC_DESCRIPTOR_SIZE((gfc_descriptor_t *)&temp_src_desc));
    return;
  }

#ifndef GCC_GE_8
  static bool warning_given = false;
  if (!warning_given)
  {
    fprintf(stderr,
            "lib_caf_mpi::sendget_by_ref(): Warning !! sendget_by_ref() is "
            "mostly unfunctional due to a design error. Split up your "
            "statement with coarray refs on both sides of the assignment "
            "when the datatype transfered is non 4-byte-integer compatible, "
            "or use gcc >= 8.\n");
    warning_given = true;
  }
#endif
  /* Reset the token. */
  src_mpi_token = (mpi_caf_token_t *)src_token;
  remote_memptr = src_mpi_token->memptr;
  dst_index = 0;
#ifdef EXTRA_DEBUG_OUTPUT
  dprint("dst_rank: %d\n", dst_rank);
  for (i = 0; i < dst_rank; ++i)
  {
    dprint("temp_src_dim[%zd] = (%zd, %zd)\n", i,
           temp_src_desc.dim[i].lower_bound, temp_src_desc.dim[i]._ubound);
  }
#endif
  i = 0;
  dprint("calling get_for_ref.\n");
  get_for_ref(src_refs, &i, dst_index, src_mpi_token,
              (gfc_descriptor_t *)&temp_src_desc, src_mpi_token->desc,
              temp_src_desc.base.base_addr, remote_memptr, 0, NULL, 0, dst_kind,
              src_kind, 0, 0, 1, src_stat, global_src_rank, memptr_src_rank,
              false, false
#ifdef GCC_GE_8
              ,
              src_type
#endif
  );
  dprint("calling send_for_ref. num elems: size = %zd, elem size in bytes: "
         "src_size = %zd\n",
         size, src_size);
  i = 0;

  send_for_ref(dst_refs, &i, src_index, dst_mpi_token, dst_mpi_token->desc,
               (gfc_descriptor_t *)&temp_src_desc, dst_mpi_token->memptr,
               temp_src_desc.base.base_addr, 0, NULL, 0, dst_kind, src_kind, 0,
               0, 1, dst_stat, global_dst_rank, memptr_dst_rank, false, false
#ifdef GCC_GE_8
               ,
               dst_type
#endif
  );
}

int
PREFIX(is_present)(caf_token_t token, int image_index, caf_reference_t *refs)
{
  const char unsupportedRefType[] = "Unsupported ref-type in caf_is_present().";
  const char unexpectedEndOfRefs[]
      = "Unexpected end of references in caf_is_present.";
  const char remotesInnerRefNA[]
      = "Memory referenced on the remote image is not allocated.";
  const int ptr_size = sizeof(void *);
  const int remote_image = image_index - 1;
  mpi_caf_token_t *mpi_token = (mpi_caf_token_t *)token;
  ptrdiff_t local_offset = 0;
  void *remote_memptr = NULL, *remote_base_memptr = NULL;
  bool carryOn = true, firstDesc = true;
  caf_reference_t *riter = refs;
  size_t i, ref_rank;
  int ierr;
  gfc_max_dim_descriptor_t src_desc;
  caf_array_ref_t array_ref;

  while (carryOn && riter)
  {
    switch (riter->type)
    {
      case CAF_REF_COMPONENT:
        if (riter->u.c.caf_token_offset)
        {
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, mpi_token->memptr_win);
          ierr = MPI_Get(&remote_memptr, ptr_size, MPI_BYTE, remote_image,
                         local_offset + riter->u.c.offset, ptr_size, MPI_BYTE,
                         mpi_token->memptr_win);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, mpi_token->memptr_win);
          dprint("Got first remote address %p from offset %zd\n", remote_memptr,
                 local_offset);
          local_offset = 0;
          carryOn = false;
        }
        else
          local_offset += riter->u.c.offset;
        break;
      case CAF_REF_ARRAY:
        {
          const gfc_descriptor_t *src
              = (gfc_descriptor_t *)(mpi_token->memptr + local_offset);
          for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
          {
            array_ref = riter->u.a.mode[i];
            dprint("i = %zd, array_ref = %s\n", i,
                   caf_array_ref_str[array_ref]);
            switch (array_ref)
            {
              case CAF_ARR_REF_FULL:
                /* The local_offset stays unchanged when ref'ing the first
                 * element in a dimension. */
                break;
              case CAF_ARR_REF_SINGLE:
                local_offset
                    += (riter->u.a.dim[i].s.start - src->dim[i].lower_bound)
                       * src->dim[i]._stride * riter->item_size;
                break;
              case CAF_ARR_REF_VECTOR:
              case CAF_ARR_REF_RANGE:
              case CAF_ARR_REF_OPEN_END:
              case CAF_ARR_REF_OPEN_START:
                /* Intentionally fall through, because these are not
                 * suported here. */
              default:
                caf_runtime_error(unsupportedRefType);
                return false;
            }
          }
        }
        break;
      case CAF_REF_STATIC_ARRAY:
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          dprint("i = %zd, array_ref = %s\n", i, caf_array_ref_str[array_ref]);
          switch (array_ref)
          {
            case CAF_ARR_REF_FULL:
              /* The local_offset stays unchanged when ref'ing the first
               * element in a dimension. */
              break;
            case CAF_ARR_REF_SINGLE:
              local_offset += riter->u.a.dim[i].s.start
                              * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_VECTOR:
            case CAF_ARR_REF_RANGE:
            case CAF_ARR_REF_OPEN_END:
            case CAF_ARR_REF_OPEN_START:
            default:
              caf_runtime_error(unsupportedRefType);
              return false;
          }
        }
        break;
      default:
        caf_runtime_error(unsupportedRefType);
        return false;
    } // switch
    riter = riter->next;
  }

  if (carryOn)
  {
    // This can only happen, when riter == NULL.
    caf_runtime_error(unexpectedEndOfRefs);
  }

  if (remote_memptr != NULL)
    remote_base_memptr = remote_memptr + local_offset;

  dprint("Remote desc address is %p from remote memptr %p and offset %zd\n",
         remote_base_memptr, remote_memptr, local_offset);

  while (riter)
  {
    switch (riter->type)
    {
      case CAF_REF_COMPONENT:
        /* After reffing the first allocatable/pointer component, descriptors
         * need to be picked up from the global_win. */
        firstDesc = firstDesc && riter->u.c.caf_token_offset == 0;
        local_offset += riter->u.c.offset;
        remote_base_memptr = remote_memptr + local_offset;
        CAF_Win_lock(MPI_LOCK_SHARED, remote_image, global_dynamic_win);
        ierr = MPI_Get(&remote_memptr, ptr_size, MPI_BYTE, remote_image,
                       (MPI_Aint)remote_base_memptr, ptr_size, MPI_BYTE,
                       global_dynamic_win);
        chk_err(ierr);
        CAF_Win_unlock(remote_image, global_dynamic_win);
        dprint("Got remote address %p from offset %zd nd base memptr %p\n",
               remote_memptr, local_offset, remote_base_memptr);
        local_offset = 0;
        break;
      case CAF_REF_ARRAY:
        if (remote_base_memptr == NULL)
        {
          /* Refing an unallocated array ends in a full_ref. Check that this
           * is true. Error when not full-refing. */
          for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
          {
            if (riter->u.a.mode[i] != CAF_ARR_REF_FULL)
              break;
          }
          if (riter->u.a.mode[i] != CAF_ARR_REF_NONE)
            caf_runtime_error(remotesInnerRefNA);
          break;
        }
        if (firstDesc)
        {
          /* The first descriptor is accessible by the mpi_token->memptr_win.
           * Count the dims to fetch. */
          for (ref_rank = 0; riter->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          dprint("Getting remote descriptor of rank %zd from win: %d, "
                 "sizeof() %zd\n",
                 ref_rank, mpi_token->memptr_win,
                 sizeof_desc_for_rank(ref_rank));
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, mpi_token->memptr_win);
          ierr = MPI_Get(&src_desc, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                         remote_image, local_offset,
                         sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                         mpi_token->memptr_win);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, mpi_token->memptr_win);
          firstDesc = false;
        }
        else
        {
          /* All inner descriptors go by the dynamic window.
           * Count the dims to fetch. */
          for (ref_rank = 0; riter->u.a.mode[ref_rank] != CAF_ARR_REF_NONE;
               ++ref_rank)
            ;
          dprint("Getting remote descriptor of rank %zd from: %p, "
                 "sizeof() %zd\n",
                 ref_rank, remote_base_memptr, sizeof_desc_for_rank(ref_rank));
          CAF_Win_lock(MPI_LOCK_SHARED, remote_image, global_dynamic_win);
          ierr = MPI_Get(&src_desc, sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                         remote_image, (MPI_Aint)remote_base_memptr,
                         sizeof_desc_for_rank(ref_rank), MPI_BYTE,
                         global_dynamic_win);
          chk_err(ierr);
          CAF_Win_unlock(remote_image, global_dynamic_win);
        }
#ifdef EXTRA_DEBUG_OUTPUT
        {
          gfc_descriptor_t *src = (gfc_descriptor_t *)(&src_desc);
          dprint("remote desc rank: %d (ref_rank: %zd)\n",
                 GFC_DESCRIPTOR_RANK(src), ref_rank);
          for (i = 0; i < GFC_DESCRIPTOR_RANK(src); ++i)
          {
            dprint("remote desc dim[%zd] = (lb=%zd, ub=%zd, stride=%zd)\n", i,
                   src_desc.dim[i].lower_bound, src_desc.dim[i]._ubound,
                   src_desc.dim[i]._stride);
          }
        }
#endif

        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          dprint("i = %zd, array_ref = %s\n", i, caf_array_ref_str[array_ref]);
          switch (array_ref)
          {
            case CAF_ARR_REF_FULL:
              /* The local_offset stays unchanged when ref'ing the first
               * element in a dimension. */
              break;
            case CAF_ARR_REF_SINGLE:
              local_offset
                  += (riter->u.a.dim[i].s.start - src_desc.dim[i].lower_bound)
                     * src_desc.dim[i]._stride * riter->item_size;
              break;
            case CAF_ARR_REF_VECTOR:
            case CAF_ARR_REF_RANGE:
            case CAF_ARR_REF_OPEN_END:
            case CAF_ARR_REF_OPEN_START:
              /* Intentionally fall through, because these are not suported
               * here. */
            default:
              caf_runtime_error(unsupportedRefType);
              return false;
          }
        }
        break;
      case CAF_REF_STATIC_ARRAY:
        for (i = 0; riter->u.a.mode[i] != CAF_ARR_REF_NONE; ++i)
        {
          array_ref = riter->u.a.mode[i];
          dprint("i = %zd, array_ref = %s\n", i, caf_array_ref_str[array_ref]);
          switch (array_ref)
          {
            case CAF_ARR_REF_FULL:
              /* The memptr stays unchanged when ref'ing the first element
               * in a dimension. */
              break;
            case CAF_ARR_REF_SINGLE:
              local_offset += riter->u.a.dim[i].s.start
                              * riter->u.a.dim[i].s.stride * riter->item_size;
              break;
            case CAF_ARR_REF_VECTOR:
            case CAF_ARR_REF_RANGE:
            case CAF_ARR_REF_OPEN_END:
            case CAF_ARR_REF_OPEN_START:
            default:
              caf_runtime_error(unsupportedRefType);
              return false;
          }
        }
        break;
      default:
        caf_runtime_error(unsupportedRefType);
        return false;
    } // switch
    riter = riter->next;
  }

  dprint("Got remote_memptr: %p\n", remote_memptr);
  return remote_memptr != NULL;
}
#endif // GCC_GE_7

/* SYNC IMAGES. Note: SYNC IMAGES(*) is passed as count == -1 while
 * SYNC IMAGES([]) has count == 0. Note further that SYNC IMAGES(*)
 * is not semantically equivalent to SYNC ALL. */

void
PREFIX(sync_images)(int count, int images[], int *stat, char *errmsg,
                    charlen_t errmsg_len)
{
  sync_images_internal(count, images, stat, errmsg, errmsg_len, false);
}

static void
sync_images_internal(int count, int images[], int *stat, char *errmsg,
                     size_t errmsg_len, bool internal)
{
  /* Marked as unused, because of conditional compilation.  */
  int ierr = 0, i = 0, j = 0, int_zero = 0, done_count = 0,
      flag __attribute__((unused));
  MPI_Status s;

#ifdef WITH_FAILED_IMAGES
  no_stopped_images_check_in_errhandler = true;
#endif
  dprint("Entering\n");
  if (count == 0 || (count == 1 && images[0] == caf_this_image))
  {
    if (stat)
      *stat = 0;
#ifdef WITH_FAILED_IMAGES
    no_stopped_images_check_in_errhandler = false;
#endif
    dprint("Leaving early.\n");
    return;
  }

  /* halt execution if sync images contains duplicate image numbers */
  for (i = 0; i < count; ++i)
  {
    for (j = 0; j < i; ++j)
    {
      if (images[i] == images[j])
      {
        ierr = STAT_DUP_SYNC_IMAGES;
        if (stat)
          *stat = ierr;
        goto sync_images_err_chk;
      }
    }
  }

#ifdef GFC_CAF_CHECK
  for (i = 0; i < count; ++i)
  {
    if (images[i] < 1 || images[i] > caf_num_images)
    {
      fprintf(stderr, "COARRAY ERROR: Invalid image index %d to SYNC IMAGES",
              images[i]);
      terminate_internal(1, 1);
    }
  }
#endif

  if (unlikely(caf_is_finalized))
  {
    ierr = STAT_STOPPED_IMAGE;
  }
  else
  {
    if (count == -1)
    {
      count = caf_num_images - 1;
      images = images_full;
    }

#if defined(NONBLOCKING_PUT) && !defined(CAF_MPI_LOCK_UNLOCK)
    explicit_flush();
#endif

#ifdef WITH_FAILED_IMAGES
    /* Provoke detecting process fails. */
    ierr = MPI_Test(&alive_request, &flag, MPI_STATUS_IGNORE);
    chk_err(ierr);
#endif
    /* A rather simple way to synchronice:
     * - expect all images to sync with receiving an int,
     * - on the other side, send all processes to sync with an int,
     * - when the int received is STAT_STOPPED_IMAGE the return immediately,
     *   else wait until all images in the current set of images have send
     *   some data, i.e., synced.
     *
     * This approach as best as possible implements the syncing of different
     * sets of images and figuring that an image has stopped.  MPI does not
     * provide any direct means of syncing non-coherent sets of images.
     * The groups/communicators of MPI always need to be consistent, i.e.,
     * have the same members on all images participating.  This is
     * contradictiory to the sync images statement, where syncing, e.g., in a
     * ring pattern is possible.
     *
     * This implementation guarantees, that as long as no image is stopped
     * an image only is allowed to continue, when all its images to sync to
     * also have reached a sync images statement.  This implementation makes
     * no assumption when the image continues or in which order synced
     * images continue. */
    for (i = 0; i < count; ++i)
    {
      /* Need to have the request handlers contigously in the handlers
       * array or waitany below will trip about the handler as illegal. */
      ierr = MPI_Irecv(&arrived[images[i] - 1], 1, MPI_INT, images[i] - 1,
                       MPI_TAG_CAF_SYNC_IMAGES, CAF_COMM_WORLD,
                       &sync_handles[i]);
      chk_err(ierr);
    }
    for (i = 0; i < count; ++i)
    {
      ierr = MPI_Send(&int_zero, 1, MPI_INT, images[i] - 1,
                      MPI_TAG_CAF_SYNC_IMAGES, CAF_COMM_WORLD);
      chk_err(ierr);
    }
    done_count = 0;
    while (done_count < count)
    {
      ierr = MPI_Waitany(count, sync_handles, &i, &s);
      if (ierr == MPI_SUCCESS && i != MPI_UNDEFINED)
      {
        ++done_count;
        if (ierr == MPI_SUCCESS && arrived[s.MPI_SOURCE] == STAT_STOPPED_IMAGE)
        {
          /* Possible future extension: Abort pending receives.  At the
           * moment the receives are discarded by the program
           * termination.  For the tested mpi-implementation this is ok. */
          ierr = STAT_STOPPED_IMAGE;
          break;
        }
      }
      else if (ierr != MPI_SUCCESS)
#ifdef WITH_FAILED_IMAGES
      {
        int err;
        MPI_Error_class(ierr, &err);
        if (err == MPIX_ERR_PROC_FAILED)
        {
          dprint("Image failed, provoking error handling.\n");
          ierr = STAT_FAILED_IMAGE;
          /* Provoke detecting process fails. */
          MPI_Test(&alive_request, &flag, MPI_STATUS_IGNORE);
        }
        break;
      }
#else
        break;
#endif // WITH_FAILED_IMAGES
    }
  }

sync_images_err_chk:
#ifdef WITH_FAILED_IMAGES
  no_stopped_images_check_in_errhandler = false;
#endif
  dprint("Leaving\n");
  if (stat)
    *stat = ierr;
#ifdef WITH_FAILED_IMAGES
  else if (ierr == STAT_FAILED_IMAGE)
    terminate_internal(ierr, 0);
#endif

  if (ierr != 0 && ierr != STAT_FAILED_IMAGE)
  {
    char msg[80];
    strcpy(msg, "SYNC IMAGES failed");
    if (caf_is_finalized)
      strcat(msg, " - there are stopped images");

    if (errmsg_len > 0)
    {
      size_t len = (strlen(msg) > errmsg_len) ? errmsg_len : strlen(msg);
      memcpy(errmsg, msg, len);
      if (errmsg_len > len)
        memset(&errmsg[len], ' ', errmsg_len - len);
    }
    else if (!internal && stat == NULL)
      caf_runtime_error(msg);
  }
}

#define GEN_REDUCTION(name, datatype, operator)                                \
  static void name(datatype *invec, datatype *inoutvec, int *len,              \
                   MPI_Datatype *datatype __attribute__((unused)))             \
  {                                                                            \
    for (int i = 0; i < len; ++i)                                              \
    {                                                                          \
      operator;                                                                \
    }                                                                          \
  }

#define REFERENCE_FUNC(TYPE) TYPE##_by_reference
#define VALUE_FUNC(TYPE) TYPE##_by_value

#define GEN_COREDUCE(name, dt)                                                 \
  static void name##_by_reference_adapter(void *invec, void *inoutvec,         \
                                          int *len, MPI_Datatype *datatype)    \
  {                                                                            \
    for (int i = 0; i < *len; ++i)                                             \
    {                                                                          \
      *((dt *)inoutvec)                                                        \
          = (dt)(REFERENCE_FUNC(dt)((dt *)invec, (dt *)inoutvec));             \
      invec += sizeof(dt);                                                     \
      inoutvec += sizeof(dt);                                                  \
    }                                                                          \
  }                                                                            \
  static void name##_by_value_adapter(void *invec, void *inoutvec, int *len,   \
                                      MPI_Datatype *datatype)                  \
  {                                                                            \
    for (int i = 0; i < *len; ++i)                                             \
    {                                                                          \
      *((dt *)inoutvec) = (dt)(VALUE_FUNC(dt)(*(dt *)invec, *(dt *)inoutvec)); \
      invec += sizeof(dt);                                                     \
      inoutvec += sizeof(dt);                                                  \
    }                                                                          \
  }

GEN_COREDUCE(redux_int8, int8_t)
GEN_COREDUCE(redux_int16, int16_t)
GEN_COREDUCE(redux_int32, int32_t)
GEN_COREDUCE(redux_int64, int64_t)
GEN_COREDUCE(redux_real32, float)
GEN_COREDUCE(redux_real64, double)

static void
redux_char_by_reference_adapter(void *invec, void *inoutvec, int *len,
                                MPI_Datatype *datatype)
{
  MPI_Aint lb, string_len;
  MPI_Type_get_extent(*datatype, &lb, &string_len);
  for (int i = 0; i < *len; i++)
  {
    /* The length of the result is fixed, i.e., no deferred string length is
     * allowed there. */
    REFERENCE_FUNC(char)
    ((char *)inoutvec, string_len, (char *)invec, (char *)inoutvec, string_len,
     string_len);
    invec += sizeof(char) * string_len;
    inoutvec += sizeof(char) * string_len;
  }
}

#ifndef MPI_INTEGER1
GEN_REDUCTION(do_sum_int1, int8_t, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_int1, int8_t,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_int1, int8_t,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
#endif

/*
#ifndef MPI_INTEGER2
GEN_REDUCTION(do_sum_int1, int16_t, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_int1, int16_t,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_int1, int16_t,
             inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
#endif
*/

#if defined(MPI_INTEGER16) && defined(GFC_INTEGER_16)
GEN_REDUCTION(do_sum_int1, GFC_INTEGER_16, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_int1, GFC_INTEGER_16,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_int1, GFC_INTEGER_16,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
#endif

#if defined(GFC_DTYPE_REAL_10)                                                 \
    || (!defined(GFC_DTYPE_REAL_10) && defined(GFC_DTYPE_REAL_16))
GEN_REDUCTION(do_sum_real10, long double, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_real10, long double,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_real10, long double,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_sum_complex10, _Complex long double, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_complex10, _Complex long double,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_complex10, _Complex long double,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
#endif

#if defined(GFC_DTYPE_REAL_10) && defined(GFC_DTYPE_REAL_16)
GEN_REDUCTION(do_sum_real10, __float128, inoutvec[i] += invec[i])
GEN_REDUCTION(do_min_real10, __float128,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_real10, __float128,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_sum_complex10, _Complex __float128, inoutvec[i] += invec[i])
GEN_REDUCTION(do_mincomplexl10, _Complex __float128,
              inoutvec[i] = (invec[i] >= inoutvec[i] ? inoutvec[i] : invec[i]))
GEN_REDUCTION(do_max_complex10, _Complex __float128,
              inoutvec[i] = (invec[i] <= inoutvec[i] ? inoutvec[i] : invec[i]))
#endif
#undef GEN_REDUCTION

static MPI_Datatype
get_MPI_datatype(gfc_descriptor_t *desc, int char_len)
{
  int ierr;
  /* FIXME: Better check whether the sizes are okay and supported;
   * MPI3 adds more types, e.g. MPI_INTEGER1. */
  switch (GFC_DTYPE_TYPE_SIZE(desc))
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

      /* Note that we cannot use REAL_16 as we do not know whether it matches
       * REAL(10) or REAL(16), which have both the same bitsize and only make
       * use of less bits. */
    case GFC_DTYPE_COMPLEX_4:
      return MPI_COMPLEX;
    case GFC_DTYPE_COMPLEX_8:
      return MPI_DOUBLE_COMPLEX;
  }
  /* gfortran passes character string arguments with a
   * GFC_DTYPE_TYPE_SIZE == GFC_TYPE_CHARACTER + 64*strlen */
  if ((GFC_DTYPE_TYPE_SIZE(desc) - GFC_DTYPE_CHARACTER) % 64 == 0)
  {
    MPI_Datatype string;

    if (char_len == 0)
      char_len = GFC_DESCRIPTOR_SIZE(desc);
    ierr = MPI_Type_contiguous(char_len, MPI_CHARACTER, &string);
    chk_err(ierr);
    ierr = MPI_Type_commit(&string);
    chk_err(ierr);
    return string;
  }

  return MPI_BYTE;
  /* caf_runtime_error("Unsupported data type in collective: %zd\n", */
  /*                   GFC_DTYPE_TYPE_SIZE(desc)); */
  /* return 0; */
}

static void
internal_co_reduce(MPI_Op op, gfc_descriptor_t *source, int result_image,
                   int *stat, char *errmsg, int src_len, size_t errmsg_len)
{
  size_t i, size;
  int j, ierr, rank = GFC_DESCRIPTOR_RANK(source);
  ptrdiff_t dimextent;

  MPI_Datatype datatype = get_MPI_datatype(source, src_len);

  size = 1;
  for (j = 0; j < rank; ++j)
  {
    dimextent = source->dim[j]._ubound - source->dim[j].lower_bound + 1;
    if (dimextent < 0)
      dimextent = 0;
    size *= dimextent;
  }

  if (rank == 0 || PREFIX(is_contiguous)(source))
  {
    if (result_image == 0)
    {
      ierr = MPI_Allreduce(MPI_IN_PLACE, source->base_addr, size, datatype, op,
                           CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else if (result_image == caf_this_image)
    {
      ierr = MPI_Reduce(MPI_IN_PLACE, source->base_addr, size, datatype, op,
                        result_image - 1, CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else
    {
      ierr = MPI_Reduce(source->base_addr, NULL, size, datatype, op,
                        result_image - 1, CAF_COMM_WORLD);
      chk_err(ierr);
    }
    if (ierr)
      goto error;
    goto co_reduce_cleanup;
  }

  for (i = 0; i < size; ++i)
  {
    ptrdiff_t array_offset_sr = 0, tot_ext = 1, extent = 1;
    for (j = 0; j < rank - 1; ++j)
    {
      extent = source->dim[j]._ubound - source->dim[j].lower_bound + 1;
      array_offset_sr += ((i / tot_ext) % extent) * source->dim[j]._stride;
      tot_ext *= extent;
    }
    array_offset_sr += (i / tot_ext) * source->dim[rank - 1]._stride;
    void *sr = (void *)((char *)source->base_addr
                        + array_offset_sr * GFC_DESCRIPTOR_SIZE(source));
    if (result_image == 0)
    {
      ierr = MPI_Allreduce(MPI_IN_PLACE, sr, 1, datatype, op, CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else if (result_image == caf_this_image)
    {
      ierr = MPI_Reduce(MPI_IN_PLACE, sr, 1, datatype, op, result_image - 1,
                        CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else
    {
      ierr = MPI_Reduce(sr, NULL, 1, datatype, op, result_image - 1,
                        CAF_COMM_WORLD);
      chk_err(ierr);
    }
    if (ierr)
      goto error;
  }

co_reduce_cleanup:
  if (GFC_DESCRIPTOR_TYPE(source) == BT_CHARACTER)
  {
    ierr = MPI_Type_free(&datatype);
    chk_err(ierr);
  }
  if (stat)
    *stat = 0;
  return;
error:
  /* FIXME: Put this in an extra function and use it elsewhere. */
  if (stat)
  {
    *stat = ierr;
    if (!errmsg)
      return;
  }

  int len = sizeof(err_buffer);
  MPI_Error_string(ierr, err_buffer, &len);
  if (!stat)
  {
    err_buffer[len == sizeof(err_buffer) ? len - 1 : len] = '\0';
    caf_runtime_error("CO_SUM failed with %s\n", err_buffer);
  }
  memcpy(errmsg, err_buffer, (errmsg_len > len) ? len : errmsg_len);
  if (errmsg_len > len)
    memset(&errmsg[len], '\0', errmsg_len - len);
}

void
PREFIX(co_broadcast)(gfc_descriptor_t *a, int source_image, int *stat,
                     char *errmsg, charlen_t errmsg_len)
{
  size_t i, size;
  int j, ierr, rank = GFC_DESCRIPTOR_RANK(a);
  ptrdiff_t dimextent;

  MPI_Datatype datatype = get_MPI_datatype(a, 0);

  size = 1;
  for (j = 0; j < rank; ++j)
  {
    dimextent = a->dim[j]._ubound - a->dim[j].lower_bound + 1;
    if (dimextent < 0)
      dimextent = 0;
    size *= dimextent;
  }

  dprint("Using mpi-datatype: 0x%x in co_broadcast (base_addr=%p, rank= %d).\n",
         datatype, a->base_addr, rank);
  if (rank == 0)
  {
    if (datatype == MPI_BYTE)
    {
      ierr = MPI_Bcast(a->base_addr, size * GFC_DESCRIPTOR_SIZE(a), datatype,
                       source_image - 1, CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else if (datatype != MPI_CHARACTER)
    {
      ierr = MPI_Bcast(a->base_addr, size, datatype, source_image - 1,
                       CAF_COMM_WORLD);
      chk_err(ierr);
    }
    else
    {
      int a_length;
      if (caf_this_image == source_image)
        a_length = strlen(a->base_addr);
      /* Broadcast the string lenth */
      ierr = MPI_Bcast(&a_length, 1, MPI_INT, source_image - 1, CAF_COMM_WORLD);
      chk_err(ierr);
      if (ierr)
        goto error;
      /* Broadcast the string itself */
      ierr = MPI_Bcast(a->base_addr, a_length, datatype, source_image - 1,
                       CAF_COMM_WORLD);
      chk_err(ierr);
    }

    if (ierr)
      goto error;
    goto co_broadcast_exit;
  }
  else if (datatype == MPI_CHARACTER) /* rank !=0 */
  {
    caf_runtime_error("Co_broadcast of character arrays are "
                      "not yet supported\n");
  }

  for (i = 0; i < size; ++i)
  {
    ptrdiff_t array_offset = 0, tot_ext = 1, extent = 1;
    for (j = 0; j < rank - 1; ++j)
    {
      extent = a->dim[j]._ubound - a->dim[j].lower_bound + 1;
      array_offset += ((i / tot_ext) % extent) * a->dim[j]._stride;
      tot_ext *= extent;
    }
    array_offset += (i / tot_ext) * a->dim[rank - 1]._stride;
    dprint("The array offset for element %zd used in co_broadcast is %td\n", i,
           array_offset);
    void *sr = (void *)((char *)a->base_addr
                        + array_offset * GFC_DESCRIPTOR_SIZE(a));

    ierr = MPI_Bcast(sr, 1, datatype, source_image - 1, CAF_COMM_WORLD);
    chk_err(ierr);

    if (ierr)
      goto error;
  }

co_broadcast_exit:
  if (stat)
    *stat = 0;
  if (GFC_DESCRIPTOR_TYPE(a) == BT_CHARACTER)
  {
    ierr = MPI_Type_free(&datatype);
    chk_err(ierr);
  }
  return;

error:
  /* FIXME: Put this in an extra function and use it elsewhere. */
  if (stat)
  {
    *stat = ierr;
    if (!errmsg)
      return;
  }

  int len = sizeof(err_buffer);
  MPI_Error_string(ierr, err_buffer, &len);
  if (!stat)
  {
    err_buffer[len == sizeof(err_buffer) ? len - 1 : len] = '\0';
    caf_runtime_error("CO_SUM failed with %s\n", err_buffer);
  }
  memcpy(errmsg, err_buffer, (errmsg_len > len) ? len : errmsg_len);
  if (errmsg_len > len)
    memset(&errmsg[len], '\0', errmsg_len - len);
}

/* The front-end function for co_reduce functionality.  It sets up the MPI_Op
 * for use in MPI_*Reduce functions. */
void
PREFIX(co_reduce)(gfc_descriptor_t *a, void *(*opr)(void *, void *),
                  int opr_flags, int result_image, int *stat, char *errmsg,
                  int a_len, charlen_t errmsg_len)
{
  MPI_Op op;
  int type_a = GFC_DESCRIPTOR_TYPE(a), ierr;
  /* Integers and logicals can be treated the same. */
  if (type_a == BT_INTEGER || type_a == BT_LOGICAL)
  {
    /* When the ARG_VALUE opr_flag is set, then the user-function expects its
     * arguments to be passed by value. */
    if ((opr_flags & GFC_CAF_ARG_VALUE) > 0)
    {
#define ifTypeGen(type)                                                        \
  if (GFC_DESCRIPTOR_SIZE(a) == sizeof(type##_t))                              \
  {                                                                            \
    type##_t_by_value = (typeof(VALUE_FUNC(type##_t)))opr;                     \
    int ierr = MPI_Op_create(redux_##type##_by_value_adapter, 1, &op);         \
    chk_err(ierr);                                                             \
  }
      ifTypeGen(int8) else ifTypeGen(int16) else ifTypeGen(
          int32) else ifTypeGen(int64) else
      {
        caf_runtime_error("CO_REDUCE unsupported integer datatype");
      }
#undef ifTypeGen
    }
    else
    {
      int32_t_by_reference = (typeof(REFERENCE_FUNC(int32_t)))opr;
      ierr = MPI_Op_create(redux_int32_by_reference_adapter, 1, &op);
      chk_err(ierr);
    }
  }
  /* Treat reals/doubles. */
  else if (type_a == BT_REAL)
  {
    /* When the ARG_VALUE opr_flag is set, then the user-function expects its
     * arguments to be passed by value. */
    if (GFC_DESCRIPTOR_SIZE(a) == sizeof(float))
    {
      if ((opr_flags & GFC_CAF_ARG_VALUE) > 0)
      {
        float_by_value = (typeof(VALUE_FUNC(float)))opr;
        ierr = MPI_Op_create(redux_real32_by_value_adapter, 1, &op);
        chk_err(ierr);
      }
      else
      {
        float_by_reference = (typeof(REFERENCE_FUNC(float)))opr;
        ierr = MPI_Op_create(redux_real32_by_reference_adapter, 1, &op);
        chk_err(ierr);
      }
    }
    else
    {
      /* When the ARG_VALUE opr_flag is set, then the user-function expects
       * its arguments to be passed by value. */
      if ((opr_flags & GFC_CAF_ARG_VALUE) > 0)
      {
        double_by_value = (typeof(VALUE_FUNC(double)))opr;
        ierr = MPI_Op_create(redux_real64_by_value_adapter, 1, &op);
        chk_err(ierr);
      }
      else
      {
        double_by_reference = (typeof(REFERENCE_FUNC(double)))opr;
        ierr = MPI_Op_create(redux_real64_by_reference_adapter, 1, &op);
        chk_err(ierr);
      }
    }
  }
  else if (type_a == BT_CHARACTER)
  {
    /* Char array functions always pass by reference. */
    char_by_reference = (typeof(REFERENCE_FUNC(char)))opr;
    ierr = MPI_Op_create(redux_char_by_reference_adapter, 1, &op);
    chk_err(ierr);
  }
  else
  {
    caf_runtime_error("Data type not yet supported for co_reduce\n");
  }

  internal_co_reduce(op, a, result_image, stat, errmsg, a_len, errmsg_len);
}

void
PREFIX(co_sum)(gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
               charlen_t errmsg_len)
{
  internal_co_reduce(MPI_SUM, a, result_image, stat, errmsg, 0, errmsg_len);
}

void
PREFIX(co_min)(gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
               int src_len, charlen_t errmsg_len)
{
  internal_co_reduce(MPI_MIN, a, result_image, stat, errmsg, src_len,
                     errmsg_len);
}

void
PREFIX(co_max)(gfc_descriptor_t *a, int result_image, int *stat, char *errmsg,
               int src_len, charlen_t errmsg_len)
{
  internal_co_reduce(MPI_MAX, a, result_image, stat, errmsg, src_len,
                     errmsg_len);
}

/* Locking functions */

void
PREFIX(lock)(caf_token_t token, size_t index, int image_index,
             int *acquired_lock, int *stat, char *errmsg, charlen_t errmsg_len)
{
  MPI_Win *p = TOKEN(token);
  mutex_lock(*p, (image_index == 0) ? caf_this_image : image_index, index, stat,
             acquired_lock, errmsg, errmsg_len);
}

void
PREFIX(unlock)(caf_token_t token, size_t index, int image_index, int *stat,
               char *errmsg, charlen_t errmsg_len)
{
  MPI_Win *p = TOKEN(token);
  mutex_unlock(*p, (image_index == 0) ? caf_this_image : image_index, index,
               stat, errmsg, errmsg_len);
}

/* Atomics operations */

void
PREFIX(atomic_define)(caf_token_t token, size_t offset, int image_index,
                      void *value, int *stat, int type __attribute__((unused)),
                      int kind)
{
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0, image = (image_index != 0) ? image_index - 1 : mpi_this_image;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Accumulate(value, 1, dt, image, offset, 1, dt, MPI_REPLACE, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#else  // MPI_VERSION
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Put(value, 1, dt, image, offset, 1, dt, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#endif // MPI_VERSION

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    terminate_internal(ierr, 0);

  return;
}

void
PREFIX(atomic_ref)(caf_token_t token, size_t offset, int image_index,
                   void *value, int *stat, int type __attribute__((unused)),
                   int kind)
{
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0, image = (image_index != 0) ? image_index - 1 : mpi_this_image;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Fetch_and_op(NULL, value, dt, image, offset, MPI_NO_OP, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#else  // MPI_VERSION
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Get(value, 1, dt, image, offset, 1, dt, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#endif // MPI_VERSION

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    terminate_internal(ierr, 0);

  return;
}

void
PREFIX(atomic_cas)(caf_token_t token, size_t offset, int image_index, void *old,
                   void *compare, void *new_val, int *stat,
                   int type __attribute__((unused)), int kind)
{
  MPI_Win *p = TOKEN(token);
  MPI_Datatype dt;
  int ierr = 0, image = (image_index != 0) ? image_index - 1 : mpi_this_image;

  selectType(kind, &dt);

#if MPI_VERSION >= 3
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Compare_and_swap(new_val, compare, old, dt, image, offset, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#else // MPI_VERSION
#warning atomic_cas for MPI-2 is not yet implemented
  printf("We apologize but atomic_cas for MPI-2 is not yet implemented\n");
  ierr = 1;
#endif // MPI_VERSION

  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    terminate_internal(ierr, 0);

  return;
}

void
PREFIX(atomic_op)(int op, caf_token_t token, size_t offset, int image_index,
                  void *value, void *old, int *stat,
                  int type __attribute__((unused)), int kind)
{
  int ierr = 0;
  MPI_Datatype dt;
  MPI_Win *p = TOKEN(token);
  int image = (image_index != 0) ? image_index - 1 : mpi_this_image;

#if MPI_VERSION >= 3
  old = malloc(kind);
  selectType(kind, &dt);

  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  /* Atomic_add */
  switch (op)
  {
    case 1:
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_SUM, *p);
      chk_err(ierr);
      break;
    case 2:
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BAND, *p);
      chk_err(ierr);
      break;
    case 4:
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BOR, *p);
      chk_err(ierr);
      break;
    case 5:
      ierr = MPI_Fetch_and_op(value, old, dt, image, offset, MPI_BXOR, *p);
      chk_err(ierr);
      break;
    default:
      printf("We apologize but the atomic operation requested for MPI < 3 "
             "is not yet implemented\n");
      break;
  }
  CAF_Win_unlock(image, *p);

  free(old);
#else // MPI_VERSION
#warning atomic_op for MPI is not yet implemented
  printf("We apologize but atomic_op for MPI < 3 is not yet implemented\n");
#endif // MPI_VERSION
  if (stat)
    *stat = ierr;
  else if (ierr != 0)
    terminate_internal(ierr, 0);

  return;
}

/* Events */

void
PREFIX(event_post)(caf_token_t token, size_t index, int image_index, int *stat,
                   char *errmsg, charlen_t errmsg_len)
{
  int value = 1, ierr = 0;
  MPI_Win *p = TOKEN(token);
  const char msg[] = "Error on event post";
  int image = (image_index == 0) ? mpi_this_image : image_index - 1;

  if (stat != NULL)
    *stat = 0;

#if MPI_VERSION >= 3
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Accumulate(&value, 1, MPI_INT, image, index * sizeof(int), 1,
                        MPI_INT, MPI_SUM, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#else // MPI_VERSION
#warning Events for MPI-2 are not implemented
  printf("Events for MPI-2 are not supported, "
         "please update your MPI implementation\n");
#endif // MPI_VERSION

  check_image_health(image_index, stat);

  if (!stat && ierr == STAT_FAILED_IMAGE)
    terminate_internal(ierr, 0);

  if (ierr != MPI_SUCCESS)
  {
    if (stat != NULL)
      *stat = ierr;
    if (errmsg != NULL)
    {
      memset(errmsg, ' ', errmsg_len);
      memcpy(errmsg, msg, MIN(errmsg_len, strlen(msg)));
    }
  }
}

void
PREFIX(event_wait)(caf_token_t token, size_t index, int until_count, int *stat,
                   char *errmsg, charlen_t errmsg_len)
{
  int ierr = 0, count = 0, i, image = mpi_this_image;
  int *var = NULL, flag, old = 0, newval = 0;
  const int spin_loop_max = 20000;
  MPI_Win *p = TOKEN(token);
  const char msg[] = "Error on event wait";

  if (stat != NULL)
    *stat = 0;

  ierr = MPI_Win_get_attr(*p, MPI_WIN_BASE, &var, &flag);
  chk_err(ierr);

  MPI_Win_lock_all(MPI_MODE_NOCHECK, *p);
  for (i = 0; i < spin_loop_max; ++i)
  {
    ierr = MPI_Win_sync(*p);
    chk_err(ierr);
    count = var[index];
    if (count >= until_count)
      break;
  }

  i = 1;
  while (count < until_count)
  {
    ierr = MPI_Win_sync(*p);
    chk_err(ierr);
    count = var[index];
    usleep(10 * i);
    ++i;
    /* Needed to enforce MPI progress */
    ierr = MPI_Win_flush(image, *p);
    chk_err(ierr);
  }

  newval = -until_count;

  MPI_Win_unlock_all(*p);
  CAF_Win_lock(MPI_LOCK_SHARED, image, *p);
  ierr = MPI_Fetch_and_op(&newval, &old, MPI_INT, image, index * sizeof(int),
                          MPI_SUM, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);

  check_image_health(image, stat);

  if (!stat && ierr == STAT_FAILED_IMAGE)
    terminate_internal(ierr, 0);

  if (ierr != MPI_SUCCESS)
  {
    if (stat != NULL)
      *stat = ierr;
    if (errmsg != NULL)
    {
      memset(errmsg, ' ', errmsg_len);
      memcpy(errmsg, msg, MIN(errmsg_len, strlen(msg)));
    }
  }
}

void
PREFIX(event_query)(caf_token_t token, size_t index, int image_index,
                    int *count, int *stat)
{
  MPI_Win *p = TOKEN(token);
  int ierr = 0, image = (image_index == 0) ? mpi_this_image : image_index - 1;

  if (stat != NULL)
    *stat = 0;

#if MPI_VERSION >= 3
  CAF_Win_lock(MPI_LOCK_EXCLUSIVE, image, *p);
  ierr = MPI_Fetch_and_op(NULL, count, MPI_INT, image, index * sizeof(int),
                          MPI_NO_OP, *p);
  chk_err(ierr);
  CAF_Win_unlock(image, *p);
#else // MPI_VERSION
#warning Events for MPI-2 are not implemented
  printf("Events for MPI-2 are not supported, "
         "please update your MPI implementation\n");
#endif // MPI_VERSION
  if (ierr != MPI_SUCCESS && stat != NULL)
    *stat = ierr;
}

/* Internal function to execute the part that is common to all (error) stop
 * functions. */

static void
terminate_internal(int stat_code, int exit_code)
{
  dprint("terminate_internal (stat_code = %d, exit_code = %d).\n", stat_code,
         exit_code);
  finalize_internal(stat_code);

#ifndef WITH_FAILED_IMAGES
  MPI_Abort(MPI_COMM_WORLD, exit_code);
#endif
  exit(exit_code);
}

#ifdef GCC_GE_8
#undef QUIETARG
#define QUIETARG , bool quiet
#endif

/* STOP function for integer arguments. */

void
PREFIX(stop_numeric)(int stop_code QUIETARG)
{
#ifndef GCC_GE_8
  bool quiet = false;
#endif
  if (!quiet)
    fprintf(stderr, "STOP %d\n", stop_code);

  /* Stopping includes taking down the runtime regularly and returning the
   * stop_code. */
  terminate_internal(STAT_STOPPED_IMAGE, stop_code);
}

/* STOP function for string arguments. */

void
PREFIX(stop_str)(const char *string, charlen_t len QUIETARG)
{
#ifndef GCC_GE_8
  bool quiet = false;
#endif
  if (!quiet)
  {
    fputs("STOP ", stderr);
    while (len--)
      fputc(*(string++), stderr);
    fputs("\n", stderr);
  }
  /* Stopping includes taking down the runtime regularly. */
  terminate_internal(STAT_STOPPED_IMAGE, 0);
}

/* ERROR STOP function for string arguments. */

static void
error_stop_str(const char *string, size_t len, bool quiet)
{
  if (!quiet)
  {
    fputs("ERROR STOP ", stderr);
    while (len--)
      fputc(*(string++), stderr);
    fputs("\n", stderr);
  }
  terminate_internal(STAT_STOPPED_IMAGE, 1);
}

void
PREFIX(error_stop_str)(const char *string, charlen_t len QUIETARG)
{
#ifndef GCC_GE_8
  bool quiet = false;
#endif
  error_stop_str(string, len, quiet);
}

/* ERROR STOP function for numerical arguments. */

void
PREFIX(error_stop)(int error QUIETARG)
{
#ifndef GCC_GE_8
  bool quiet = false;
#endif
  if (!quiet)
    fprintf(stderr, "ERROR STOP %d\n", error);

  terminate_internal(STAT_STOPPED_IMAGE, error);
}

/* FAIL IMAGE statement. */

void
PREFIX(fail_image)(void)
{
  fputs("IMAGE FAILED!\n", stderr);

  raise(SIGKILL);
  /* A failing image is expected to take down the runtime regularly. */
  terminate_internal(STAT_FAILED_IMAGE, 0);
}

int
PREFIX(image_status)(int image)
{
#ifdef GFC_CAF_CHECK
  if (image < 1 || image > caf_num_images)
  {
    char errmsg[60];
    sprintf(errmsg, "Image #%d out of bounds of images 1..%d.", image,
            caf_num_images);
    caf_runtime_error(errmsg);
  }
#endif
#ifdef WITH_FAILED_IMAGES
  if (image_stati[image - 1] == 0)
  {
    int status, ierr;
    /* Check that we are fine before doing anything.
     *
     * Do an MPI-operation to learn about failed/stopped images, that have
     * not been detected yet. */
    ierr = MPI_Test(&alive_request, &status, MPI_STATUSES_IGNORE);
    chk_err(ierr);
    MPI_Error_class(ierr, &status);
    if (ierr == MPI_SUCCESS)
    {
      CAF_Win_lock(MPI_LOCK_SHARED, image - 1, *stat_tok);
      ierr = MPI_Get(&status, 1, MPI_INT, image - 1, 0, 1, MPI_INT, *stat_tok);
      chk_err(ierr);
      dprint("Image status of image #%d is: %d\n", image, status);
      CAF_Win_unlock(image - 1, *stat_tok);
      image_stati[image - 1] = status;
    }
    else if (status == MPIX_ERR_PROC_FAILED)
    {
      image_stati[image - 1] = STAT_FAILED_IMAGE;
    }
    else
    {
      const int strcap = 200;
      char errmsg[strcap];
      int slen, supplied_len;
      sprintf(errmsg, "Image status for image #%d returned mpi error: ", image);
      slen = strlen(errmsg);
      supplied_len = strcap - slen;
      MPI_Error_string(status, &errmsg[slen], &supplied_len);
      caf_runtime_error(errmsg);
    }
  }
  return image_stati[image - 1];
#else
  unsupported_fail_images_message("IMAGE_STATUS()");
#endif // WITH_FAILED_IMAGES

  return 0;
}

void
PREFIX(failed_images)(gfc_descriptor_t *array, int team __attribute__((unused)),
                      int *kind)
{
  int local_kind = kind ? *kind : 4; /* GFC_DEFAULT_INTEGER_KIND = 4*/

#ifdef WITH_FAILED_IMAGES
  void *mem = calloc(num_images_failed, local_kind);
  array->base_addr = mem;
  for (int i = 0; i < caf_num_images; ++i)
  {
    if (image_stati[i] == STAT_FAILED_IMAGE)
    {
      switch (local_kind)
      {
        case 1:
          *(int8_t *)mem = i + 1;
          break;
        case 2:
          *(int16_t *)mem = i + 1;
          break;
        case 4:
          *(int32_t *)mem = i + 1;
          break;
        case 8:
          *(int64_t *)mem = i + 1;
          break;
#ifdef HAVE_GFC_INTEGER_16
        case 16:
          *(int128t *)mem = i + 1;
          break;
#endif
        default:
          caf_runtime_error("Unsupported integer kind %1 "
                            "in caf_failed_images.",
                            local_kind);
      }
      mem += local_kind;
    }
  }
  array->dim[0]._ubound = num_images_failed - 1;
#else
  unsupported_fail_images_message("FAILED_IMAGES()");
  array->dim[0]._ubound = -1;
  array->base_addr = NULL;
#endif // WITH_FAILED_IMAGES

#ifdef GCC_GE_8
  array->dtype.type = BT_INTEGER;
  array->dtype.elem_len = local_kind;
#else
  array->dtype = ((BT_INTEGER << GFC_DTYPE_TYPE_SHIFT)
                  | (local_kind << GFC_DTYPE_SIZE_SHIFT));
#endif
  array->dim[0].lower_bound = 0;
  array->dim[0]._stride = 1;
  array->offset = 0;
}

void
PREFIX(stopped_images)(gfc_descriptor_t *array,
                       int team __attribute__((unused)), int *kind)
{
  int local_kind = kind ? *kind : 4; /* GFC_DEFAULT_INTEGER_KIND = 4*/

#ifdef WITH_FAILED_IMAGES
  void *mem = calloc(num_images_stopped, local_kind);
  array->base_addr = mem;
  for (int i = 0; i < caf_num_images; ++i)
  {
    if (image_stati[i])
    {
      switch (local_kind)
      {
        case 1:
          *(int8_t *)mem = i + 1;
          break;
        case 2:
          *(int16_t *)mem = i + 1;
          break;
        case 4:
          *(int32_t *)mem = i + 1;
          break;
        case 8:
          *(int64_t *)mem = i + 1;
          break;
#ifdef HAVE_GFC_INTEGER_16
        case 16:
          *(int128t *)mem = i + 1;
          break;
#endif
        default:
          caf_runtime_error("Unsupported integer kind %1 "
                            "in caf_stopped_images.",
                            local_kind);
      }
      mem += local_kind;
    }
  }
  array->dim[0]._ubound = num_images_stopped - 1;
#else
  unsupported_fail_images_message("STOPPED_IMAGES()");
  array->dim[0]._ubound = -1;
  array->base_addr = NULL;
#endif // WITH_FAILED_IMAGES

#ifdef GCC_GE_8
  array->dtype.type = BT_INTEGER;
  array->dtype.elem_len = local_kind;
#else
  array->dtype = ((BT_INTEGER << GFC_DTYPE_TYPE_SHIFT)
                  | (local_kind << GFC_DTYPE_SIZE_SHIFT));
#endif
  array->dim[0].lower_bound = 0;
  array->dim[0]._stride = 1;
  array->offset = 0;
}

/* Give a descriptive message when failed images support is not available. */
void
unsupported_fail_images_message(const char *functionname)
{
  fprintf(stderr,
          "*** caf_mpi-lib runtime message on image %d:\n"
          "*** The failed images feature '%s' "
          "*** of Fortran 2015 standard is not available in this build."
          "*** You need a compiler with failed images support activated and "
          "*** compile OpenCoarrays with failed images support.\n",
          caf_this_image, functionname);
#ifdef STOP_ON_UNSUPPORTED
  exit(EXIT_FAILURE);
#endif
}

#ifdef GCC_GE_16
void
PREFIX(form_team)(int team_id, caf_team_t *team, int *new_index, int *stat,
                  char *errmsg, charlen_t errmsg_len)
{
  static const char *negative_team_id
      = "FORM TEAM: Team id shall be a positive unique integer";
  static const char *negative_new_index
      = "FORM TEAM: NEW_INDEX= shall be a positive unique integer";
  caf_teams_list_t *new_team;
  MPI_Comm current_comm = CAF_COMM_WORLD;
  int ierr;
  int key = new_index ? *new_index : caf_this_image;

  if (stat)
    *stat = 0;

  if (team_id < 0)
  {
    caf_internal_error(negative_team_id, stat, errmsg, errmsg_len);
    return;
  }
  if (new_index && *new_index < 0)
  {
    caf_internal_error(negative_new_index, stat, errmsg, errmsg_len);
    return;
  }

  new_team = (caf_teams_list_t *)malloc(sizeof(struct caf_teams_list));
  new_team->team_id = team_id;
  new_team->image_id = key;
  new_team->prev = teams_list;

  ierr
      = MPI_Comm_split(current_comm, team_id, key - 1, &new_team->communicator);
  chk_err(ierr);

  teams_list = new_team;
  *team = new_team;
}
#else
void
PREFIX(form_team)(int team_id, caf_team_t *team,
                  int index __attribute__((unused)))
{
  struct caf_teams_list *tmp;
  MPI_Comm *newcomm;
  MPI_Comm current_comm = CAF_COMM_WORLD;
  int ierr;

  newcomm = (MPI_Comm *)calloc(1, sizeof(MPI_Comm));
  ierr = MPI_Comm_split(current_comm, team_id, mpi_this_image, newcomm);
  chk_err(ierr);

  tmp = calloc(1, sizeof(struct caf_teams_list));
  tmp->prev = teams_list;
  teams_list = tmp;
  teams_list->team_id = team_id;
  teams_list->team = newcomm;
  *team = tmp;
}
#endif

#ifdef GCC_GE_16
void
PREFIX(change_team)(caf_team_t team, int *stat, char *errmsg,
                    charlen_t errmsg_len)
{
  caf_team_stack_node_t *new_current_team = NULL;
  caf_teams_list_t *provisioned_team = (caf_teams_list_t *)team;
  int ierr;

  if (stat)
    *stat = 0;

  if (provisioned_team == NULL)
  {
    caf_internal_error("CHANGE TEAM: Called on a non-existing team", stat,
                       errmsg, errmsg_len);
    return;
  }

  new_current_team
      = (caf_team_stack_node_t *)malloc(sizeof(caf_team_stack_node_t));
  *new_current_team
      = (caf_team_stack_node_t){provisioned_team, NULL, current_team};

  current_team = new_current_team;
  CAF_COMM_WORLD_store = &current_team->team_list_elem->communicator;
  ierr = MPI_Comm_rank(CAF_COMM_WORLD, &mpi_this_image);
  chk_err(ierr);
  caf_this_image = mpi_this_image + 1;
  ierr = MPI_Comm_size(CAF_COMM_WORLD, &caf_num_images);
  chk_err(ierr);
  ierr = MPI_Barrier(CAF_COMM_WORLD);
  chk_err(ierr);
}
#else
void
PREFIX(change_team)(caf_team_t *team, int coselector __attribute__((unused)))
{
  caf_used_teams_list *tmp_used = NULL;
  caf_teams_list *tmp_list = NULL;
  void *tmp_team;
  MPI_Comm *tmp_comm;

  tmp_list = (struct caf_teams_list *)*team;
  tmp_team = (void *)tmp_list->team;
  tmp_comm = (MPI_Comm *)tmp_team;

  tmp_used = (caf_used_teams_list *)calloc(1, sizeof(caf_used_teams_list));
  tmp_used->prev = used_teams;

  /* We need to look in the teams_list and find the appropriate element.
   * This is not efficient but can be easily fixed in the future.
   * Instead of keeping track of the communicator in the compiler
   * we should keep track of the caf_teams_list element associated with it. */

  /*
  tmp_list = teams_list;

 while (tmp_list)
 {
   if (tmp_list->team == tmp_team)
     break;
   tmp_list = tmp_list->prev;
 }
 */

  if (tmp_list == NULL)
    caf_runtime_error("CHANGE TEAM called on a non-existing team");

  tmp_used->team_list_elem = tmp_list;
  used_teams = tmp_used;
  tmp_team = tmp_used->team_list_elem->team;
  tmp_comm = (MPI_Comm *)tmp_team;
  CAF_COMM_WORLD = *tmp_comm;
  int ierr = MPI_Comm_rank(*tmp_comm, &mpi_this_image);
  chk_err(ierr);
  caf_this_image = mpi_this_image + 1;
  ierr = MPI_Comm_size(*tmp_comm, &caf_num_images);
  chk_err(ierr);
  ierr = MPI_Barrier(*tmp_comm);
  chk_err(ierr);
}
#endif

#ifdef GCC_GE_16
MPI_Fint
PREFIX(get_communicator)(caf_team_t *team)
{
  caf_teams_list_t *cur_team = team ? *team : current_team->team_list_elem;

  MPI_Fint ret = MPI_Comm_c2f(cur_team->communicator);

  return ret;
}

int
PREFIX(team_number)(caf_team_t team)
{
  if (team != NULL)
    return ((caf_teams_list_t *)team)->team_id;
  else
    return current_team->team_list_elem->team_id; /* current team */
}

caf_team_t
PREFIX(get_team)(int32_t *level)
{
  if (!level)
    return current_team->team_list_elem;

  switch ((caf_team_level_t)*level)
  {
    case CAF_INITIAL_TEAM:
      return initial_team->team_list_elem;
    case CAF_PARENT_TEAM:
      return current_team->parent ? current_team->parent->team_list_elem
                                  : initial_team->team_list_elem;
    case CAF_CURRENT_TEAM:
      return current_team->team_list_elem;
    default:
      caf_runtime_error("Illegal value for GET_TEAM");
  }
  return NULL; /* To prevent any warnings.  */
}
#else
MPI_Fint
PREFIX(get_communicator)(caf_team_t *team)
{
  if (team != NULL)
    caf_runtime_error("get_communicator does not yet support "
                      "the optional team argument");

  MPI_Comm *comm_ptr = teams_list->team;
  MPI_Fint ret = MPI_Comm_c2f(*comm_ptr);

  return ret;
}

int
PREFIX(team_number)(caf_team_t *team)
{
  if (team != NULL)
    return ((caf_teams_list *)team)->team_id;
  else
    return used_teams->team_list_elem->team_id; /* current team */
}
#endif

#ifdef GCC_GE_16
void
PREFIX(end_team)(int *stat, char *errmsg, charlen_t errmsg_len)
{
  caf_team_stack_node_t *ending_team = current_team;
  int ierr;

  if (stat)
    *stat = 0;

  ierr = MPI_Barrier(CAF_COMM_WORLD);
  chk_err(ierr);
  if (current_team->parent == NULL)
  {
    caf_internal_error("END TEAM called on initial team", stat, errmsg,
                       errmsg_len);
    return;
  }

  for (struct allocated_tokens_t *ac = current_team->allocated_tokens; ac;)
  {
    struct allocated_tokens_t *nac = ac->next;

    PREFIX(deregister)((void **)&ac->token,
                       CAF_DEREGTYPE_COARRAY_DEALLOCATE_ONLY, stat, errmsg,
                       errmsg_len);
    free(ac);
    ac = nac;
  }
  current_team = current_team->parent;

  CAF_COMM_WORLD_store = &current_team->team_list_elem->communicator;
  ierr = MPI_Comm_rank(CAF_COMM_WORLD, &mpi_this_image);
  chk_err(ierr);
  caf_this_image = mpi_this_image + 1;
  ierr = MPI_Comm_size(CAF_COMM_WORLD, &caf_num_images);
  chk_err(ierr);
  free(ending_team);
}
#else
void
PREFIX(end_team)(caf_team_t *team __attribute__((unused)))
{
  caf_used_teams_list *tmp_used = NULL;
  void *tmp_team;
  MPI_Comm *tmp_comm;
  int ierr;

  ierr = MPI_Barrier(CAF_COMM_WORLD);
  chk_err(ierr);
  if (used_teams->prev == NULL)
    caf_runtime_error("END TEAM called on initial team");

  tmp_used = used_teams;
  used_teams = used_teams->prev;
  free(tmp_used);
  tmp_used = used_teams;
  tmp_team = tmp_used->team_list_elem->team;
  tmp_comm = (MPI_Comm *)tmp_team;
  CAF_COMM_WORLD = *tmp_comm;
  /* CAF_COMM_WORLD = (MPI_Comm)*tmp_used->team_list_elem->team; */
  ierr = MPI_Comm_rank(CAF_COMM_WORLD, &mpi_this_image);
  chk_err(ierr);
  caf_this_image = mpi_this_image + 1;
  ierr = MPI_Comm_size(CAF_COMM_WORLD, &caf_num_images);
  chk_err(ierr);
}
#endif

#ifdef GCC_GE_16
void
PREFIX(sync_team)(caf_team_t team, int *stat, char *errmsg,
                  charlen_t errmsg_len)
{
  caf_teams_list_t *team_to_sync = (caf_teams_list_t *)team;
  caf_team_stack_node_t *active_team = current_team;

  if (stat)
    *stat = 0;

  /* Check if team to sync is a child of the current team, aka not changed to
   * yet.
   */
  if (team_to_sync->prev != active_team->team_list_elem)
    /* if the team is not a child */
    for (; active_team && active_team->team_list_elem != team_to_sync;
         active_team = active_team->parent)
      ;

  if (!active_team)
  {
    caf_internal_error("SYNC TEAM: Called on team different from current, "
                       "or ancestor, or child",
                       stat, errmsg, errmsg_len);
    return;
  }

  int ierr = MPI_Barrier(team_to_sync->communicator);
  chk_err(ierr);
}
#else
void
PREFIX(sync_team)(caf_team_t *team, int unused __attribute__((unused)))
{
  caf_teams_list *tmp_list = NULL;
  caf_used_teams_list *tmp_used = NULL;
  void *tmp_team;
  MPI_Comm *tmp_comm;

  tmp_used = used_teams;
  tmp_list = (struct caf_teams_list *)*team;
  tmp_team = (void *)tmp_list->team;
  tmp_comm = (MPI_Comm *)tmp_team;

  /* if the team is not a child */
  if (tmp_used->team_list_elem != tmp_list->prev)
    /* then search backwards through the team list, first checking if it's the
     * current team, then if it is an ancestor team */
    while (tmp_used)
    {
      if (tmp_used->team_list_elem == tmp_list)
        break;
      tmp_used = tmp_used->prev;
    }

  if (tmp_used == NULL)
    caf_runtime_error("SYNC TEAM called on team different from current, "
                      "or ancestor, or child");

  int ierr = MPI_Barrier(*tmp_comm);
  chk_err(ierr);
}
#endif

extern void
_gfortran_random_seed_i4(int32_t *size, gfc_dim1_descriptor_t *put,
                         gfc_dim1_descriptor_t *get);

void
PREFIX(random_init)(bool repeatable, bool image_distinct)
{
  static gfc_dim1_descriptor_t rand_seed;
  static bool rep_needs_init = true, arr_needs_init = true;
  static int32_t seed_size;

  if (arr_needs_init)
  {
    _gfortran_random_seed_i4(&seed_size, NULL, NULL);
    memset(&rand_seed, 0, sizeof(gfc_dim1_descriptor_t));
    rand_seed.base.base_addr
        = malloc(seed_size * sizeof(int32_t)); // because using seed_i4
    rand_seed.base.offset = -1;
    rand_seed.base.dtype.elem_len = sizeof(int32_t);
    rand_seed.base.dtype.rank = 1;
    rand_seed.base.dtype.type = BT_INTEGER;
    rand_seed.base.span = 0;
    rand_seed.dim[0].lower_bound = 1;
    rand_seed.dim[0]._ubound = seed_size;
    rand_seed.dim[0]._stride = 1;

    arr_needs_init = false;
  }

  if (repeatable)
  {
    if (rep_needs_init)
    {
      int32_t lcg_seed = 57911963;
      if (image_distinct)
      {
        lcg_seed *= caf_this_image;
      }
      int32_t *curr = rand_seed.base.base_addr;
      for (int i = 0; i < seed_size; ++i)
      {
        const int32_t a = 16087;
        const int32_t m = INT32_MAX;
        const int32_t q = 127773;
        const int32_t r = 2836;
        lcg_seed = a * (lcg_seed % q) - r * (lcg_seed / q);
        if (lcg_seed <= 0)
          lcg_seed += m;
        *curr = lcg_seed;
        ++curr;
      }
      rep_needs_init = false;
    }
    _gfortran_random_seed_i4(NULL, &rand_seed, NULL);
  }
  else if (image_distinct)
  {
    _gfortran_random_seed_i4(NULL, NULL, NULL);
  }
  else
  {
    if (caf_this_image == 0)
    {
      _gfortran_random_seed_i4(NULL, NULL, &rand_seed);
      MPI_Bcast(rand_seed.base.base_addr, seed_size, MPI_INT32_T, 0,
                CAF_COMM_WORLD);
    }
    else
    {
      MPI_Bcast(rand_seed.base.base_addr, seed_size, MPI_INT32_T, 0,
                CAF_COMM_WORLD);
      _gfortran_random_seed_i4(NULL, &rand_seed, NULL);
    }
  }
}
