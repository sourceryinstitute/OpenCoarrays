noinst_PROGRAMS = async_hello
dist_async_hello_SOURCES = async-hello.F90
async_hello_FCFLAGS = -fcoarray=lib -lmpi
async_hello_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = async_hello
