.SUFFIXES: .f90 .armci .mpi .gasnet

TOP=../../..
include $(TOP)/make.inc

OBJS=increment_my_neighbor.o add_myself_to_my_neighbor.o send_array_test.o
EXES=$(OBJS:.o=.exe)


all: $(OBJS) armci gasnet mpi


clean:
	/bin/rm -fr *.o *.armci *.mpi *.gasnet


mpi: $(EXES:.exe=.mpi)

armci: $(EXES:.exe=.armci)

gasnet: $(EXES:.exe=.gasnet)


.o.mpi:
	$(MPFC) -o $@ $< -lcaf_mpi -L$(TOP)/mpi
.o.armci:
	$(MPFC)  -o $@ $< -lcaf_armci -L$(TOP)/armci $(ARMCI_LDFLAGS)
.o.gasnet:
	$(MPFC)  -o $@ $< -lcaf_gasnet -L$(TOP)/gasnet $(GASNET_LDFLAGS)
.f90.o:
	$(FC) -fcoarray=lib $(FFLAGS) -c $< -o $@
