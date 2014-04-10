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
	$(MAKE) -k -C single clean
	$(MAKE) -k -C armci clean
	$(MAKE) -k -C gasnet clean
	$(MAKE) -k -C mpi clean

distclean: clean
	$(MAKE) -k -C single distclean
	$(MAKE) -k -C armci distclean
	$(MAKE) -k -C gasnet distclean
	$(MAKE) -k -C mpi distclean
