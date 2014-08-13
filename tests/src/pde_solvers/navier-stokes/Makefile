# Choose desired compiler (options: gnu, intel, cray):
compiler=gnu
executable=cshear
fft=fft_sse.a
opencoarrays_dir=/opt/opencoarrays/lib

ifeq ($(compiler),gnu)
  opt=-L $(opencoarrays_dir)
  libcaf_dir = $(opencoarrays_dir)
  compile=mpif90 -fcoarray=lib -L $(libcaf_dir)
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

$(executable): cshear.o $(objects) Makefile
	$(compile) $(opt) cshear.o $(objects) -o $(executable) $(lib) $(fft)

cshear.o: cshear.f90 $(objects) Makefile
	$(compile) -c cshear.f90

global_field.o: global_field.f90 local_field.o Makefile
	$(compile) -c global_field.f90

local_field.o: local_field.f90 Makefile
	$(compile) $(OPTS) -c local_field.f90

.PHONY : clean
clean :
	-rm -f  cshear.o *.mod $(executable) core fort.* 
