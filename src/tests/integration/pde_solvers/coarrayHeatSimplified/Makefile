# Choose desired compiler (options: gnu, intel, cray):
compiler=gnu
executable=co_heat

ifeq ($(compiler),gnu)
  compile=caf
else
  ifeq ($(compiler),intel)
    compile=ifort -coarray=shared -standard-semantics -O3 -coarray-num-images=2
  else
    ifeq ($(compiler),cray)
      compile=ftn -ew -h caf
    endif
  endif
endif

objects = local_field.o global_field.o

$(executable): main.o $(objects) Makefile
	$(compile) $(opt) main.o $(objects) -o $(executable) $(lib)

main.o: main.f90 $(objects) Makefile
	$(compile) -c main.f90

global_field.o: global_field.f90 local_field.o Makefile
	$(compile) -c global_field.f90

local_field.o: local_field.f90 Makefile
	$(compile) $(OPTS) -c local_field.f90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.*
