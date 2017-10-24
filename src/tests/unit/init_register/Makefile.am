if COND_OPT
  MAYBE_PROG = async_comp_alloc
endif

noinst_PROGRAMS = allocate_as_barrier register_alloc_comp_2 allocate_as_barrier_proc register_alloc_comp_3 register_alloc_vector register register_vector register_alloc_comp_1 $(MAYBE_PROG)
#initialize_mpi async_comp_alloc

dist_allocate_as_barrier_SOURCES = allocate_as_barrier.f90
allocate_as_barrier_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
allocate_as_barrier_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_register_alloc_comp_1_SOURCES = register_alloc_comp_1.f90
register_alloc_comp_1_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_alloc_comp_1_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_register_alloc_comp_2_SOURCES = register_alloc_comp_2.f90
register_alloc_comp_2_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_alloc_comp_2_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_register_alloc_comp_3_SOURCES = register_alloc_comp_3.f90
register_alloc_comp_3_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_alloc_comp_3_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_allocate_as_barrier_proc_SOURCES = allocate_as_barrier_proc.f90
allocate_as_barrier_proc_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
allocate_as_barrier_proc_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

if COND_OPT
dist_async_comp_alloc_SOURCES = async_comp_alloc.f90
async_comp_alloc_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
async_comp_alloc_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o
endif

dist_register_SOURCES = register.f90
register_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_register_vector_SOURCES = register_vector.f90
register_vector_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_vector_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_register_alloc_vector_SOURCES = register_alloc_vector.f90
register_alloc_vector_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
register_alloc_vector_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

#dist_initialize_mpi_SOURCES = initialize_mpi.F90
#initialize_mpi_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi -I
#initialize_mpi_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = allocate_as_barrier register_alloc_comp_2 allocate_as_barrier_proc register_alloc_comp_3 async_comp_alloc register_alloc_vector register initialize_mpi register_vector register_alloc_comp_1
