include make.inc
export

.PHONY : single
.PHONY : armci
.PHONY : gasnet

all: single armci gasnet

single: caf_auxiliary.o
	$(MAKE) -C $@

armci: caf_auxiliary.o
	$(MAKE) -C $@

gasnet: caf_auxiliary.o
	$(MAKE) -C $@

caf_auxiliary.o: caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f  caf_auxiliary.o
	$(MAKE) -C single clean
	$(MAKE) -C armci clean
	$(MAKE) -C gasnet clean

distclean: clean
	$(MAKE) -C single distclean
	$(MAKE) -C armci distclean
	$(MAKE) -C gasnet distclean
