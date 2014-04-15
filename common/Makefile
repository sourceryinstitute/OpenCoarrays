include ../make.inc

caf_auxiliary.o: caf_auxiliary.c ../libcaf.h ../libcaf-gfortran-descriptor.h
	$(CC) -I.. $(CFLAGS) -c $< -o $@

clean:
	rm -f caf_auxiliary.o

distclean: clean
