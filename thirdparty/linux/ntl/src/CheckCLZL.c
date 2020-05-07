#include <NTL/mach_desc.h>

void touch(unsigned long& x);

long CountLeadingZeros(unsigned long x)
{
   return __builtin_clzl(x);
}

int main()
{
   unsigned long x = 3;
   touch(x);
   if (CountLeadingZeros(x) == NTL_BITS_PER_LONG-2)
      return 0;
   else
      return -1;
}



