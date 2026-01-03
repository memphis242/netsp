#include <stdio.h>

int main(void)
{
   int * ptr = nullptr;
   printf("Core Dump trigger via SIGSEGV\n"
          "-----------------------------\n");
   printf("Write to address: ");
   scanf("%lu", &ptr);
   *ptr = 1;

   return 0;
}
