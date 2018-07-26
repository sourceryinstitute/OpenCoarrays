include ../make.inc

all: co_sum-test hello-world

hello-world: hello-world-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort hello-world-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a -o hello-world

co_sum-test: co_sum-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort co_sum-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a -o co_sum-test

co_sum-transformed.o: co_sum-transformed.F90 opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort -c co_sum-transformed.F90

hello-world-transformed.o: hello-world-transformed.F90 opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort -c hello-world-transformed.F90

opencoarrays.o: opencoarrays.F90
	mpifort -c opencoarrays.F90

../mpi/libcaf_mpi.a: ../mpi/mpi_caf.c
	make -C ../mpi

clean:
	rm -f *.o *.mod co_sum-test hello-world ../common/caf_auxiliary.o
	make -C ../mpi clean

distclean: clean
	make -C ../mpi distclean
