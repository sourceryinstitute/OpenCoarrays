# Choose desired compiler (options: gnu, intel, cray):
compiler=gnu
fft=fft_sse.a
opencoarrays_dir=/home/alex/Documents/opencoarrays/mpi

ifeq ($(compiler),gnu)
  opt=-L $(opencoarrays_dir) -fcoarray=lib 
  libcaf_dir = $(opencoarrays_dir)
  compile=mpif90 
  lib=-lcaf_mpi
else
  ifeq ($(compiler),intel)
    compile=ifort -coarray=shared -standard-semantics -O3 -coarray-num-images=2
  else
    ifeq ($(compiler),cray)
      compile=ftn -ew -h caf
    endif
  endif
endif

objects = walltime.o 

all: coarray-shear mpi-shear

coarray-shear: coarray-shear.o $(objects) Makefile
	$(compile) $(opt) -pg coarray-shear.o $(objects) -o coarray-shear $(lib) $(fft)

mpi-shear: mpi-shear.o $(objects) Makefile
	$(compile) -pg mpi-shear.o $(objects) -o mpi-shear $(fft)

coarray-shear.o: coarray-shear.f90 $(objects) Makefile
	$(compile) $(opt) -pg -c coarray-shear.f90 

mpi-shear.o: mpi-shear.f90 $(objects) Makefile
	$(compile) -pg -c mpi-shear.f90

.PHONY : clean
clean :
	-rm -f  coarray-shear.o coarray-shear mpi-shear.o mpi-shear coarray-shear core fort.* *.mod
