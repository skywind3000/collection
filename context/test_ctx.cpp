#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "contextu.h"


void test1(void)
{
	ctx_context_t r;
	int hr;
	volatile int mode = 0;
	memset(&r, 0, sizeof(r));

	printf("size=%d\n", (int)sizeof(r));
#if 0
	hr = ctx_regs_get(&r.regs);
#else
	hr = ctx_getcontext(&r);
#endif

	printf("ctx_regs_get() -> %d\n", hr);

	if (mode == 0) {
		mode++;
		printf("first run\n");
	#if CTX_ARCH_X86
		r.regs.eax = 1024;
	#else
		/* r.regs.rax = 1025; */
	#endif
		ctx_setcontext(&r);
	}
	else {
		printf("second run\n");
	}
	printf("endup\n");
}

static char temp_stack[8192 * 1024];
ctx_context_t mc, cc;

int raw_thread(void*p) {
	printf("remote: hello %s\n", (char*)p);
	ctx_swapcontext(&cc, &mc);
	printf("remote: back again\n");
	ctx_swapcontext(&cc, &mc);
	printf("remote: return\n");
	return 1024;
}

void test2(void)
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
}

#ifdef __CYGWIN__
#endif

int main(void)
{
	test1();
	return 0;
}



