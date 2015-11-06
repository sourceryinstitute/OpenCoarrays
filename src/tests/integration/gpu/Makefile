executable=co_dot

compile_fortran=/lustre/atlas/proj-shared/csc109/gcc/bin/gfortran -fcoarray=lib
compile_cuda=nvcc
link=ftn

API_PATH=$(OPENCOARRAYS_SRC_DIR)/src/extensions/
TIMER_PATH=$(OPENCOARRAYS_SRC_DIR)/src/tests/integration/pde_solvers/navier-stokes

OPTS=-L$(OPENCOARRAYS_SRC_DIR)/src/cuda_mpi 
objects = opencoarrays.o co_dot.o kernelCaller.o $(TIMER_PATH)/walltime.o

$(executable): $(objects) Makefile
	$(link) $(OPTS) $(objects) -o $(executable) -lcaf_cuda

opencoarrays.o: $(API_PATH)/opencoarrays.F90 Makefile
	$(compile_fortran) -c $(API_PATH)/opencoarrays.F90 -DCOMPILER_PROVIDES_MPI

kernelCaller.o: kernelCaller.cu Makefile
	$(compile_cuda) -c kernelCaller.cu

co_dot.o: co_dot.f90 Makefile
	$(compile_fortran) -c co_dot.f90


clean:
	rm $(executable) *.o *.mod
