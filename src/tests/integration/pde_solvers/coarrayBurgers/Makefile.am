SUBDIRS = include-files

noinst_PROGRAMS = coarrayBurgers

dist_coarrayBurgers_SOURCES = \
$(srcdir)/library/object_interface.F90 \
$(srcdir)/library/co_object_interface.F90 \
$(srcdir)/library/ForTrilinos_assertion_utility.F90 \
$(srcdir)/local_field.F90 \
$(srcdir)/global_field.F90 \
$(srcdir)/main.F90

coarrayBurgers_FCFLAGS = -fcoarray=lib -L$(MPI_LIB) -lmpi

coarrayBurgers_LDADD = \
$(top_builddir)/src/mpi/mpi_caf.o \
$(top_builddir)/src/common/caf_auxiliary.o

CLEANFILES = \
coarrayBurgers\
co_object_interface.mod \
fortrilinos_assertion_utility.mod \
global_field_module.mod \
local_field_module.mod \
object_interface.mod \
compiler_capabilities.txt
