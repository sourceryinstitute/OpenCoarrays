// Test image initialization and finalization.

#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);  // initialize image

   caf_finalize(); // finalize image

   return 0;
}
