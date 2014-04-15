include ../make.inc

libcaf_single.a: single.o ../common/caf_auxiliary.o
	ar rcv $@ single.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(CC) $(CFLAGS) $(SINGLE_CFLAGS) -I.. -c $< -o $@

single.o: single.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f single.o

distclean: clean
	rm -f libcaf_single.a
