noinst_PROGRAMS = coarrayHeatSimplified
dist_coarrayHeatSimplified_SOURCES = $(srcdir)/local_field.f90 $(srcdir)/global_field.f90 $(srcdir)/main.f90
coarrayHeatSimplified_FCFLAGS = -fcoarray=lib -lmpi
coarrayHeatSimplified_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = coarrayHeatSimplified
