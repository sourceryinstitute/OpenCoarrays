include mpi-seq.mak
#include ibv-seq.mak
#include smp-seq.mak

include ../make.inc

CCINCL=./caf
CCOPT=-std=gnu99

all: libcaf_gasnet.a hello

.c.o:
	$(GASNET_CC) $(CFLAGS) $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) -I$(CCINCL) $(CCOPT) -c -o $@ $<

hello: hello.o gasnet.o
	$(GASNET_LD) $(GASNET_LDFLAGS) -I$(CCINCL) gasnet.o -o $@ $< $(GASNET_LIBS)

libcaf_gasnet.a: gasnet.o ../caf_auxiliary.o
	ar rcv $@ gasnet.o ../caf_auxiliary.o
	ranlib $@

../caf_auxiliary.o: ../caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f gasnet.o hello.o

distclean: clean
	rm -f libcaf_armci.a hello ../caf_auxiliary.o
