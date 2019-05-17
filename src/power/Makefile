MPICXX := mpic++ -std=c++17
ARCHIVE := ar
LIBRARY_TARGET := libcaf_mpi.a

caf_init_finalize: caf_init_finalize.o

# Build library
$(LIBRARY_TARGET): mpi_caf_core.o
	$(ARCHIVE) rcs $@ $^

# Compile object files
%.o: %.cpp
	$(MPICXX)  -c $< -L ./  -lcaf_mpi

clean:
	rm -f *.o
	rm caf_init_finalize lib$(LIBRARY_TARGET).a
