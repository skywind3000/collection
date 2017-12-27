#include "contextu.h"


#if CTX_ARCH_X86
//---------------------------------------------------------------------
// X86
//---------------------------------------------------------------------
#ifndef _MSC_VER
asm( "\
.globl _ctx_getcontext \n\
.globl ctx_getcontext \n\
_ctx_getcontext: \n\
ctx_getcontext: \n\
	movl 4(%esp), %eax; \n\
	movl %eax, 0(%eax); \n\
	movl %ebx, 4(%eax); \n\
	movl %ecx, 8(%eax); \n\
	movl %edx, 12(%eax); \n\
	movl %esi, 16(%eax); \n\
	movl %edi, 20(%eax); \n\
	leal 4(%esp), %ecx; \n\
	movl %ecx, 24(%eax); \n\
	movl %ebp, 28(%eax); \n\
	movl 0(%esp), %ecx; \n\
	movl %ecx, 32(%eax); \n\
	pushfl; \n\
	pop %ecx;  \n\
	movl %ecx, 36(%eax); \n\
	movl 8(%eax), %ecx; \n\
	fnstenv 44(%eax); \n\
	fldenv 44(%eax); \n\
	xor %eax, %eax; \n\
	ret; \n\
");

asm( "\
.globl _ctx_setcontext \n\
.globl ctx_setcontext \n\
_ctx_setcontext:   \n\
ctx_setcontext:  \n\
	movl 4(%esp), %eax; \n\
	movl 4(%eax), %ebx; \n\
	movl 8(%eax), %ecx; \n\
	movl 12(%eax), %edx; \n\
	movl 16(%eax), %esi; \n\
	movl 20(%eax), %edi; \n\
	movl 24(%eax), %esp; \n\
	movl 28(%eax), %ebp; \n\
	movl 32(%eax), %ecx; \n\
	pushl %ecx; \n\
	movl 36(%eax), %ecx; \n\
	pushl %ecx; \n\
	popfl; \n\
	movl 8(%eax), %ecx; \n\
	fldenv 44(%eax); \n\
	movl 0(%eax), %eax; \n\
	ret; \n\
");

asm("\
.globl _ctx_swapcontext \n\
.globl ctx_swapcontext \n\
_ctx_swapcontext:   \n\
ctx_swapcontext:  \n\
	movl 4(%esp), %eax; \n\
	movl %eax, 0(%eax); \n\
	movl %ebx, 4(%eax); \n\
	movl %ecx, 8(%eax); \n\
	movl %edx, 12(%eax); \n\
	movl %esi, 16(%eax); \n\
	movl %edi, 20(%eax); \n\
	leal 4(%esp), %ecx; \n\
	movl %ecx, 24(%eax); \n\
	movl %ebp, 28(%eax); \n\
	movl 0(%esp), %ecx; \n\
	movl %ecx, 32(%eax); \n\
	pushfl; \n\
	pop %ecx;  \n\
	movl %ecx, 36(%eax); \n\
	movl 8(%eax), %ecx; \n\
	fnstenv 44(%eax); \n\
	movl 8(%esp), %eax; \n\
	movl 4(%eax), %ebx; \n\
	movl 8(%eax), %ecx; \n\
	movl 12(%eax), %edx; \n\
	movl 16(%eax), %esi; \n\
	movl 20(%eax), %edi; \n\
	movl 24(%eax), %esp; \n\
	movl 28(%eax), %ebp; \n\
	movl 32(%eax), %ecx; \n\
	pushl %ecx; \n\
	movl 36(%eax), %ecx; \n\
	pushl %ecx; \n\
	popfl; \n\
	movl 8(%eax), %ecx; \n\
	fldenv 44(%eax); \n\
	movl 0(%eax), %eax; \n\
	ret; \n\
");

#else
__declspec(naked) int ctx_getcontext(ctx_context_t *ctx)
{
	_asm {
		mov eax, [esp + 4];
		mov [eax], eax;
		mov [eax + 4], ebx;
		mov [eax + 8], ecx;
		mov [eax + 12], edx;
		mov [eax + 16], esi;
		mov [eax + 20], edi;
		lea ecx, [esp + 4];
		mov [eax + 24], ecx;
		mov [eax + 28], ebp;
		mov ecx, [esp];
		mov [eax + 32], ecx;
		pushfd;
		pop ecx;
		mov [eax + 36], ecx;
		mov ecx, [eax + 8];
		fnstenv [eax + 44];
		fldenv [eax + 44];
		xor eax, eax;
		ret;
	}
}

__declspec(naked) int ctx_setcontext(const ctx_context_t *ctx)
{
	_asm {
		mov eax, [esp + 4];
		mov ebx, [eax + 4];
		mov ecx, [eax + 8];
		mov edx, [eax + 12];
		mov esi, [eax + 16];
		mov edi, [eax + 20];
		mov esp, [eax + 24];
		mov ebp, [eax + 28];
		mov ecx, [eax + 32];
		push ecx; 
		mov ecx, [eax + 36];
		push ecx;
		popfd;
		mov ecx, [eax + 8];
		fldenv [eax + 44];
		mov eax, [eax + 0];
		ret;
	}
}

