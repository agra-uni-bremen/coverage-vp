#include <assert.h>
#include <stddef.h>

extern void make_symbolic(volatile void *, size_t);

int main() {
	char buf[3] = {0, 42, 5};
	unsigned int a;

	make_symbolic(&a, sizeof(a));
	if (a < sizeof(buf)) {
		if (buf[a] > 10) {
			buf[a] = 23;
			asm("EBREAK");
		}
	}

	return 0;
}
