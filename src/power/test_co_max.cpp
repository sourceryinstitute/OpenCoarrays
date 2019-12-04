// Test co max

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

   // declare array for use in collective call and array to compare against for expected results
   const size_t LENGTH = 5;
   int *arr = new int[LENGTH];
   int init_val = (num + 1) * 10;
   int *expected{ new int[LENGTH]{init_val++, init_val++, init_val++, init_val++, init_val} };

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
   
   // necessary for caf_co_min() call
   int stat;
   size_t errmsg_len = 20;
   char *errmsg = new char[errmsg_len];
   int result_image = 1;          // image where result should be stored

   caf_co_max(&arr_desc, result_image, &stat, errmsg, errmsg_len);

   // check result
   if (img == result_image) {
      //      assert(arraysAreEquiv<int>(arr, expected, LENGTH));
      if (arraysAreEquiv<int>(arr, expected, LENGTH)) {
         cout << "Test passed\n";
      }
      else {
         cout << "Test failed\n";
      }
   }

   caf_finalize();                // finalize image

   // clean up memory
   delete[] arr;
   delete[] expected;
   delete[] errmsg;

   return 0;
}
