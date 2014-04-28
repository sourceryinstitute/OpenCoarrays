#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>

static struct timezone tz;
static struct timeval start_time, finish_time;

/* Start measuring a time delay */
void start_timer(void)
{
    gettimeofday( &start_time, &tz);
}

/* Retunrn elapsed time in microseconds */
int elapsed_time(void)
{
    gettimeofday( &finish_time, &tz);
    return(1000000.0*(finish_time.tv_sec - start_time.tv_sec) +
           (finish_time.tv_usec - start_time.tv_usec) );
}

/* Return the stopping time in microseconds */
double stop_timer(void)
{
    gettimeofday( &finish_time, &tz);
    return(1000000.0*finish_time.tv_sec + finish_time.tv_usec);
}
