noinst_PROGRAMS = syncall syncimages syncimages2 duplicate_syncimages syncimages_status sync_image_ring_abort_on_stopped_image

dist_syncall_SOURCES = syncall.f90
syncall_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
syncall_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

dist_syncimages_SOURCES = syncimages.f90
syncimages_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
syncimages_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

dist_syncimages2_SOURCES = syncimages2.f90
syncimages2_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
syncimages2_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

dist_duplicate_syncimages_SOURCES = duplicate_syncimages.f90
duplicate_syncimages_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
duplicate_syncimages_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

dist_syncimages_status_SOURCES = syncimages_status.f90
syncimages_status_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
syncimages_status_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

dist_sync_image_ring_abort_on_stopped_image_SOURCES = sync_image_ring_abort_on_stopped_image.f90
sync_image_ring_abort_on_stopped_image_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
sync_image_ring_abort_on_stopped_image_LDADD=$(top_builddir)/src/mpi_caf.o $(top_builddir)/src/caf_auxiliary.o

CLEANFILES = syncall syncimages syncimages2 duplicate_syncimages syncimages_status sync_image_ring_abort_on_stopped_image
