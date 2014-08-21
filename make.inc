PREFIX_NAME=_gfortran_caf_
FC=gfortran
CC=gcc
MPFC=mpifort

FFLAGS += -O2 -g -fcoarray=lib
CFLAGS += -DPREFIX_NAME=$(PREFIX_NAME) -g -O2 -DHAVE_INT128_T
LDFLAGS +=

SINGLE_CFLAGS += -Wall -Wextra

ARMCI_DIR =
ARMCI_CFLAGS  += -I$(ARMCI_DIR)/include/ -Werror -Wall -Wextra
ARMCI_LDFLAGS += -L$(ARMCI_DIR)/lib -L$(ARMCI_DIR)/lib64 -larmci
ARMCI_CC = mpicc
ARMCI_RUN = mpirun -np 2

MPI_CFLAGS += -Wall -Wextra -Wno-error=cpp -Wno-error=unused-parameter
#MPICC =
MPI_RUN = mpirun -np 2

GASNET_CFLAGS +=
GASNET_MAK = /home/rouson/Downloads/GASNet-1.22.4/smp-conduit/smp-par.mak
GASNET_LDFLAGS +=$(GASNET_LIBS)
GASNET_RUN = mpirun -np 2

