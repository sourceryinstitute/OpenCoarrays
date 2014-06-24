# Choose desired compiler (options: gnu, intel, cray, tau_intel):
compiler=gnu
include_path=../include-files
library_path=../library
executable= burgers_static_caf

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

OPTS=
objects = field.o periodic_2nd_order.o kind_parameters.o co_object_interface.o object_interface.o math_constants.o ForTrilinos_assertion_utility.o ForTrilinos_error.o

$(executable): main.o $(objects) Makefile
	$(compile) $(opt)  main.o $(objects) -o $(executable)  $(lib)

main.o: main.F90 $(objects) Makefile
	$(compile) $(OPTS) -c main.F90

periodic_2nd_order.o: periodic_2nd_order.F90 field.o kind_parameters.o co_object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o preprocessor_definitions Makefile
	$(compile) $(OPTS) -c periodic_2nd_order.F90

field.o: field.F90 kind_parameters.o math_constants.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o Makefile
	$(compile) $(OPTS) -c field.F90

kind_parameters.o: kind_parameters.F90 Makefile
	$(compile) $(OPTS) -c kind_parameters.F90

math_constants.o: math_constants.F90 kind_parameters.o preprocessor_definitions Makefile
	cp $(include_path)/intel_capabilities.txt compiler_capabilities.txt
	$(compile) $(OPTS) -c math_constants.F90

object_interface.o: $(library_path)/object_interface.F90 preprocessor_definitions Makefile
	$(compile) $(OPTS) -c $(library_path)/object_interface.F90

co_object_interface.o: $(library_path)/co_object_interface.F90 Makefile
	$(compile) $(OPTS) -c $(library_path)/co_object_interface.F90

ForTrilinos_assertion_utility.o: $(library_path)/ForTrilinos_assertion_utility.F90 preprocessor_definitions Makefile 
	$(compile) $(OPTS) -c $(library_path)/ForTrilinos_assertion_utility.F90

ForTrilinos_error.o: $(library_path)/ForTrilinos_error.F90 Makefile
	$(compile) $(OPTS) -c $(library_path)/ForTrilinos_error.F90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.* compiler_capabilities.txt $(library_path)/compiler_capabilities.txt profile* MULT* *.trc *.edf

preprocessor_definitions: 
ifeq ($(compiler),gnu)
	cp $(include_path)/gfortran_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/gfortran_capabilities.txt $(library_path)/compiler_capabilities.txt
else
  ifeq ($(compiler),intel)
	cp $(include_path)/intel_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/intel_capabilities.txt $(library_path)/compiler_capabilities.txt
  else
      ifeq ($(compiler),cray)
	cp $(include_path)/cray_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/cray_capabilities.txt $(library_path)/compiler_capabilities.txt
      endif
  endif
endif

ifeq ($(compiler),tau_gnu)
	cp $(include_path)/gfortran_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/gfortran_capabilities.txt $(library_path)/compiler_capabilities.txt
else
  ifeq ($(compiler),tau_intel)
	cp $(include_path)/intel_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/intel_capabilities.txt $(library_path)/compiler_capabilities.txt
  else
      ifeq ($(compiler),tau_cray)
	cp $(include_path)/cray_capabilities.txt compiler_capabilities.txt
	cp $(include_path)/cray_capabilities.txt $(library_path)/compiler_capabilities.txt
      endif
  endif
endif
