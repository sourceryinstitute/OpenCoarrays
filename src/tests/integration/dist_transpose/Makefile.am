noinst_PROGRAMS = coarray_distributed_transpose
dist_coarray_distributed_transpose_SOURCES = coarray_distributed_transpose.F90
coarray_distributed_transpose_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
coarray_distributed_transpose_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

CLEANFILES = coarray_distributed_transpose run_size.mod
