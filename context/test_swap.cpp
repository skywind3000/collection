#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "contextu.c"


char temp_stack[32768];
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
	ctx_makecontext(&cc, raw_thread, (char*)"girl");

	printf("before switch: %d\n", cc.stack_size);
	ctx_swapcontext(&mc, &cc);

	printf("local: here\n");
	ctx_swapcontext(&mc, &cc);

	printf("local: again\n");
	ctx_swapcontext(&mc, &cc);

	printf("local: end\n");
	return 0;
}



