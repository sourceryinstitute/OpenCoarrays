include ../make.inc

MPICC ?= mpicc

libcaf_mpi.a: mpi_caf.o ../common/caf_auxiliary.o
	ar rcv $@ mpi_caf.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(MPICC) $(CFLAGS) $(MPI_CFLAGS) -I.. -c $< -o $@

mpi_caf.o: mpi_caf.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f mpi_caf.o

distclean: clean
	rm -f libcaf_mpi.a
