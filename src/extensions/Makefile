co_sum-transformed: co_sum-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort co_sum-transformed.o opencoarrays.o ../mpi/libcaf_mpi.a -o co_sum-transformed 

co_sum-transformed.o: co_sum-transformed.F90 opencoarrays.o ../mpi/libcaf_mpi.a
	mpifort -c co_sum-transformed.F90

opencoarrays.o: opencoarrays.F90
	mpifort -c opencoarrays.F90

../mpi/libcaf_mpi.a: ../mpi/mpi_caf.c
	cd ../mpi && make

clean:
	rm -f *.o *.mod co_sum-transformed ../common/caf_auxiliary.o
	cd ../mpi && make clean

distclean: clean
	cd ../mpi && make distclean
