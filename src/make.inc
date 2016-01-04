PREFIX_NAME=_gfortran_caf_
FC=gfortran
CC=gcc
MPFC=mpifort

FFLAGS = -O2 -g
CFLAGS = -O2 -g
FFLAGS_EXTRA = -fcoarray=lib
CFLAGS_EXTRA = -DPREFIX_NAME=$(PREFIX_NAME) -DHAVE_INT128_T
FFLAGS += $(FFLAGS_EXTRA)
CFLAGS += $(CFLAGS_EXTRA)
LDFLAGS +=

SINGLE_CFLAGS += -Wall -Wextra

ARMCI_DIR =
ARMCI_CFLAGS  += -I$(ARMCI_DIR)/include/ -Werror -Wall -Wextra
ARMCI_LDFLAGS += -L$(ARMCI_DIR)/lib -L$(ARMCI_DIR)/lib64 -larmci
ARMCI_CC = mpicc
ARMCI_RUN = mpirun -np 2

MPI_EXTRA_FLAGS = -Wall -Wextra -Wno-error=cpp -Wno-error=unused-parameter -DSTRIDED#-DNONBLOCKING_PUT -DCAF_MPI_LOCK_UNLOCK
MPI_CFLAGS += $(MPI_EXTRA_FLAGS)
#MPICC =
MPI_RUN = mpirun -np 2

GASNET_CFLAGS += #-DSTRIDED
GASNET_MAK = /home/rouson/Downloads/GASNet-1.22.4/smp-conduit/smp-par.mak
GASNET_LDFLAGS +=$(GASNET_LIBS)
GASNET_RUN = mpirun -np 2
