MPICXX := mpic++ -std=c++17
ARCHIVE := ar
LIBRARY_TARGET := libcaf_mpi.a

init_finalize_test := caf_init_finalize
error_stop_test := error_stop
num_images_test := num_images

tests := $(init_finalize_test) $(error_stop_test) $(num_images_test)

all: $(tests)

# $<  expands to just the first prerequisite
$(init_finalize_test): test_caf_init_finalize.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(init_finalize_test)

$(error_stop_test): test_error_stop.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(error_stop_test)

$(num_images_test): test_num_images.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(num_images_test)

# Build library
# $@  expands to the target
# $^  expands to all prerequisites
$(LIBRARY_TARGET): mpi_caf_core.o
	$(ARCHIVE) rcs $@ $^

mpi_caf_core.o: mpi_caf_core.cpp
	$(MPICXX)  -c -o $@ $<

# Compile object files
%.o: %.cpp
	$(MPICXX)  -c $<

clean:
	rm -f *.o a.out
	rm $(tests) $(LIBRARY_TARGET)
