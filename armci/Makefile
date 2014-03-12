include ../make.inc

libcaf_armci.a: armci.o
	ar rcv $@ $<
	ranlib $@

armci.o: armci.c ../libcaf.h	
	$(CC) $(CFLAGS) $(ARMCI_CFLAGS) -I.. -c $< -o $@

clean:
	rm -f armci.o

distclean: clean
	rm -f libcaf_armci.a
