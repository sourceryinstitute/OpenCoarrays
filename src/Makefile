include make.inc
export

.PHONY : common
.PHONY : single
.PHONY : gasnet
.PHONY : mpi
#.PHONY : armci

mpi: common
	$(MAKE) -C $@

all: common single gasnet mpi # armci

common:
	$(MAKE) -C $@

single: common
	$(MAKE) -C $@

#armci: common
#	$(MAKE) -C $@

gasnet: common
	$(MAKE) -C $@

test: single gasnet mpi #armci
	$(MAKE) -C tests test
test-mpi: mpi
	$(MAKE) -C tests test-mpi
test-gasnet: gasnet
	$(MAKE) -C tests test-gasnet
#test-armci: armci
#	$(MAKE) -C tests test-armci
test-single: single
	$(MAKE) -C tests test-single

run: single armci gasnet mpi
	$(MAKE) -C testsuite run
run-mpi: mpi
	$(MAKE) -C testsuite run-mpi
run-gasnet: gasnet
	$(MAKE) -C testsuite run-gasnet
#run-armci: armci
#	$(MAKE) -C testsuite run-armci
run-single: single
	$(MAKE) -C testsuite run-single

clean:
#	$(MAKE) -k -C common clean
#	$(MAKE) -k -C single clean
#	$(MAKE) -k -C gasnet clean
	$(MAKE) -k -C mpi clean
#	$(MAKE) -k -C tests clean
#	$(MAKE) -k -C armci clean

distclean: clean
	$(MAKE) -k -C common distclean
#	$(MAKE) -k -C single distclean
	$(MAKE) -k -C mpi distclean
#	$(MAKE) -k -C gasnet distclean
#	$(MAKE) -k -C testsuite distclean
#	(MAKE) -k -C armci distclean
