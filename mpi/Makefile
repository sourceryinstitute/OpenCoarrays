include ../make.inc

MPICC = mpicc

libcaf_mpi: mpi_caf.o ../caf_auxiliary.o
	ar rcv $@ mpi_caf.o ../caf_auxiliary.o
	ranlib $@

../caf_auxiliary.o: ../caf_auxiliary.c
	$(MPICC) $(CFLAGS) -c $< -o $@

mpi_caf.o: mpi_caf.c ../libcaf.h	
	$(MPICC) $(CFLAGS) $(ARMCI_CFLAGS) -I.. -c $< -o $@

clean:
	rm -f mpi_caf.o

distclean: clean
	rm -f libcaf_mpi.a ../caf_auxiliary.o
