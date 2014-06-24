# Choose desired compiler (options: gnu, intel, cray, tau_intel):
compiler=gnu
include_path=../include-files
library_path=../library
executable=hello_multiverse

ifeq ($(compiler),gnu)
  opencoarrays_dir=../../../../../
  opt=-L $(opencoarrays_dir)/dev/mpi
  libcaf_dir = $(opencoarrays_dir)/dev/
  compile=mpif90 -fcoarray=lib -L $(libcaf_dir)
  lib=-lcaf_mpi
else
  ifeq ($(compiler),intel)
    compile=ifort -coarray=shared -standard-semantics -O3 -coarray-num-images=2
  else
    ifeq ($(compiler),cray)
      compile=ftn -ew -h caf
    else
      ifeq ($(compiler),tau_intel)
        compile=tauf90 -tau:serial,icpc,pdt  -coarray=shared -standard-semantics -O3 -coarray-num-images=4 -DTAU=1
      else
        ifeq ($(compiler),tau_aciss)
          tau_aciss=tau_f90.sh -v -Bdynamic -coarray=shared -standard-semantics -O3
        else
          ifeq ($(compiler),tau_cray)
            tau_hopper=tau_f90.sh -optCompInst -ew -h caf -g
          endif
        endif
      endif
    endif
  endif
endif

$(executable): hello_multiverse.o  Makefile
	$(compile) $(opt) hello_multiverse.o -o $(executable) $(lib)

hello_multiverse.o: hello_multiverse.F90  Makefile
	$(compile) -c hello_multiverse.F90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable)

