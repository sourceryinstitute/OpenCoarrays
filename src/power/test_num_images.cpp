// Test retrieving number of images

#include <cassert>
#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);    // initialize image

   int images = caf_num_images(); // get number of images
   assert(images > 0);

   caf_finalize();            // finalize image

   return 0;
}
