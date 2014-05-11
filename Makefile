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

test: single armci gasnet mpi
	$(MAKE) -C testsuite test
test-mpi: mpi
	$(MAKE) -C testsuite test-mpi
test-gasnet: gasnet
	$(MAKE) -C testsuite test-gasnet
test-armci: armci
	$(MAKE) -C testsuite test-armci
test-single: single
	$(MAKE) -C testsuite test-single

run: single armci gasnet mpi
	$(MAKE) -C testsuite run
run-mpi: mpi
	$(MAKE) -C testsuite run-mpi
run-gasnet: gasnet
	$(MAKE) -C testsuite run-gasnet
run-armci: armci
	$(MAKE) -C testsuite run-armci
run-single: single
	$(MAKE) -C testsuite run-single

clean:
	$(MAKE) -k -C common clean
	$(MAKE) -k -C single clean
	$(MAKE) -k -C armci clean
	$(MAKE) -k -C gasnet clean
	$(MAKE) -k -C mpi clean
	$(MAKE) -k -C testsuite clean

distclean: clean
	$(MAKE) -k -C common distclean
	$(MAKE) -k -C single distclean
	$(MAKE) -k -C armci distclean
	$(MAKE) -k -C gasnet distclean
	$(MAKE) -k -C gasnet distclean
	$(MAKE) -k -C testsuite distclean
