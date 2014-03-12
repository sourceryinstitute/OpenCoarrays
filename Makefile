include make.inc
export

.PHONY : armci
.PHONY : single

all: single armci

armci:
	$(MAKE) -C $@

single:
	$(MAKE) -C $@

clean:
	$(MAKE) -C armci clean
	$(MAKE) -C single clean

distclean:
	$(MAKE) -C armci distclean
	$(MAKE) -C single distclean
