noinst_PROGRAMS = async_hello
dist_async_hello_SOURCES = async-hello.F90
async_hello_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
async_hello_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

CLEANFILES = async_hello
