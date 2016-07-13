# Makefile
#
# -- This Makefile builds the OpenCoarrays libraries using setings in make.inc.
#    (Please edit make.inc before using this Makefile.)
#
#    Instead of using this Makefile, the preferred alternative is to invoke 
#    ./install.sh, which builds the caf compiler wrapper, the cafrun program 
#    launcher, the test suite, and any undetected prerequisite packages, and 
#    which customizes the build for the environment.
#
# Usage (default: GASNET=no):
#    make                      - Builds libcaf_mpi.a, the MPI transport layer.
#    make GASNET=no all        - Builds libcaf_mpi.a and libcaf_gasnet.a.
#    make clean                - Deletes *.o files. Retains *.a files.
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

include make.inc
export

# Conditionally enable the build of the GASNet library libcaf_gasnet.a,
# an unmaintained but previously functional MPI alterntive.
ifeq ($(GASNET),yes)
  gasnet_=gasnet
endif

.PHONY : common
.PHONY : gasnet
.PHONY : mpi
.PHONY : $(gasent_)

all: common mpi $(gasnet_) 

mpi: common
	$(MAKE) -C $@

common:
	$(MAKE) -C $@

ifeq ($(GASNET),yes)
gasnet: common
	$(MAKE) -C $@
endif

# Delete temporary files files:
clean:
	$(MAKE) -k -C mpi clean
	$(MAKE) -k -C common clean
ifeq ($(GASNET),yes)
	$(MAKE) -k -C gasnet clean
endif

distclean: clean
	$(MAKE) -k -C common distclean
	$(MAKE) -k -C mpi distclean
ifeq ($(GASNET),yes)
	$(MAKE) -k -C gasnet distclean
endif
