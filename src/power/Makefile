MPICXX := mpic++ -std=c++17
ARCHIVE := ar
LIBRARY_TARGET := libcaf_mpi.a

init_finalize_test := caf_init_finalize
error_stop_test := error_stop
num_images_test := num_images
this_image_test := this_image
sync_all_test := sync_all
co_min_test := co_min
co_max_test := co_max
co_sum_test := co_sum
co_broadcast_test := co_broadcast

tests := $(init_finalize_test) $(error_stop_test) $(num_images_test) $(this_image_test) $(sync_all_test) $(co_min_test) $(co_max_test) $(co_sum_test) $(co_broadcast_test)

all: $(tests)

# $<  expands to just the first prerequisite
$(init_finalize_test): test_caf_init_finalize.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(init_finalize_test)

$(error_stop_test): test_error_stop.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(error_stop_test)

$(num_images_test): test_num_images.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(num_images_test)

$(this_image_test): test_this_image.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(this_image_test)

$(sync_all_test): test_sync_all.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(sync_all_test)

$(co_min_test): test_co_min.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(co_min_test)

$(co_max_test): test_co_max.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(co_max_test)

$(co_sum_test): test_co_sum.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(co_sum_test)

$(co_broadcast_test): test_co_broadcast.o $(LIBRARY_TARGET)
	$(MPICXX)  $< -L ./ -lcaf_mpi -o $(co_broadcast_test)

# Build library
# $@  expands to the target
# $^  expands to all prerequisites
$(LIBRARY_TARGET): mpi_caf_core.o mpi_caf_collectives.o
	$(ARCHIVE) rcs $@ $^

mpi_caf_core.o: mpi_caf_core.cpp
	$(MPICXX)  -c -o $@ $<

mpi_caf_collectives.o: mpi_caf_collectives.cpp
	$(MPICXX)  -c -o $@ $<

# Compile object files
%.o: %.cpp
	$(MPICXX)  -c $<

check:
	mpirun ./caf_init_finalize
	mpirun -np 3 ./num_images
	mpirun ./num_images
	mpirun -np 8 ./this_image
	mpirun ./this_image
	mpirun -np 8 ./sync_all
	mpirun ./sync_all
	mpirun -np 2 ./co_sum
	mpirun -np 3 ./co_min
	mpirun -np 4 ./co_max
	mpirun -np 3 ./co_broadcast

clean:
	rm -f *.o a.out
	rm $(tests) $(LIBRARY_TARGET)
