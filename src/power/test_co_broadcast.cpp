// Test co broadcast

#include <cassert>
#include <iostream>
using std::cout;

#include "libcaf.h"
#include <mpi.h>
#include <ISO_Fortran_binding.h>
using Fortran::ISO::Fortran_2018::CFI_cdesc_t;

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);        // initialize image

   int num = caf_num_images();    // get number of images
   assert(num > 0);

   int img = caf_this_image();    // get this image
   assert(img > 0 && img <= num); // assert this image is within range of number of images

   // declare array for use in collective call
   const size_t LENGTH = 5;
   int *arr = new int[LENGTH];

   // manually fill out descriptor (for now)
   CFI_cdesc_t arr_desc;
   arr_desc.base_addr = arr;
   arr_desc.type = CFI_type_int;
   arr_desc.rank = 1;
   arr_desc.dim[0].extent = LENGTH;
   arr_desc.dim[0].lower_bound = 1; // or 0? for c indexing?
   arr_desc.dim[0].sm = sizeof(int);

   // fill out array
   for (int i = 0; i < LENGTH; ++i) {
      arr[i] = (img + 1) * 10 + i;
   }

   // necessary for caf_co_broadcast() call
   int stat;
   size_t errmsg_len = 20;
   char *errmsg = new char[errmsg_len];
   int source_image = 1;          // image which should broadcast values to other images

   // initalize array with expected values after broadcast is done
   int init_val = (source_image + 1) * 10;
   int *expected{ new int[LENGTH]{init_val++, init_val++, init_val++, init_val++, init_val} };

   caf_co_broadcast(&arr_desc, source_image, &stat, errmsg, errmsg_len);

   // check result of caf_co_broadcast() call
   assert(arraysAreEquiv<int>(arr, expected, LENGTH));

   caf_finalize();                // finalize image

   // clean up memory
   delete[] arr;
   delete[] expected;
   delete[] errmsg;

   if (img == source_image) {
      cout << "Test passed\n";
   }

   return 0;
}
