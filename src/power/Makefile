MPICXX := mpic++ -std=c++17

caf_init_finalize: caf_init_finalize.o

# Compile object files
%.o: %.cpp
	$(MPICXX)  -c $< -L ./  -lcaf_mpi

clean:
	rm -f *.o
	rm CafInitFinalize
