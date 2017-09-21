mofo_root=..
library_root=../library
executable=burgers_mpi

#include TAU_MAKEFILE $TAU_ROOT_DIR/x86_64/lib/selected_make_file

# For benchmark purposes the -DBENCHMARK flag is required.
# Another option is to add #define BENCHMARK to “compiler_capabilities.txt”
# but it is not recommended.
# A quick way to activate the benchmark mode is through make CMDFLAGS=-DBENCHMARK.

# Note: A benchmark purpose is the comparison among this solver and the
# Coarrays counterpart.

#Cray compiler command
cray=ftn -ew -h caf
#GNU compiler command
gnu=mpifort -fcoarray=lib -ffree-form -ffree-line-length-none

#Intel compiler command
intel= tau_f90.sh -DTAU=1 -Bdynamic -standard-semantics -O3 -g -optCompInst
intel= mpiifort -Bdynamic -standard-semantics -O3 -L/usr/lib64 #-lpmi

#TAU compiler command
tau=tauf90 -tau:serial,icpc,pdt -DTAU=1 -Bdynamic -standard-semantics -O3 -g -optCompInst
#TAU compiler command cray
tau_cray=tau_f90.sh -optCompInst -ew

compile=$(gnu)
#compile=$(intel)
#compile=$(cray)
#compile=$(tau_cray)
# PLEASE UNCOMMENT THE # TO USE TAU

OPTS=
objects = periodic_2nd_order.o mpi_module.o shared.o mpi_share.o kind_parameters.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o input_file.o
CFLAGS = -c  $(CMDFLAGS)

$(executable): main.o $(objects) Makefile
	$(compile) $(OPTS)  main.o $(objects) -o $(executable) -lcaf_mpi

main.o: main.F90 $(objects) Makefile
	$(compile) $(OPTS) $(CFLAGS) main.F90

periodic_2nd_order.o: periodic_2nd_order.F90 preprocessor_definitions object_interface.o shared.o mpi_share.o kind_parameters.o ForTrilinos_assertion_utility.o ForTrilinos_error.o mpi_module.o input_file.o Makefile
	$(compile) $(OPTS)$(CFLAGS) periodic_2nd_order.F90

mpi_share.o: mpi_share.F90 Makefile mpi_module.o
	$(compile) $(OPTS) $(CFLAGS) mpi_share.F90

mpi_module.o: mpi_module.F90 kind_parameters.o object_interface.o shared.o ForTrilinos_assertion_utility.o ForTrilinos_error.o Makefile
	$(compile) $(OPTS) $(CFLAGS) mpi_module.F90

shared.o: shared.F90 Makefile kind_parameters.o
	$(compile) $(OPTS) $(CFLAGS) shared.F90


kind_parameters.o: kind_parameters.F90 Makefile
	$(compile) $(OPTS)$(CFLAGS)  kind_parameters.F90

input_file.o: input_file.F90 Makefile
	$(compile) $(OPTS) $(CFLAGS) input_file.F90

object_interface.o: $(library_root)/object_interface.F90 kind_parameters.o Makefile preprocessor_definitions
	$(compile) $(OPTS) $(CFLAGS) $(library_root)/object_interface.F90

ForTrilinos_assertion_utility.o: $(library_root)/ForTrilinos_assertion_utility.F90 Makefile
	$(compile) $(OPTS) $(CFLAGS) $(library_root)/ForTrilinos_assertion_utility.F90

ForTrilinos_error.o: $(library_root)/ForTrilinos_error.F90 Makefile
	$(compile) $(OPTS)$(CFLAGS) $(library_root)/ForTrilinos_error.F90


.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.* compiler_capabilities.txt profile* MULT* *.trc *.edf $(library_root)/compiler_capabilities.txt

preprocessor_definitions:
ifeq ($(compile),$(gnu))
	cp $(mofo_root)/include-files/gfortran_capabilities.txt compiler_capabilities.txt
	cp $(mofo_root)/include-files/gfortran_capabilities.txt $(library_root)/compiler_capabilities.txt
else
  ifeq ($(compile),$(intel))
	cp $(mofo_root)/include-files/intel_capabilities.txt compiler_capabilities.txt
	cp $(mofo_root)/include-files/intel_capabilities.txt $(library_root)/compiler_capabilities.txt
  else
    ifeq ($(compile),$(cray))
	cp $(mofo_root)/include-files/cray_capabilities.txt compiler_capabilities.txt
	cp $(mofo_root)/include-files/cray_capabilities.txt $(library_root)/compiler_capabilities.txt
    endif
  endif
endif
ifeq ($(compile),$(tau))
	cp $(mofo_root)/include-files/intel_capabilities.txt compiler_capabilities.txt
	cp $(mofo_root)/include-files/intel_capabilities.txt $(library_root)/compiler_capabilities.txt
endif
ifeq ($(compile),$(tau_cray))
	cp $(mofo_root)/include-files/tau_cray_capabilities.txt compiler_capabilities.txt
	cp $(mofo_root)/include-files/tau_cray_capabilities.txt $(library_root)/compiler_capabilities.txt
endif
