手痒昨天手痒写了个有栈的 coroutine ，接口反照 ucontext 的接口，不必无栈的复杂多少：

```cpp
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
```

使用 ctx_getcontext / ctx_setcontext 可以实现保存现场，恢复现场的功能，该程序输出：

    ctx_getcontext() -0
    first run
    ctx_getcontext() -6356604
    second run
    endup

继续使用 ctx_makecontext / ctx_swapcontext 可以实现初始化协程和切换上下文的功能：

```cpp
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
```

这里创建了一个协程，接着主程序和协程互相切换，程序输出：

    before switch: 32768
    remote: hello girl
    local: here
    remote: back again
    local: again
    remote: return
    local: end

你再写两行，就可以包装出一个 yield 了，具体可以参考云风的 coroutine 的几百行代码。

