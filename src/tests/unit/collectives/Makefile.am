noinst_PROGRAMS = co_min co_max co_sum co_reduce co_broadcast co_reduce_string co_reduce_res_im

dist_co_min_SOURCES = co_min.F90
co_min_FCFLAGS = -fcoarray=lib -lmpi
co_min_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_max_SOURCES = co_max.F90
co_max_FCFLAGS = -fcoarray=lib -lmpi
co_max_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_sum_SOURCES = co_sum.F90
co_sum_FCFLAGS = -fcoarray=lib -lmpi
co_sum_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_reduce_SOURCES = co_reduce.F90
co_reduce_FCFLAGS = -fcoarray=lib -lmpi
co_reduce_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_broadcast_SOURCES = co_broadcast.F90
co_broadcast_FCFLAGS = -fcoarray=lib -lmpi
co_broadcast_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_reduce_string_SOURCES = co_reduce_string.f90
co_reduce_string_FCFLAGS = -fcoarray=lib -lmpi
co_reduce_string_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_co_reduce_res_im_SOURCES = co_reduce_res_im.f90
co_reduce_res_im_FCFLAGS = -fcoarray=lib -lmpi
co_reduce_res_im_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = co_min co_max co_sum co_reduce co_broadcast co_reduce_string co_reduce_res_im co_intrinsics_module.mod
