// Test error stop functionality.

#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);   // initialize image

   // arbitrarily initializing values for test purposes
   int error = 1;
   bool quiet = false;

   error_stop(error, quiet);

   caf_finalize();           // finalize image

   return 0;
}
