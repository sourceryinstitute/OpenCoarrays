PREFIX_NAME=_gfortran_caf_
FC=gfortran
MPFC=mpifort

FFLAGS += -O2 -g -fcoarray=lib
CFLAGS += -DPREFIX_NAME=$(PREFIX_NAME) -g -O2 -DHAVE_INT128_T

SINGLE_CFLAGS += -Werror -Wall -Wextra

ARMCI_DIR =
ARMCI_CFLAGS  += -I$(ARMCI_DIR)/include/ -Werror -Wall -Wextra
ARMCI_LDFLAGS += -L$(ARMCI_DIR)/lib -L$(ARMCI_DIR)/lib64 -larmci
ARMCI_CC = mpicc
ARMCI_RUN = mpirun -np 2

MPI_CFLAGS += -Werror -Wall -Wextra
#MPICC =
MPI_RUN = mpirun -np 2

GASNET_CFLAGS +=
GASNET_MAK= #/opt/gasnet/openmpi/1.8.0/gnu/4.9-caf/include/mpi-conduit/mpi-par.mak
GASNET_LDFLAGS +=$(GASNET_LIBS)
GASNET_RUN = mpirun -np 2