__declspec(naked) int ctx_swapcontext(ctx_context_t *old, const ctx_context_t *target)
{
	_asm {
		mov eax, [esp + 4];
		mov [eax], eax;
		mov [eax + 4], ebx;
		mov [eax + 8], ecx;
		mov [eax + 12], edx;
		mov [eax + 16], esi;
		mov [eax + 20], edi;
		lea ecx, [esp + 4];
		mov [eax + 24], ecx;
		mov [eax + 28], ebp;
		mov ecx, [esp];
		mov [eax + 32], ecx;
		pushfd;
		pop ecx;
		mov [eax + 36], ecx;
		mov ecx, [eax + 8];
		fnstenv [eax + 44];

		mov eax, [esp + 8];
		mov ebx, [eax + 4];
		mov ecx, [eax + 8];
		mov edx, [eax + 12];
		mov esi, [eax + 16];
		mov edi, [eax + 20];
		mov esp, [eax + 24];
		mov ebp, [eax + 28];
		mov ecx, [eax + 32];
		push ecx; 
		mov ecx, [eax + 36];
		push ecx;
		popfd;
		mov ecx, [eax + 8];
		fldenv [eax + 44];
		mov eax, [eax + 0];
		ret;
	}
}


#endif


static void ctx_context_startup(ctx_context_t *link, void *fun, void *args)
{
	int (*fn)(void*) = (int (*)(void*))fun;
	int hr;
	hr = fn(args);
	link->regs.eax = (hr == 0)? 0xffffffff : (unsigned int)hr;
	ctx_setcontext(link);
}


int ctx_makecontext(ctx_context_t *ctx, int (*fun)(void*args), void *args)
{
	char *esp = ctx->stack + ctx->stack_size;
	unsigned int *stack;
	esp = esp - ((size_t)esp & 15) - 16;
	stack = (unsigned int*)esp;
	stack[0] = 0;
	stack[1] = (unsigned int)(ctx->link);
	stack[2] = (unsigned int)((void*)fun);
	stack[3] = (unsigned int)args;
	ctx->regs.esp = (unsigned int)esp;
	ctx->regs.eip = (unsigned int)((void*)ctx_context_startup);
	return 0;
}


#elif CTX_ARCH_X86_64
//---------------------------------------------------------------------
// X86_64
//---------------------------------------------------------------------
#ifndef _MSC_VER
asm(" \
.globl _ctx_getcontext \n\
.globl ctx_getcontext \n\
_ctx_getcontext:  \n\
ctx_getcontext:  \n"
#if CTX_OS_WINDOWS || __CYGWIN__
	// Microsoft x64 calling convention (Windows, Cygwin)
	"movq %rcx, %rax; \n"
#else
	// System V AMD64 ABI (Linux, FreeBSD, OS X, Solaris)
	"movq %rdi, %rax; \n"
#endif
	"\n\
	movq %rdx, 24(%rax); \n\
	movq %rax, 0(%rax); \n\
	movq %rbx, 8(%rax); \n\
	movq %rcx, 16(%rax); \n\
	movq %rsi, 32(%rax); \n\
	movq %rdi, 40(%rax); \n\
	leaq 8(%rsp), %rdx; \n\
	movq %rdx, 48(%rax); \n\
	movq %rbp, 56(%rax); \n\
	movq 0(%rsp), %rdx; \n\
	movq %rdx, 64(%rax); \n\
	pushfq; \n\
	popq %rdx;  \n\
	movq %rdx, 72(%rax); \n\
	movq %r8, 80(%rax); \n\
	movq %r9, 88(%rax); \n\
	movq %r10, 96(%rax); \n\
	movq %r11, 104(%rax); \n\
	movq %r12, 112(%rax); \n\
	movq %r13, 120(%rax); \n\
	movq %r14, 128(%rax); \n\
	movq %r15, 136(%rax); \n\
	movq 24(%rax), %rdx; \n\
	stmxcsr 144(%rax); \n\
	fnstenv 152(%rax); \n\
	fldenv 152(%rax); \n\
	xor %rax, %rax; \n\
	ret; \n\
");


