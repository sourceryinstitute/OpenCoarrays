noinst_PROGRAMS = get_array_test get_with_offset_1d sameloc send_array_test strided_get strided_sendget whole_get_array

dist_get_array_test_SOURCES = get_array_test.f90
get_array_test_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
get_array_test_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_get_with_offset_1d_SOURCES = get_with_offset_1d.f90
get_with_offset_1d_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
get_with_offset_1d_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_sameloc_SOURCES = sameloc.f90
sameloc_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
sameloc_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_send_array_test_SOURCES = send_array_test.f90
send_array_test_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
send_array_test_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_strided_get_SOURCES = strided_get.f90
strided_get_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
strided_get_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_strided_sendget_SOURCES = strided_sendget.f90
strided_sendget_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
strided_sendget_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

dist_whole_get_array_SOURCES = whole_get_array.f90
whole_get_array_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi
whole_get_array_LDADD=$(top_builddir)/src/mpi/mpi_caf.o $(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = get_array_test get_with_offset_1d old_get_array_test sameloc send_array_test strided_get strided_sendget whole_get_array

