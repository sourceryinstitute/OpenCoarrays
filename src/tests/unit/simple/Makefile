.SUFFIXES: .f90 .mpi .gasnet

TOP=../../..
include $(TOP)/make.inc

OBJS=increment_my_neighbor.o add_myself_to_my_neighbor.o send_array_test.o
EXES=$(OBJS:.o=.exe)


all: $(OBJS) gasnet mpi


clean:
	/bin/rm -fr *.o *.mpi *.gasnet


mpi: $(EXES:.exe=.mpi)

gasnet: $(EXES:.exe=.gasnet)


.o.mpi:
	$(MPFC) -o $@ $< -lcaf_mpi -L$(TOP)/mpi
.o.gasnet:
	$(MPFC)  -o $@ $< -lcaf_gasnet -L$(TOP)/gasnet $(GASNET_LDFLAGS)
.f90.o:
	$(FC) -fcoarray=lib $(FFLAGS) -c $< -o $@
