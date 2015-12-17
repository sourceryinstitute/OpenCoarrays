# Choose desired platform (options: virtual, cees):
machine=virtual
# Choose desired compiler (options: gnu, intel, cray, pg, ibm, nag, tau_intel, tau_aciss, tau_cray):
compiler=gnu

executable=burgers
library_path=./library
include_path=./include-files

ifeq ($(compiler),gnu)
# Use these two lines to compile executables that will ONLY work in single-image runs
# lib=-lcaf_single
# compile=gfortran -fcoarray=single
  lib=-lcaf_mpi
  compile=caf -fcoarray=lib
  compile=/Users/rouson/Code/builds/oc/bin/caf -fcoarray=lib
  ifeq ($(machine),virtual)
    opt=-L ../../../../mpi
  else
    ifeq ($(machine),cees)
    opt=-L /opt/opencoarrays-src/mpi
    endif
  endif
else
  ifeq ($(compiler),intel)
    compile=ifort -standard-semantics -O3
  else
    ifeq ($(compiler),cray)
      compile=ftn -ew
    else
      ifeq ($(compiler),tau_intel)
        compile=tauF90 -tau:serial,icpc,pdt  -standard-semantics -O3 -DTAU=1
      else
        ifeq ($(compiler),tau_aciss)
          compile=tau_F90.sh -standard-semantics -O3
        else
          ifeq ($(compiler),tau_cray)
            compile=tau_F90.sh -optCompInst -ew
          else
            ifeq ($(compiler),pg)
              compile=pgfortran -Mallocatable=03
            else
              ifeq ($(compiler),ibm)
                compile=xlf2003
              else
                ifeq ($(compiler),nag)
                  compile=nagfor -f2008 -fpp -C=all
                endif
              endif
            endif
          endif
        endif
      endif
    endif
  endif
endif

objects = local_field.o global_field.o co_object_interface.o object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o

$(executable): main.o $(objects) Makefile
	$(compile) $(opt)  main.o $(objects) -o $(executable)  $(lib)

main.o: main.F90 $(objects) ForTrilinos_assertion_utility.o ForTrilinos_error.o global_field.o Makefile
	$(compile)  -c main.F90

global_field.o: global_field.F90 local_field.o co_object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o preprocessor_definitions Makefile
	$(compile)  -c global_field.F90

local_field.o: local_field.F90 object_interface.o ForTrilinos_assertion_utility.o ForTrilinos_error.o Makefile
	$(compile)  -c local_field.F90

object_interface.o: $(library_path)/object_interface.F90 preprocessor_definitions Makefile
	$(compile)  -c $(library_path)/object_interface.F90

co_object_interface.o: $(library_path)/co_object_interface.F90 Makefile
	$(compile)  -c $(library_path)/co_object_interface.F90

ForTrilinos_assertion_utility.o: $(library_path)/ForTrilinos_assertion_utility.F90 Makefile
	$(compile)  -c $(library_path)/ForTrilinos_assertion_utility.F90

ForTrilinos_error.o: $(library_path)/ForTrilinos_error.F90 Makefile
	$(compile)  -c $(library_path)/ForTrilinos_error.F90

.PHONY : clean
clean :
	-rm -f  *.o *.mod $(executable) core fort.* compiler_capabilities.txt $(library_path)/compiler_capabilities.txt profile.* *.ppk

preprocessor_definitions:
ifeq ($(compiler),gnu)
	cp $(include_path)/gfortran_capabilities.txt $(library_path)/compiler_capabilities.txt
else
  ifeq ($(compiler),intel)
	cp $(include_path)/intel_capabilities.txt $(library_path)/compiler_capabilities.txt
  else
    ifeq ($(compiler),cray)
	cp $(include_path)/cray_capabilities.txt $(library_path)/compiler_capabilities.txt
    endif
  endif
endif