asm( "\
.globl _ctx_setcontext \n\
.globl ctx_setcontext \n\
_ctx_setcontext:   \n\
ctx_setcontext:  \n"
#if CTX_OS_WINDOWS || __CYGWIN__
	// Microsoft x64 calling convention (Windows, Cygwin)
	"movq %rcx, %rax; \n"
#else
	// System V AMD64 ABI (Linux, FreeBSD, OS X, Solaris)
	"movq %rdi, %rax; \n"
#endif
	"\n\
	movq 8(%rax), %rbx; \n\
	movq 16(%rax), %rcx; \n\
	movq 24(%rax), %rdx; \n\
	movq 32(%rax), %rsi; \n\
	movq 40(%rax), %rdi; \n\
	movq 48(%rax), %rsp; \n\
	movq 56(%rax), %rbp; \n\
	movq 64(%rax), %rdx; \n\
	pushq %rdx; \n\
	movq 72(%rax), %rdx; \n\
	pushq %rdx; \n\
	popfq; \n\
	movq 80(%rax), %r8; \n\
	movq 88(%rax), %r9; \n\
	movq 96(%rax), %r10; \n\
	movq 104(%rax), %r11; \n\
	movq 112(%rax), %r12; \n\
	movq 120(%rax), %r13; \n\
	movq 128(%rax), %r14; \n\
	movq 136(%rax), %r15; \n\
	movq 24(%rax), %rdx; \n\
	ldmxcsr 144(%rax); \n\
	fldenv 152(%rax); \n\
	movq $1, %rax; \n\
	ret; \n\
");

asm("\
.globl _ctx_swapcontext \n\
.globl ctx_swapcontext \n\
_ctx_swapcontext: \n\
ctx_swapcontext: \n"
#if CTX_OS_WINDOWS || __CYGWIN__
	"movq %rcx, %rax; \n"
#else
	"movq %rdi, %rax; \n"
#endif
	"\n\
	movq %rdx, 24(%rax); \n\
	movq %rax, 0(%rax); \n\
	movq %rbx, 8(%rax); \n\
	movq %rcx, 16(%rax); \n\
	movq %rsi, 32(%rax); \n\
	movq %rdi, 40(%rax); \n\
	leaq 8(%rsp), %rdx; \n\
	movq %rdx, 48(%rax); \n\
	movq %rbp, 56(%rax); \n\
	movq 0(%rsp), %rdx; \n\
	movq %rdx, 64(%rax); \n\
	pushfq; \n\
	popq %rdx;  \n\
	movq %rdx, 72(%rax); \n\
	movq %r8, 80(%rax); \n\
	movq %r9, 88(%rax); \n\
	movq %r10, 96(%rax); \n\
	movq %r11, 104(%rax); \n\
	movq %r12, 112(%rax); \n\
	movq %r13, 120(%rax); \n\
	movq %r14, 128(%rax); \n\
	movq %r15, 136(%rax); \n\
	movq 24(%rax), %rdx; \n\
	stmxcsr 144(%rax); \n\
	fnstenv 152(%rax); \n\
	fldenv 152(%rax); \n"
#if CTX_OS_WINDOWS || __CYGWIN__
	"movq %rdx, %rax\n"
#else
	"movq %rsi, %rax\n"
#endif
	"\n\
	movq 8(%rax), %rbx; \n\
	movq 16(%rax), %rcx; \n\
	movq 24(%rax), %rdx; \n\
	movq 32(%rax), %rsi; \n\
	movq 40(%rax), %rdi; \n\
	movq 48(%rax), %rsp; \n\
	movq 56(%rax), %rbp; \n\
	movq 64(%rax), %rdx; \n\
	pushq %rdx; \n\
	movq 72(%rax), %rdx; \n\
	pushq %rdx; \n\
	popfq; \n\
	movq 80(%rax), %r8; \n\
	movq 88(%rax), %r9; \n\
	movq 96(%rax), %r10; \n\
	movq 104(%rax), %r11; \n\
	movq 112(%rax), %r12; \n\
	movq 120(%rax), %r13; \n\
	movq 128(%rax), %r14; \n\
	movq 136(%rax), %r15; \n\
	movq 24(%rax), %rdx; \n\
	ldmxcsr 144(%rax); \n\
	fldenv 152(%rax); \n\
	movq 0(%rax), %rax; \n\
	ret; \n\
");

#else

// Inline assembly is inhibited in MSVC on X86_64
#endif


static void ctx_context_startup(ctx_context_t *link, void *fun, void *args)
{
	int (*fn)(void*) = (int (*)(void*))fun;
	int hr;
	hr = fn(args);
	link->regs.rax = (hr == 0)? (~((unsigned long long)0)) : (unsigned int)hr;
	ctx_setcontext(link);
}

int ctx_makecontext(ctx_context_t *ctx, int (*fun)(void*args), void *args)
{
	char *rsp = ctx->stack + ctx->stack_size;
	unsigned long long *stack;
	rsp = rsp - ((size_t)rsp & 63) - 64;
	stack = (unsigned long long*)rsp;
	stack[0] = 0;
	stack[1] = (unsigned long long)(ctx->link);
	stack[2] = (unsigned long long)((void*)fun);
	stack[3] = (unsigned long long)args;
	ctx->regs.rsp = ((unsigned long long)rsp - 0);
	ctx->regs.rip = (unsigned long long)((void*)ctx_context_startup);
#if CTX_OS_WINDOWS || __CYGWIN__
	ctx->regs.rcx = stack[1];
	ctx->regs.rdx = stack[2];
	ctx->regs.r8 = stack[3];
#else
	ctx->regs.rdi = stack[1];
	ctx->regs.rsi = stack[2];
	ctx->regs.rdx = stack[3];
#endif
	return 0;
}


#endif


