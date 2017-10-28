noinst_PROGRAMS = coarray_distributed_transpose
dist_coarray_distributed_transpose_SOURCES = coarray_distributed_transpose.F90
coarray_distributed_transpose_FCFLAGS = -fcoarray=lib -lmpi
coarray_distributed_transpose_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = coarray_distributed_transpose run_size.mod
