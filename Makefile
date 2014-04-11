include make.inc
export

.PHONY : common
.PHONY : single
.PHONY : armci
.PHONY : gasnet
.PHONY : mpi

all: common single armci gasnet mpi

common:
	$(MAKE) -C $@

single: common
	$(MAKE) -C $@

armci: common
	$(MAKE) -C $@

gasnet: common
	$(MAKE) -C $@

mpi: common
	$(MAKE) -C $@

clean:
	$(MAKE) -k -C common clean
	$(MAKE) -k -C single clean
	$(MAKE) -k -C armci clean
	$(MAKE) -k -C gasnet clean
	$(MAKE) -k -C mpi clean

distclean: clean
	$(MAKE) -k -C common distclean
	$(MAKE) -k -C single distclean
	$(MAKE) -k -C armci distclean
	$(MAKE) -k -C gasnet distclean
	$(MAKE) -k -C mpi distclean
