#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "contextu.c"


int main(void)
{
	ctx_context_t r;
	int hr;
	volatile int mode = 0;

	hr = ctx_getcontext(&r);
	printf("ctx_getcontext() -> %d\n", hr);

	if (mode == 0) {
		mode++;
		printf("first run\n");
		ctx_setcontext(&r);
	}
	else {
		printf("second run\n");
	}
	printf("endup\n");

	return 0;
}


