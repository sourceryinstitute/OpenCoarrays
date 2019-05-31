// Test syncing all the images

#include <cassert>
#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);        // initialize image

   int num = num_images();        // get number of images
   assert(num > 0);

   int img = this_image();        // get this image
   assert(img > 0 && img <= num); // assert this image is within range of number of images

   sync_all();                    // sync images

   caf_finalize();                // finalize image

   return 0;
}
