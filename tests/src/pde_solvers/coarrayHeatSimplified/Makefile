executable=co_heat
#Cray compiler command
cray=ftn -ew -h caf
#Intel compiler command
intel=ifort -coarray=shared -standard-semantics -O3 -coarray-num-images=2
#GNU compiler command
gnu=gfortran -fcoarray=lib
#Change the name in parenthesis to choose one of the above compilers:
compile=$(gnu)

objects = local_field.o global_field.o 

$(executable): main.o $(objects) Makefile
	$(compile) main.o $(objects) -o $(executable) 

main.o: main.f90 $(objects) Makefile
	$(compile) -c main.f90

global_field.o: global_field.f90 local_field.o Makefile
	$(compile) -c global_field.f90

local_field.o: local_field.f90 Makefile
	$(compile) $(OPTS) -c local_field.f90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.* 
