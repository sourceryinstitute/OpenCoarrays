noinst_PROGRAMS = increment_my_neighbor atomics

dist_increment_my_neighbor_SOURCES = increment_neighbor.f90
increment_my_neighbor_FCFLAGS = -fcoarray=lib -lmpi
increment_my_neighbor_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_atomics_SOURCES = testAtomics.f90
atomics_FCFLAGS = -fcoarray=lib -lmpi
atomics_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = increment_my_neighbor atomics
