/* Caf collective functions
*
* Copyright (c) 2012-2019, Sourcery, Inc.
* Copyright (c) 2012-2019, Sourcery Institute
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

#include <cassert>
#include <iostream>
#include <cstring>

#include "libcaf.h"

void caf_co_broadcast(CFI_cdesc_t *a, int source_image, int *stat, char *errmsg, size_t errmsg_len)
{
   MPI_Datatype datatype = get_MPI_datatype(a);
   size_t size = get_size(a);

   int ierr = MPI_Bcast(a->base_addr, size, datatype, source_image - 1, MPI_COMM_WORLD);
}

void caf_co_max(CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len)
{
   internal_co_reduce(MPI_MAX, a, result_image, stat, errmsg, errmsg_len);
}

void caf_co_min(CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len)
{
   internal_co_reduce(MPI_MIN, a, result_image, stat, errmsg, errmsg_len);
}

void caf_co_sum(CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len)
{
   internal_co_reduce(MPI_SUM, a, result_image, stat, errmsg, errmsg_len);
}

void internal_co_reduce(MPI_Op op, CFI_cdesc_t *source, int result_image,
                        int *stat, char *errmsg, size_t errmsg_len)
{
   MPI_Datatype datatype = get_MPI_datatype(source);
   size_t size = get_size(source);
   bool isContiguous = true;            // assuming is contiguous for now
   CFI_rank_t rank = source->rank;
   int ierr;

   if (rank == 0 || isContiguous) {
      if (result_image == 0) {
         std::cerr << "result_image == 0 not yet implemented\n";
      }
      else if (result_image == caf_this_image()) {
         ierr = MPI_Reduce(MPI_IN_PLACE, source->base_addr, size, datatype, op, result_image - 1, MPI_COMM_WORLD);
      }
      else {
         ierr = MPI_Reduce(source->base_addr, NULL, size, datatype, op, result_image - 1, MPI_COMM_WORLD);
      }

      // deal with errors
      if (ierr) {
         if (error(ierr, stat, errmsg, errmsg_len)) {
            return;
         }
      }

      // co reduce cleanup
      if (datatype == CFI_type_char) {      // what is CFI equivalent of BT_CHARACTER?
         ierr = MPI_Type_free(&datatype);// chk_err(ierr);
      }
      if (stat) {
         *stat = 0;
      }
      //      return;
   }
   else {        // is not contiguous
#if 0
      for (int i = 0; i < size; ++i) {
         ptrdiff_t array_offset_sr = 0, tot_ext = 1, extent = 1;
         for (int j = 0; j < rank - 1; ++j) {
            extent = source->dim[j].extent;
            array_offset_sr += ((i / tot_ext) % extent) * source->dim[j].sm;
            tot_ext *= extent;
         }
         array_offset_sr += (i / tot_ext) * source->dim[rank - 1].sm;
         void *sr = (void *)((char *)source->base_addr
                             + array_offset_sr * source->elem_len);
         if (result_image == 0) {
            ierr = MPI_Allreduce(MPI_IN_PLACE, sr, 1, datatype, op, MPI_COMM_WORLD);
            //            chk_err(ierr);
         }
         else if (result_image == caf_this_image()) {
            ierr = MPI_Reduce(MPI_IN_PLACE, sr, 1, datatype, op, result_image - 1,
                              MPI_COMM_WORLD);// chk_err(ierr);
         }
         else {
            ierr = MPI_Reduce(sr, NULL, 1, datatype, op, result_image - 1,
                              MPI_COMM_WORLD);// chk_err(ierr);
         }

         // deal with errors
         if (ierr) {
            if (error(ierr, stat, errmsg, errmsg_len)) {
               return;
            }
         }
      }
#endif
   }
}

char err_buffer[MPI_MAX_ERROR_STRING];

bool error(int ierr, int *stat, char *errmsg, size_t errmsg_len)
{
   if (stat) {
      *stat = ierr;
      if (!errmsg) {
         return true;
      }
   }

   int len = sizeof(err_buffer);
   MPI_Error_string(ierr, err_buffer, &len);

   if (!stat) {
      err_buffer[len == sizeof(err_buffer) ? len - 1 : len] = '\0';
      caf_runtime_error("CO_SUM failed with %s\n", err_buffer);
   }

   memcpy(errmsg, err_buffer, (errmsg_len > len) ? len : errmsg_len);

   if (errmsg_len > len) {
      memset(&errmsg[len], '\0', errmsg_len - len);
   }

  return false;

}

/* Keep in sync with single.c. */

static void
caf_runtime_error (const char *message, ...)
{
   va_list ap;
   fprintf(stderr, "Fortran runtime error on image %d: ", caf_this_image);
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

static MPI_Datatype get_MPI_datatype(CFI_cdesc_t *desc)
{
   switch (desc->type)
      {
      case CFI_type_signed_char:
         return MPI_SIGNED_CHAR;
      case CFI_type_short:
         return MPI_SHORT;
      case CFI_type_int:
         return MPI_INT;
      case CFI_type_long:
         return MPI_LONG;
      case CFI_type_long_long:
         return MPI_LONG_LONG_INT;
      case CFI_type_int8_t:
         return MPI_INT8_T;
      case CFI_type_int16_t:
         return MPI_INT16_T;
      case CFI_type_int32_t:
         return MPI_INT32_T;
      case CFI_type_int64_t:
         return MPI_INT64_T;
      case CFI_type_double:
         return MPI_DOUBLE;
      case CFI_type_long_double:
         return MPI_LONG_DOUBLE;
      case CFI_type_float_Complex:
         return MPI_C_FLOAT_COMPLEX;
      case CFI_type_double_Complex:
         return MPI_C_DOUBLE_COMPLEX;
      case CFI_type_long_double_Complex:
         return MPI_C_LONG_DOUBLE_COMPLEX;
      case CFI_type_Bool:
         return MPI_C_BOOL;
      case CFI_type_char:
         return MPI_CHAR;
#if 0
      case CFI_type_size_t:
      case CFI_type_int128_t:
      case CFI_type_int_least8_t:
      case CFI_type_int_least16_t:
      case CFI_type_int_least32_t:
      case CFI_type_int_least64_t:
      case CFI_type_int_least128_t:
      case CFI_type_int_fast8_t:
      case CFI_type_int_fast16_t:
      case CFI_type_int_fast32_t:
      case CFI_type_int_fast64_t:
      case CFI_type_int_fast128_t:
      case CFI_type_intmax_t:
      case CFI_type_intptr_t:
      case CFI_type_ptrdiff_t:
      case CFI_type_float:
      case CFI_type_cptr:
      case CFI_type_struct:
#endif

      default:
         std::cerr << "default case, no case yet for type: " << desc->type << "\n";

      }

  return MPI_INT;
}

static size_t get_size(CFI_cdesc_t *desc)
{
   size_t size;
   int rank = desc->rank;
   ptrdiff_t dimextent;

   size = 1;
   for (int j = 0; j < rank; ++j) {
      dimextent = desc->dim[j].extent;
      if (dimextent < 0) {
         dimextent = 0;
      }
      size *= dimextent;
   }

   return size;
}
