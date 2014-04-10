PREFIX_NAME=_gfortran_caf_

SINGLE_CFLAGS +=
ARMCI_CFLAGS += -I/home/tob/projects/armci-inst/include -L/home/tob/projects/armci-inst/lib64
GASNET_CFLAGS += -Icaf -std=gnu99
GASNET_MAK = /home/tob/projects/gasnet-inst/include/mpi-conduit/mpi-seq.mak
MPI_CFLAGS =
MPICC =

CFLAGS += -DPREFIX_NAME=$(PREFIX_NAME) -g -Wall -Werror -O2
