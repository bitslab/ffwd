#include <stdio.h>
 
void cpuid(unsigned info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
  *eax = info;
  __asm volatile
    ("mov %%ebx, %%edi;" /* 32bit PIC: don't clobber ebx */
     "cpuid;"
     "mov %%ebx, %%esi;"
     "mov %%edi, %%ebx;"
     :"+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
     : :"edi");
}
 
int main()
{
  unsigned int eax, ebx, ecx, edx;
  int i;

	printf("              %%eax       %%ebx       %%ecx       %%edx\n");
  for (i = 0; i < 6; ++i)
		{
			cpuid(i, &eax, &ebx, &ecx, &edx);
			printf("%-8i: %#010x %#010x %#010x %#010x\n", i, eax, ebx, ecx, edx);
		}
 
  return 0;
}
