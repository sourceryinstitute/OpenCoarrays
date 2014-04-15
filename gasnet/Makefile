include ../make.inc

MY_GASNET_CFLAGS := $(GASNET_CFLAGS)
MY_GASNET_CC := $(GASNET_CC)
MY_GASNET_LD := $(GASNET_LD)
MY_GASNET_LDFLAGS := $(GASNET_LDFLAGS)

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

libcaf_gasnet.a: gasnet.o ../common/caf_auxiliary.o
	ar rcv $@ gasnet.o ../common/caf_auxiliary.o
	ranlib $@

.c.o:
	$(GASNET_CC) $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) -I.. -c -o $@ $<

gasnet.o: gasnet.c ../libcaf.h ../libcaf-gfortran-descriptor.h

../common/caf_auxiliary.o:
	$(MAKE) -C ../common

clean:
	rm -f gasnet.o

distclean: clean
	rm -f libcaf_armci.a
