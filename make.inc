PREFIX_NAME=_gfortran_caf_
FC=gfortran
MPFC=mpifort

FFLAGS += -O2 -g
CFLAGS += -DPREFIX_NAME=$(PREFIX_NAME) -g -Wall -O2 -DHAVE_INT128_T

SINGLE_CFLAGS += -Werror

ARMCI_DIR    = /opt/ga/5.3/gnu/4.9
ARMCI_CFLAGS  += -I$(ARMCI_DIR)/include/ -Werror
ARMCI_LDFLAGS += -larmci -L$(ARMCI_DIR)/lib -L$(ARMCI_DIR)/lib64

MPI_CFLAGS += -Werror
#MPICC =

GASNET_CFLAGS +=
GASNET_MAK=/opt/gasnet/openmpi/1.8.0/gnu/4.9-caf/include/mpi-conduit/mpi-par.mak
GASNET_LDFLAGS +=$(GASNET_LIBS)
