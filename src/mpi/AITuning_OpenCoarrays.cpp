#include "mpi.h"
#include <string>
#include <iostream>
#include <time.h>
#include "Probes.h"
#include "Variables.h"
#include "Controller.h"

using namespace std;

double start_time, start_get, start_put, start_collective, start_barrier;
static int n_procs, my_id;

static SingleProbe *total_time_p;

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  int provided_t, err = -1;
  bool async_on = false;

  AITuning_start("MPICH");
  UserDefinedPerformanceVar *total_time_v = new UserDefinedPerformanceVar((char*)"total_time",(char*)"total_time_log.txt");
  AITuning_addUserDefinedPerformanceVar(total_time_v);
  total_time_p = new SingleProbe((char*)"total_time_probe", total_time_v);

  err = PMPI_Init_thread(argc, argv, required, provided);

  MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  
  start_time = MPI_Wtime();
  
  return err;
}

int MPI_Finalize(void){

  int err = -1;
  double end_time,average,total_time,elapsed_time;

  end_time = MPI_Wtime();

  elapsed_time = end_time - start_time;

  MPI_Reduce(&elapsed_time, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  total_time = total_time/n_procs;

  printf("Registring value in probe\n");
  total_time_p->registerValue(total_time);

  err = PMPI_Finalize();
  
  return err;
}
