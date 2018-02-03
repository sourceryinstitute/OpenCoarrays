SUBDIRS = include-files

noinst_PROGRAMS = coarrayBurgers

dist_coarrayBurgers_SOURCES = object_interface.F90 co_object_interface.F90  ForTrilinos_assertion_utility.F90 local_field.F90 global_field.F90 main.F90

coarrayBurgers_FCFLAGS = -fcoarray=lib -lmpi

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
