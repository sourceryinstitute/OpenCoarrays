include make.inc
export

.PHONY : single
.PHONY : armci
.PHONY : gasnet
.PHONY : mpi

all: single armci gasnet mpi

single: caf_auxiliary.o
	$(MAKE) -C $@

armci: caf_auxiliary.o
	$(MAKE) -C $@

gasnet: caf_auxiliary.o
	$(MAKE) -C $@

mpi: caf_auxiliary.o
	$(MAKE) -C $@

caf_auxiliary.o: caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f  caf_auxiliary.o
	$(MAKE) -C single clean
	$(MAKE) -C armci clean
	$(MAKE) -C gasnet clean
	$(MAKE) -C mpi clean

distclean: clean
	$(MAKE) -C single distclean
	$(MAKE) -C armci distclean
	$(MAKE) -C gasnet distclean
	$(MAKE) -C mpi distclean
