#include <assert.h>
#include <stddef.h>

extern void make_symbolic(volatile void *, size_t);

int main() {
	char buf[3];
	unsigned int a;
	make_symbolic(&a, sizeof(a));

	if (a < 3) {
		a++;
	} else {
		a--;
	}

	return 0;
}
