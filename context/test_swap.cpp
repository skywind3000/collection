#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "contextu.c"


static char temp_stack[32 * 1024];
ctx_context_t mc, cc;

int raw_thread(void*p) {
	printf("remote: hello %s\n", (char*)p);
	ctx_swapcontext(&cc, &mc);
	printf("remote: back again\n");
	ctx_swapcontext(&cc, &mc);
	printf("remote: return\n");
	return 1024;
}

int main(void)
{
	cc.stack = temp_stack;
	cc.stack_size = sizeof(temp_stack);
	cc.link = &mc;
	ctx_getcontext(&cc);
	ctx_getcontext(&mc);
	ctx_makecontext(&cc, raw_thread, (char*)"girl");
	int hr = ctx_getcontext(&mc);
	printf("before switch: %d\n", cc.stack_size);
	ctx_swapcontext(&mc, &cc);
	printf("local: here: %d\n", hr);
	hr = ctx_swapcontext(&mc, &cc);
	printf("local: again\n");
	hr = ctx_swapcontext(&mc, &cc);
	printf("local: end\n");
	return 0;
}



