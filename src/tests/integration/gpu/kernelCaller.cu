#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>

extern "C" void cudaPrint(int *data, int n, int me, char *name);
extern "C" void cudaAdd(int* data_in1, int* data_in2, int *data_out, int n);
extern "C" void cudaDot(float* in1, float* in2, float* out, int n);

#define MAX_BLOCK_SZ 512

__global__ void printOnCuda(int *data, int n)
{
  int i = 0;
  int me = blockIdx.x*blockDim.x + threadIdx.x;
  if (me == 0) 
    for(i=0;i<n;i++)
      printf("From CUDA data[%d] = %d\n",i,data[i]);
}

__global__ void assignOnCuda(int *data, int n)
{
  int i = 0;
  int me = blockIdx.x*blockDim.x + threadIdx.x;
  if (me == 0)
    for(i=0;i<n;i++)
      data[i] = me + i;
//      printf("From CUDA data[%d] = %d\n",i,data[i]);
}

__global__ void Dev_dot(float x[], float y[], float z[], int n) {
   /* Use tmp to store products of vector components in each block */
   /* Can't use variable dimension here                            */
   __shared__ float tmp[MAX_BLOCK_SZ];
   int t = blockDim.x * blockIdx.x + threadIdx.x;
   int loc_t = threadIdx.x;
   
   if (t < n) tmp[loc_t] = x[t]*y[t];
   __syncthreads();

   /* This uses a tree structure to do the addtions */
   for (int stride = blockDim.x/2; stride >  0; stride /= 2) {
      if (loc_t < stride)
         tmp[loc_t] += tmp[loc_t + stride];
      __syncthreads();
   }

   /* Store the result from this cache block in z[blockIdx.x] */
   if (threadIdx.x == 0) {
      z[blockIdx.x] = tmp[0];
   }
}  /* Dev_dot */ 

__global__ void add_cuda_int(int* data_in1, int* data_in2, int *data_out, int n)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;

  if(i < n)
    data_out[i] = data_in1[i] + data_in2[i];

  __syncthreads();	
}

void cudaAssign(int *data, int n, char *name)
{
  printf("Processor %s writes data on Cuda\n",name);
  assignOnCuda<<<1,32>>>(data,n);
}
extern "C"
{
void cudaDot(float *in1, float *in2, float *out, int n)
{
  float *partial_dot;
  int nThreads = 64, i=0;
  int nBlocks = ((n-1)/nThreads)+1;

  cudaMallocManaged(&partial_dot, nBlocks * sizeof(float));

  *out = 0.0;

  Dev_dot<<<nBlocks,nThreads>>>(in1,in2,partial_dot,n);
  cudaDeviceSynchronize();

  for(i=0;i<nBlocks;i++)
    *out += partial_dot[i];

  cudaFree(partial_dot);

}

void manual_mapped_cudaDot(float* in1, float* in2, float *out, int n, int img)
{
  float *partial_dot;
  float *in1_d, *in2_d;
  int nThreads = 64, i=0;
  int nBlocks = ((n-1)/nThreads)+1;
  int count = 0;

  cudaGetDeviceCount(&count);
  cudaSetDevice(img%count);

  cudaMallocManaged(&partial_dot, nBlocks * sizeof(float));
  *out = 0.0;

  cudaHostRegister(in1,n*sizeof(float),cudaHostRegisterMapped);
  cudaHostRegister(in2,n*sizeof(float),cudaHostRegisterMapped);

  cudaHostGetDevicePointer(&in1_d,in1,0);
  cudaHostGetDevicePointer(&in2_d,in2,0); 	

  Dev_dot<<<nBlocks,nThreads>>>(in1_d,in2_d,partial_dot,n);
  cudaDeviceSynchronize();

  for(i=0;i<nBlocks;i++)
    *out += partial_dot[i];

  cudaFree(partial_dot);
}


void manual_cudaDot(float* in1, float* in2, float *out, int n, int img)
{
  float *partial_dot;
  float *in1_d, *in2_d;
  int nThreads = 64, i=0;
  int nBlocks = ((n-1)/nThreads)+1;
  int count = 0;

  cudaGetDeviceCount(&count);
  cudaSetDevice(img%count);

  cudaMallocManaged(&partial_dot, nBlocks * sizeof(float));
  *out = 0.0;

  cudaMalloc(&in1_d,sizeof(float)*n);
  cudaMalloc(&in2_d,sizeof(float)*n);

  cudaMemcpy(in1_d,in1,n*sizeof(float),cudaMemcpyHostToDevice);
  cudaMemcpy(in2_d,in2,n*sizeof(float),cudaMemcpyHostToDevice);

  Dev_dot<<<nBlocks,nThreads>>>(in1_d,in2_d,partial_dot,n);
  cudaDeviceSynchronize();

  for(i=0;i<nBlocks;i++)
    *out += partial_dot[i];

  cudaFree(partial_dot);
  cudaFree(in1_d);
  cudaFree(in2_d);
}

void cudaAdd(int *data_in1, int *data_in2, int *data_out, int n)
{
  int nThreads = 64;
  int nBlocks = ((n-1)/nThreads)+1;
//  printf("n: %d\n",n);
//  cudaDeviceSynchronize();
  add_cuda_int<<<nBlocks,nThreads>>>(data_in1,data_in2,data_out,n);
  cudaDeviceSynchronize();
}
void cudaPrint(int *data, int n, int me, char *name)
{
  printf("Image %d on processor %s reads data on Cuda\n",me,name);
  printOnCuda<<<1,32>>>(data,n);
}
}
