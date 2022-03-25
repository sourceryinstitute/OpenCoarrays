# Edit the line below to the opencarrays installation directory
opencoarrays=/opt/opencoarrays
executable=hello_multiverse
opt=-L $(opencoarrays)/lib
compile=mpifort -fcoarray=lib
lib=-lcaf_mpi

$(executable): hello_multiverse.o  Makefile
	$(compile) $(opt) hello_multiverse.o -o $(executable) $(lib)

hello_multiverse.o: hello_multiverse.F90  Makefile
	$(compile) -c hello_multiverse.F90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable)
