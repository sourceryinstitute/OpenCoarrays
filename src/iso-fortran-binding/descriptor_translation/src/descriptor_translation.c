/*
   OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
   OpenCoarrays -- ISO_Fortran_binding standard-compliant interoperability with C.
   Copyright (c) 2018, Sourcery, Inc.
   Copyright (c) 2018, Sourcery Institute
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
   3. Neither the names of the copyright holders nor the names of
      their contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HnnnOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file descriptor_translation.c
 * \author Katherine Rasmussen
 * \brief Translations between standard Fortran descriptors and gfortran descriptors
 *
 */

#include "descriptor_translation.h"

// unclear how to deal with offset and span
// as no information is coming from the fortran descriptor
#define OFFSET -1
#define SPAN    0


void gfc_to_iso_fortran(gfc_descriptor_t *gfc, CFI_cdesc_t *iso)
{
   int i;

   iso->base_addr = gfc->base_addr;
   iso->elem_len  = gfc->dtype.elem_len;
   iso->version   = gfc->dtype.version;

   iso->rank      = (CFI_rank_t)      gfc->dtype.rank;
   iso->attribute = (CFI_attribute_t) gfc->dtype.attribute;        
   iso->type      = (CFI_type_t)      gfc->dtype.type;            

   for (i = 0; i < gfc->dtype.rank; i++) {
      iso->dim[i].lower_bound = gfc->dim[i].lower_bound;
      iso->dim[i].extent      = gfc->dim[i]._ubound - gfc->dim[i].lower_bound + 1;
      iso->dim[i].sm          = gfc->dim[i]._stride;
   }
}

void iso_fortran_to_gfc(CFI_cdesc_t *iso, gfc_descriptor_t *gfc)
{
   int i;

   gfc->base_addr      = iso->base_addr;
   gfc->dtype.elem_len = iso->elem_len;
   gfc->dtype.version  = iso->version;

   // unclear how to deal with offset and span
   // as no information is coming from the fortran descriptor
   gfc->offset         = OFFSET;
   gfc->span           = SPAN; 

   gfc->dtype.rank      = (signed char)  iso->rank;
   gfc->dtype.attribute = (signed short) iso->attribute;
   gfc->dtype.type      = (signed char)  iso->type;  
   
   for (i = 0; i < iso->rank; i++) {
      gfc->dim[i].lower_bound = iso->dim[i].lower_bound;
      gfc->dim[i]._ubound     = iso->dim[i].lower_bound + iso->dim[i].extent - 1;
      gfc->dim[i]._stride     = iso->dim[i].sm;
   }
}
