# make.inc
#
# -- This make include file defines variables for use in the OpenCoarrays 
#    static Makefile.  Please edit this file before using the Makefile.)
#
# OpenCoarrays is distributed under the OSI-approved BSD 3-clause License:
# Copyright (c) 2015-2016, Sourcery, Inc.
# Copyright (c) 2015-2016, Sourcery Institute
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice, this
#    list of conditions and the following disclaimer in the documentation and/or
#    other materials provided with the distribution.
# 3. Neither the names of the copyright holders nor the names of their contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


#
# Usage:
#
#    export COMPILER=arm
#    make

# Example of printing a variable named "var" delimited by square braces
# $(info $$var is [${var}])

# Options: cray, gnu, intel, arm
ifndef COMPILER
        COMPILER := gnu
endif

ifeq ($(COMPILER),gnu)
  PREFIX_NAME := _gfortran_caf_
else
  PREFIX_NAME := _opencoarrays_
endif

ifeq ($(COMPILER),gnu)
  FC=gfortran
  CC=mpicc
  MPFC = mpifort
  MPICC = mpicc
  OSHCC = oshcc
  FFLAGS_EXTRA = -fcoarray=lib
  MPI_EXTRA_FLAGS = -Wall -Wextra -Wno-error=cpp -Wno-error=unused-parameter -DSTRIDED -DUSE_FAILED_IMAGES  #-DNONBLOCKING_PUT -DCAF_MPI_LOCK_UNLOCK
  MPI_RUN = mpiexec -np 2
else
  ifeq ($(COMPILER),cray)
    FC=ftn
    CC=cc
    MPFC=ftn
    MPICC = cc
  else
    ifeq ($(COMPILER),arm)
      MPFC = mpifort
      MPICC = mpicc
    endif
  endif
endif

FFLAGS = -O2 -g
CFLAGS = -O2 -g

CFLAGS_EXTRA = -DPREFIX_NAME=$(PREFIX_NAME) -DHAVE_INT128_T
FFLAGS_EXTRA = -DPREFIX_NAME=$(PREFIX_NAME) -DHAVE_INT128_T
FFLAGS += $(FFLAGS_EXTRA)
CFLAGS += $(CFLAGS_EXTRA)
LDFLAGS +=

SINGLE_CFLAGS += -Wall -Wextra

MPI_EXTRA_FLAGS = -DSTRIDED # -DNONBLOCKING_PUT -DCAF_MPI_LOCK_UNLOCK
MPI_CFLAGS += $(MPI_EXTRA_FLAGS)

GASNET_CFLAGS += # -DSTRIDED
GASNET_PATH =
GASNET_MAK = $(GASNET_PATH)/GASNet-1.22.4/smp-conduit/smp-par.mak
GASNET_LDFLAGS +=$(GASNET_LIBS)
GASNET_RUN = mpiexec -np 2
