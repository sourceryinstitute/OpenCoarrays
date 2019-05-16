#include "libcaf.h"

int main(int argc, char* argv[])
{
   caf_init(&argc, &argv);

   caf_finalize();

   return 0;
}
