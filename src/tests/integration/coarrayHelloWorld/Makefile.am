noinst_PROGRAMS = hello_multiverse
dist_hello_multiverse_SOURCES = hello_multiverse.F90
hello_multiverse_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
hello_multiverse_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = hello_multiverse
