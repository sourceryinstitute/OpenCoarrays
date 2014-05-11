include ../make.inc

MPICC ?= mpicc

MY_GASNET_CFLAGS := $(GASNET_CFLAGS)
MY_GASNET_CC := $(GASNET_CC)
MY_GASNET_LD := $(GASNET_LD)
MY_GASNET_LDFLAGS := $(GASNET_LDFLAGS)

ifneq ($(GASNET_MAK),)
  include $(GASNET_MAK)
endif

GASNET_CFLAGS += $(CFLAGS) $(MY_GASNET_CFLAGS)

ifneq ($(MY_GASNET_CC),)
  GASNET_CC = $(MY_GASNET_CC)
endif

ifneq ($(MY_GASNET_LD),)
  GASNET_LD = $(MY_GASNET_LD)
endif

ifneq ($(MY_GASNET_LDFLAGS),)
  GASNET_LDFLAGS = $(MY_GASNET_LDFLAGS)
endif



test: test-single test-mpi test-armci test-gasnet

run: run-single run-mpi run-armci run-gasnet

%.o: %.f90
	$(FC) $(FFLAGS) -c $< -o $@

test-single: send_array_test.single get_array_test.single
test-mpi: send_array_test.mpi get_array_test.mpi
test-armci: send_array_test.armci get_array_test.armci
test-gasnet: send_array_test.gasnet get_array_test.gasnet


run-single: test-single
	./send_array_test.single
	./get_array_test.single

run-mpi: test-mpi
	$(MPI_RUN) ./send_array_test.mpi
	$(MPI_RUN) ./get_array_test.mpi

run-armci: test-armci
	$(ARMCI_RUN) ./send_array_test.armci
	$(ARMCI_RUN) ./get_array_test.armci

run-gasnet: test-gasnet
	$(GASNET_RUN) ./send_array_test.gasnet
	$(GASNET_RUN) ./get_array_test.gasnet


send_array_test.single: send_array_test.o
	$(FC) $(FFLAGS) -o $@ $< -L../single -lcaf_single
get_array_test.single: get_array_test.o
	$(FC) $(FFLAGS) -o $@ $< -L../single -lcaf_single

send_array_test.mpi: send_array_test.o
	$(MPICC) -o $@ $< -L../mpi -lcaf_mpi -lgfortran -lm
get_array_test.mpi: get_array_test.o
	$(MPICC) -o $@ $< -L../mpi -lcaf_mpi -lgfortran -lm

send_array_test.armci: send_array_test.o
	$(ARMCI_CC) -o $@ $< -L../armci -lcaf_armci $(ARMCI_LDFLAGS) -lgfortran -lm
get_array_test.armci: get_array_test.o
	$(ARMCI_CC) -o $@ $< -L../armci -lcaf_armci $(ARMCI_LDFLAGS) -lgfortran -lm

send_array_test.gasnet: send_array_test.o
	$(GASNET_LD) -o $@ $< -L../gasnet -lcaf_gasnet $(GASNET_LDFLAGS) $(GASNET_LIBS) -lgfortran -lm
get_array_test.gasnet: get_array_test.o
	$(GASNET_LD) -o $@ $< -L../gasnet -lcaf_gasnet $(GASNET_LDFLAGS) $(GASNET_LIBS) -lgfortran -lm

clean:
	rm -f *.o

distclean: clean
	rm -f *.single *.mpi *.armci *.gasnet
