include ../make.inc

MY_GASNET_CFLAGS := $(GASNET_CFLAGS)
MY_GASNET_CC := $(GASNET_CC)
MY_GASNET_LD := $(GASNET_LD)
MY_GASNET_LDFLAGS := $(GASNET_LDFLAGS)

GASNET_MAK ?= mpi-seq.mak
# GASNET_MAK ?= ibv-seq.mak
# GASNET_MAK ?= smp-seq.mak

include $(GASNET_MAK)


GASNET_CFLAGS += $(CFLAGS) $(MY_GASNET_CFLAGS)

ifneq ($(MY_GASNET_CC),)
  GASNET_CC = $(MY_GASNET_CC)
endif

ifneq ($(MY_GASNET_LD),)
  GASNET_LD = $(MY_GASNET_LD)
endif

ifneq ($(MY_GASNET_LDFLAGS),)
  GASNET_LDFLAGS = $(MY_GASNET_LDFLAGS)
endif


all: libcaf_gasnet.a

.c.o:
	$(GASNET_CC) $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) -I.. -c -o $@ $<

hello: hello.o gasnet.o
	$(GASNET_LD) $(GASNET_LDFLAGS) -o $@ $< -L. -lcaf_gasnet $(GASNET_LIBS)

libcaf_gasnet.a: gasnet.o ../caf_auxiliary.o
	ar rcv $@ gasnet.o ../caf_auxiliary.o
	ranlib $@

../caf_auxiliary.o: ../caf_auxiliary.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f gasnet.o hello.o

distclean: clean
	rm -f libcaf_armci.a hello ../caf_auxiliary.o
