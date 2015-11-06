include ../make.inc

MPICC ?= cc

libcaf_cuda.a: mpi_caf.o ../common/caf_auxiliary.o
	ar rcv $@ mpi_caf.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(MPICC) $(CFLAGS) $(MPI_CFLAGS) -I.. -I/opt/nvidia/cudatoolkit7.0/7.0.28-1.0502.10280.4.1/include/ -L/opt/nvidia/cudatoolkit7.0/7.0.28-1.0502.10280.4.1/lib -lcuda -lcudart -c $< -o $@

mpi_caf.o: mpi_caf.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f mpi_caf.o

distclean: clean
	rm -f libcaf_mpi.a
