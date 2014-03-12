include ../make.inc

libcaf_single.a: single.o
	ar rcv $@ $<
	ranlib $@

single.o: single.c ../libcaf.h	
	$(CC) $(CFLAGS) $(SINGLE_CFLAGS) -I.. -c $< -o $@

clean:
	rm -f single.o

distclean: clean
	rm -f libcaf_single.a
