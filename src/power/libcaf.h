/* Libcaf: Application Binary Interface for parallel Fortran compilers

Copyright (c) 2012-2019, Sourcery, Inc.
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

#ifndef LIBCAF_H
#define LIBCAF_H

#include <mpi.h>

#include "/Users/katherinearasmussen/f18/f18-master/include/flang/ISO_Fortran_binding.h"
using Fortran::ISO::Fortran_2018::CFI_cdesc_t;
using Fortran::ISO::Fortran_2018::CFI_rank_t;
using Fortran::ISO::Fortran_2018::CFI_index_t;


void caf_init(int *, char ***);
void caf_finalize();

void caf_error_stop(int, bool);

int caf_num_images(int = 0);
int caf_this_image();
void caf_sync_all();

// collectives
void caf_co_broadcast(CFI_cdesc_t *a, int source_image, int *stat, char *errmsg, size_t errmsg_len);
void caf_co_max      (CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len);
void caf_co_min      (CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len);
void caf_co_sum      (CFI_cdesc_t *a, int result_image, int *stat, char *errmsg, size_t errmsg_len);

void internal_co_reduce(MPI_Op op, CFI_cdesc_t *source, int result_image, int *stat, char *errmsg, size_t errmsg_len);
static MPI_Datatype get_MPI_datatype(CFI_cdesc_t *desc);
static size_t get_size(CFI_cdesc_t *desc);
bool error(int ierr, int *stat, char *errmsg, size_t errmsg_len);
static void caf_runtime_error (const char *message, ...);
bool is_contiguous(CFI_cdesc_t *array);
int temp_is_contiguous(const CFI_cdesc_t *descriptor);



// helper functions for tests
template <typename T>
bool arraysAreEquiv(T *arr1, T *arr2, size_t length)
{
   for (int i = 0; i < length; ++i) {
      if (arr1[i] != arr2[i]) {
         return false;
      }
   }

   return true;
}


#endif
