#ifdef ASYNC_PROGRESS
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <netdb.h>

#define BUFLEN 512

static int prog_init = 0;
/* It is 0 if all processes are on the same node */
static int multiple_nodes = 0;
static const int port = 9930;
pthread_mutex_t comm_mutex = PTHREAD_MUTEX_INITIALIZER;

static int *neigh, send_sock;
static char *str;
static char (*all_str)[255];
static MPI_Request node_req;

extern MPI_Comm CAF_COMM_WORLD;
extern int caf_this_image;
extern int caf_num_images;

void * comm_thread_routine(void *arg)
{
  struct sockaddr_in si_me, si_other;
  int i, slen=sizeof(si_me),new_port,flag;
  char buffer[BUFLEN] = "";
  int sock,received=0, tmp;
  MPI_Status s;
  
  if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    error("socket");
  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  new_port = port + caf_this_image - 1;
  si_me.sin_port = htons(new_port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    error("bind");

  prog_init = 1;

  while(multiple_nodes)
    {
      if ((received = recvfrom(sock, &buffer, BUFLEN, 0, (struct sockaddr *)&si_me, &slen))==-1)
	error("recvfrom()");
#if defined(NO_MULTIPLE)
	  pthread_mutex_lock (&comm_mutex);
#endif
      for(i=0;i<(int)(received/sizeof(int));i++)
	{
	  sscanf(buffer+i*sizeof(int),"%d",&tmp);
	  MPI_Recv(&flag,1,MPI_INT,tmp,10,CAF_COMM_WORLD,&s);
	}
#if defined(NO_MULTIPLE)
	  pthread_mutex_unlock (&comm_mutex);
#endif
    }
  return 0;
}

static int host_to_ip(char *hostname , char *ip)
{
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in *h;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ( (rv = getaddrinfo( hostname , NULL , &hints , &servinfo)) != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
    }

  for(p = servinfo; p != NULL; p = p->ai_next)
    {
      h = (struct sockaddr_in *) p->ai_addr;
      strcpy(ip , inet_ntoa( h->sin_addr ) );
    }

  freeaddrinfo(servinfo);
  return 0;
}

void check_helper_init()
{
  while(prog_init == 0 && multiple_nodes) {;}
}

void neigh_list_1st()
{
  int strl,np = caf_num_images;
  neigh = calloc(np,sizeof(int));
  str = (char *)calloc(255,1);
  MPI_Get_processor_name(str, &strl);
  all_str = calloc(np,sizeof(*all_str));
  MPI_Iallgather(str,255,MPI_CHAR,all_str,255,MPI_CHAR,CAF_COMM_WORLD,&node_req);
}

void neigh_list_2nd()
{
  MPI_Status status;
  int me = caf_this_image-1;
  char tmp[255];
  int i,np=caf_num_images;

  MPI_Wait(&node_req,&status);

  for(i=0;i<np;i++)
    if(strcmp(all_str[i],all_str[me])==0)
      neigh[i] = 1;

  for(i=0;i<np;i++)
    {
      if(neigh[i] != 1)
	{
	  strcpy(tmp,all_str[i]);
	  memset(all_str[i],0,255);
	  host_to_ip(tmp, all_str[i]);
	  multiple_nodes = 1;
	}
    }
}

void setup_send_sock()
{ 
  if ((send_sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    error("socket");
}

/* Dest is already in MPI format */
void send_sig(int dest, int img)
{
  struct sockaddr_in si_other;
  int slen=sizeof(si_other),new_port;
  char buffer[BUFLEN] = "";

  if(neigh[dest]==1 || !multiple_nodes)
    return;

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  new_port = port + dest;
  si_other.sin_port = htons(new_port);
  // Retrieve IP Address
  if (inet_aton(all_str[dest], &si_other.sin_addr)==0)
    error("inet_aton:");
  sprintf(buffer,"%d",img);
  if (sendto(send_sock, buffer, sizeof(int), 0, (struct sockaddr *)&si_other, slen)==-1)
    error("sendto()");

  return;
}

void ack_sig(int img)
{
  /* We just need to send a generic interger */
  if(multiple_nodes)
    MPI_Send(&caf_this_image,1,MPI_INT,img,10,CAF_COMM_WORLD);
}

#endif
