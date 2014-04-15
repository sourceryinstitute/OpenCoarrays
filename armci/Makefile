include ../make.inc

all: libcaf_armci.a

libcaf_armci.a: armci.o ../common/caf_auxiliary.o
	ar rcv $@ armci.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(CC) $(CFLAGS) $(ARMCI_CFLAGS) -I.. -c $< -o $@

armci.o: armci.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f armci.o

distclean: clean
	rm -f libcaf_armci.a
