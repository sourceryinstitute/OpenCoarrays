library_root=./library/
include_root=./include-files/
executable=burgers_caf
#Cray compiler command
cray=ftn -ew -h caf
#Intel compiler command
intel=ifort -Bdynamic -coarray=shared -standard-semantics -O3 -coarray-num-images=2
#intel=ifort -v -coarray=shared -coarray-config-file=configuration 
#GNU compiler command
gnu=gfortran -fcoarray=single
#TAU compiler command
#tau_intel=tauf90 -tau:serial,icpc,pdt  -Bdynamic -coarray=shared -standard-s    emantics -O3 -coarray-num-images=4 -DTAU=1
tau_intel=tau_f90.sh -Bdynamic -coarray=shared -standard-semantics -O3 -optCompInst 
tau_cray=tau_f90.sh -ew -h caf -optCompInst
compile=$(intel)

OPTS=
objects = local_field.o global_field.o kind_parameters.o co_object_interface.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o math_constants.o burgers.o

$(executable): main.o $(objects) Makefile
	$(compile) $(OPTS)  main.o $(objects) -o $(executable) 

main.o: main.F90 $(objects) Makefile
	$(compile) $(OPTS) -c main.F90

global_field.o: global_field.F90 local_field.o kind_parameters.o co_object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o preprocessor_definitions Makefile
	$(compile) $(OPTS) -c global_field.F90

local_field.o: local_field.F90 kind_parameters.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o Makefile
	$(compile) $(OPTS) -c local_field.F90

burgers.o: burgers.F90 kind_parameters.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o math_constants.o Makefile
	$(compile) $(OPTS) -c burgers.F90

math_constants.o: math_constants.o kind_parameters.o Makefile
	$(compile) $(OPTS) -c math_constants.F90

kind_parameters.o: kind_parameters.F90 Makefile
	$(compile) $(OPTS) -c kind_parameters.F90

object_interface.o: $(library_root)/object_interface.F90 preprocessor_definitions Makefile 
	$(compile) $(OPTS) -c $(library_root)/object_interface.F90

co_object_interface.o: $(library_root)/co_object_interface.F90 Makefile
	$(compile) $(OPTS) -c $(library_root)/co_object_interface.F90

ForTrilinos_assertion_utility.o: $(library_root)/ForTrilinos_assertion_utility.F90 Makefile
	$(compile) $(OPTS) -c $(library_root)/ForTrilinos_assertion_utility.F90

ForTrilinos_error.o: $(library_root)/ForTrilinos_error.F90 Makefile
	$(compile) $(OPTS) -c $(library_root)/ForTrilinos_error.F90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.* compiler_capabilities.txt $(library_root)/compiler_capabilities.txt profile* MULT* *.trc *.edf

preprocessor_definitions: 
ifeq ($(compile),$(gnu))
	cp $(include_root)/gfortran_capabilities.txt compiler_capabilities.txt
	cp $(include_root)/gfortran_capabilities.txt $(library_root)/compiler_capabilities.txt
else
  ifeq ($(compile),$(intel))
	cp $(include_root)/intel_capabilities.txt compiler_capabilities.txt
	cp $(include_root)/intel_capabilities.txt $(library_root)/compiler_capabilities.txt
  else
    ifeq ($(compile),$(cray))
	cp $(include_root)/cray_capabilities.txt compiler_capabilities.txt
	cp $(include_root)/cray_capabilities.txt $(library_root)/compiler_capabilities.txt
  else
    ifeq ($(compile),$(tau_cray))
	cp $(include_root)/tau_cray_capabilities.txt compiler_capabilities.txt
	cp $(include_root)/tau_cray_capabilities.txt $(library_root)/compiler_capabilities.txt
  else
    ifeq ($(compile),$(tau_intel))
	cp $(include_root)/tau_intel_capabilities.txt compiler_capabilities.txt
	cp $(include_root)/tau_intel_capabilities.txt $(library_root)/compiler_capabilities.txt
    endif
  endif
  endif
  endif
endif
