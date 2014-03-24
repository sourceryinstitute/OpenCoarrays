#include <stdlib.h>
#include <stdio.h>
#include "libcaf.h"
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdbool.h>


static struct timezone tz;
static struct timeval start_time, finish_time;

/* Start measuring a time delay */
void start_timer(void)
{
    gettimeofday( &start_time, &tz);
}

/* Retunrn elapsed time in milliseconds */
double elapsed_time(void)
{
    gettimeofday( &finish_time, &tz);
    return(1000.0*(finish_time.tv_sec - start_time.tv_sec) +
           (finish_time.tv_usec - start_time.tv_usec)/1000.0 );
}

/* Return the stopping time in milliseconds */
double stop_time(void)
{
    gettimeofday( &finish_time, &tz);
    return(1000.0*finish_time.tv_sec + finish_time.tv_usec/1000.0);
}

int main(int argc, char **argv)
{
  int info = 0, me,np,n=1,i, *images;
  double *a_d,*d;
  caf_token_t token;
  ptrdiff_t size=sizeof(double);
  char errmsg[255];
  bool check = true;

  /* if(argc == 1) */
  /*   { */
  /*     printf("Please insert message size\n"); */
  /*     return 1; */
  /*   } */
  
  /* sscanf(argv[1],"%d",&n); */
  
  /* n = (int)n/sizeof(double); */

  /* size = n*sizeof(double); */

  _gfortran_caf_init (&argc, &argv);

  me = _gfortran_caf_this_image (1);
  np = _gfortran_caf_num_images (1, 1);

  a_d = _gfortran_caf_register(size,CAF_REGTYPE_COARRAY_STATIC,&token,&info,errmsg,255);
  
  /* start_timer(); */
  images = calloc(np,sizeof(int));

  if(me==1)
    {
      images[0] = -1;
      _gfortran_caf_sync_images(1,images,&info,errmsg,255);
    }
  else
    {
      images[0] = 1;
      _gfortran_caf_sync_images(1,images,&info,errmsg,255);

    }

  printf("proc %d dval: %lf\n",me);

  /* stop_time(); */

  if(info!=0)
    printf("Error\n");

  _gfortran_caf_finalize();

  return 0;
}
