CC = gcc
CFLAGS = -Wall

all: ISO_Fortran_binding_tests

ISO_Fortran_binding_tests: ISO_Fortran_binding.o ISO_Fortran_binding_tests.o
	$(CC) ISO_Fortran_binding.o ISO_Fortran_binding_tests.o -o ISO_Fortran_binding_tests

ISO_Fortran_binding.o: ../../../iso-fortran-binding/ISO_Fortran_binding.c
	$(CC) $(CFLAGS) -c ../../../iso-fortran-binding/ISO_Fortran_binding.c

ISO_Fortran_binding_tests.o: ISO_Fortran_binding_tests.c
	$(CC) $(CFLAGS) -c ISO_Fortran_binding_tests.c

clean:
	rm *.o ISO_Fortran_binding_tests
