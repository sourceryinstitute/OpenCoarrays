CC = mpic++ -std=c++11

CafInitFinalize: caf_init_finalize.o
        $(CC) -o CafInitFinalize caf_init_finalize.o

caf_init_finalize.o: caf_init_finalize.cpp
        $(CC) -c caf_init_finalize.cpp

check:
        make
        ./CafInitFinalize

clean:
        rm -f *.o
        rm CafInitFinalize
