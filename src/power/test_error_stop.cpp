// Test error stop functionality.

#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);   // initialize image

   // arbitrarily initializing values for test purposes
   int error_code = 1;
   bool quiet = false;

   error_stop(error_code, quiet);

   return error_code;
}
