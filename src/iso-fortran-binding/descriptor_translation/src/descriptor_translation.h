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
 * \file descriptor_translation.h
 * \author Katherine Rasmussen
 * \brief Translations between standard Fortran descriptors and gfortran descriptors
 *
 */

#define GCC_GE_8

#ifndef DESCRIPTOR_TRANSLATION_H_
#define DESCRIPTOR_TRANSLATION_H_

#include <stddef.h>
#include "libcaf-gfortran-descriptor.h"
#include "ISO_Fortran_binding.h"


/** \brief Translates a gfortan descriptor to a 
 *         standard Fortran descriptor.
 *  \param gfc_descriptor_t The gfortan descriptor
 *         that will be translated.
 *  \param CFI_cdesc_t The standard Fortran descriptor
 *         that will be filled in with the translated information.
 *  \return Void.
 */
void gfc_to_iso_fortran(gfc_descriptor_t *gfc, CFI_cdesc_t *iso);
/** \brief Translates a standard Fortran descriptor to a 
 *         gfortran descriptor.
 *  \param CFI_cdesc_t The standard Fortran descriptor
 *         that will be translated.
 *  \param gfc_descriptor_t The gfortan descriptor
 *         that will be filled in with the translated information.
 *  \return Void.
 */
void iso_fortran_to_gfc(CFI_cdesc_t *iso, gfc_descriptor_t *gfc);


#endif /* DESCRIPTOR_TRANSLATION_H_ */
