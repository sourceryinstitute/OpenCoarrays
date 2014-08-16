# Choose desired compiler (options: gnu, intel, cray):
compiler=gnu
fft=fft_sse.a
opencoarrays_dir=/opt/opencoarrays/lib

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

all: cshear mshear

cshear: cshear.o $(objects) Makefile
	$(compile) $(opt) cshear.o $(objects) -o cshear $(lib) $(fft)

mshear: mshear.o $(objects) Makefile
	$(compile) mshear.o $(objects) -o mshear $(fft)

cshear.o: cshear.f90 $(objects) Makefile
	$(compile) $(opt) -c cshear.f90 

mshear.o: mshear.f90 $(objects) Makefile
	$(compile) -c mshear.f90

.PHONY : clean
clean :
	-rm -f  cshear.o cshear mshear.o mshear cshear core fort.* 
