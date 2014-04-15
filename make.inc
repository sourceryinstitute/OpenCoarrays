PREFIX_NAME=_gfortran_caf_
FC=gfortran
MPFC=mpifort

FFLAGS += -O
CFLAGS += -DPREFIX_NAME=$(PREFIX_NAME) -g -Wall -O2
SINGLE_CFLAGS +=

ARMCI_DIR    = /opt/ga/5.3/gnu/4.9
ARMCI_CFLAGS  += -I$(ARMCI_DIR)/include/
ARMCI_LDFLAGS += -larmci -L$(ARMCI_DIR)/lib

MPI_CFLAGS +=
#MPICC =

GASNET_CFLAGS +=
GASNET_MAK=/opt/gasnet/openmpi/1.7.3/gnu/4.9-caf/include/mpi-conduit/mpi-par.mak
include $(GASNET_MAK)
GASNET_LDFLAGS +=$(GASNET_LIBS)