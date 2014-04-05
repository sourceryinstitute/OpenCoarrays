include ../make.inc

libcaf_single.a: single.o ../caf_auxiliary.o
	ar rcv $@ single.o ../caf_auxiliary.o
	ranlib $@

../caf_auxiliary.o: ../caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

single.o: single.c ../libcaf.h
	$(CC) $(CFLAGS) $(SINGLE_CFLAGS) -I.. -c $< -o $@

clean:
	rm -f single.o

distclean: clean
	rm -f libcaf_single.a ../caf_auxiliary.o
