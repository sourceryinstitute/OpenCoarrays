// Test retrieving number of images

#include "libcaf.h"
#include <iostream>

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);   // initialize image

   std::cout << "num_images() is " << num_images() << std::endl;

   caf_finalize();           // finalize image

   return 0;
}
