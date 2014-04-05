include ../make.inc

libcaf_armci.a: armci.o ../caf_auxiliary.o
	ar rcv $@ armci.o ../caf_auxiliary.o
	ranlib $@

../caf_auxiliary.o: ../caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

armci.o: armci.c ../libcaf.h	
	$(CC) $(CFLAGS) $(ARMCI_CFLAGS) -I.. -c $< -o $@

clean:
	rm -f armci.o

distclean: clean
	rm -f libcaf_armci.a ../caf_auxiliary.o
