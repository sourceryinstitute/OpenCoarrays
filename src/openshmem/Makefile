include ../make.inc

MPICC ?= oshcc

libcaf_openshmem.a: openshmem_caf.o ../common/caf_auxiliary.o
	ar rcv $@ openshmem_caf.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(MPICC) $(CFLAGS) $(MPI_CFLAGS) -I.. -c $< -o $@

openshmem_caf.o: openshmem_caf.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f openshmem_caf.o

distclean: clean
	rm -f libcaf_openshmem.a
