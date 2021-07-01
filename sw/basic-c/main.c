#include <assert.h>
#include <stddef.h>

extern void make_symbolic(volatile void *, size_t);

int main() {
	char buf[3] = {0, 42, 5};
	unsigned int a;

	make_symbolic(&a, sizeof(a));
	if (a < sizeof(buf)) {
		int x;

		x = buf[a];
		assert(x != 42);
		x--;
		if (x > 10) {
			if (x == 0) {
				a++;
			} else if (x == 5) {
				a = sizeof(buf) - 1;
			}

			buf[a] = 23;
			asm("EBREAK");
		}
	}

	return 0;
}
