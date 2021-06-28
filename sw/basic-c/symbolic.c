#include <stddef.h>

void
make_symbolic(volatile void *ptr, size_t size)
{
	__asm__ volatile ("li a7, 96\n"
			"mv a0, %0\n"
			"mv a1, %1\n"
			"ecall\n"
			: /* no output operands */
			: "r" (ptr), "r" (size)
			: "a7", "a0", "a1");
}
