include mpi-seq.mak
#include ibv-seq.mak
#include smp-seq.mak

CCINCL=./caf
CCOPT=-std=gnu99 -DPREFIX_NAME=__gfortran_caf_

.c.o:
	$(GASNET_CC) $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) -I$(CCINCL) $(CCOPT) -g -c -o $@ $<	

hello: hello.o gasnet.o
	$(GASNET_LD) $(GASNET_LDFLAGS) -I$(CCINCL) gasnet.o -o $@ $< $(GASNET_LIBS)
