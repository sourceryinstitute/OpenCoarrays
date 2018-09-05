include ../make.inc

MPICC ?= mpicc
MPIC++ ?= mpic++
AITUNING_DIR = ./AITuning/lib/libaituning.a
AITUNING_LIB = 

libcaf_mpi.a: mpi_caf.o AITuning_OpenCoarrays.o ../common/caf_auxiliary.o
	ar rcv $@ mpi_caf.o AITuning_OpenCoarrays.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(MPICC) $(CFLAGS) $(MPI_CFLAGS) -I.. -c $< -o $@
.cpp.o:
	$(MPIC++) $(CFLAGS) $(MPI_CFLAGS) -I.. -I./AITuning/include -c $< -o $@

mpi_caf.o: mpi_caf.c AITuning_OpenCoarrays.o ../libcaf.h ../libcaf-gfortran-descriptor.h $(AITUNING_DIR) $(AITUNING_LIB)

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f *.o

distclean: clean
	rm -f libcaf_mpi.a
