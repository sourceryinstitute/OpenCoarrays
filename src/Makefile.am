AM_CPPFLAGS=$(MPI_INCLUDE)
ACLOCAL_AMFLAGS='-I m4'
lib_LTLIBRARIES = libcaf_mpi.la
libcaf_mpi_la_SOURCES = mpi/mpi_caf.c common/caf_auxiliary.c
libcaf_mpi_la_LDFLAGS = -version-info 3:0:0
libcaf_mpi_la_CCFLAGS = -fPIC -O2
include_HEADERS = libcaf.h libcaf-gfortran-descriptor.h
bin_SCRIPTS = caf cafrun

caf: Makefile
	echo $(MPI_INCLUDE)
	cp ./extensions/caf ./caf
	chmod +x ./caf

cafrun: Makefile
	cp ./extensions/cafrun ./cafrun
	chmod +x ./cafrun

CLEANFILES = caf cafrun
