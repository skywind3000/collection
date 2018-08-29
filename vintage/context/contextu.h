#ifndef _CONTEXTU_H_
#define _CONTEXTU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_NOT_STDDEF_H
#include <stddef.h>
#endif


//---------------------------------------------------------------------
// ARCH DETECT
//---------------------------------------------------------------------
#ifndef CTX_ARCH_DEFINED
#define CTX_ARCH_DEFINED

#if defined(__amd64) || defined(__amd64__) || defined(_M_X86_64)
	#define CTX_ARCH_X86_64		1
#elif defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64)
	#define CTX_ARCH_X86_64		1
#elif defined(__i386) || defined(__i386__) || defined(_M_X86)
	#define CTX_ARCH_X86		1
#elif defined(__i486__) || defined(__i586__) || defined(__i686__)
	#define CTX_ARCH_X86		1
#elif defined(_M_IX86) || defined(__X86__) || defined(_X86_)
	#define CTX_ARCH_X86		1
#elif defined(__arm__) || defined(_ARM) || defined(_M_ARM)
	#define CTX_ARCH_ARM		1
#elif defined(__arm) || defined(__TARGET_ARCH_ARM)
	#define CTX_ARCH_ARM		1
#elif defined(__thumb) || defined(_M_ARMT) || defined(__TARGET_ARCH_THUMB)
	#define CTX_ARCH_ARMT		1
#elif defined(__aarch64__)
	#define CTX_ARCH_ARM64		1
#elif defined(__m68k__) || defined(__MC68K__)
	#define CTX_ARCH_M68K		1
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__)
	#define CTX_ARCH_MIPS		1
#elif defined(_R3000) || defined(_R4000) || defined(_R5900)
	#define CTX_ARCH_MIPS		1
#else
	#define CTX_ARCH_UNKNOW		1
#endif

#endif


#if defined(_WIN32) || defined(WIN32) || defined(WIN64) || defined(_WIN64)
#define CTX_OS_WINDOWS		1
#else
#define CTX_OS_UNIX			1
#endif


//=====================================================================
// ctx_regs_t
//=====================================================================
#if CTX_ARCH_X86 || CTX_ARCH_X86_64
typedef struct _ctx_regs_t {
#if CTX_ARCH_X86
	unsigned int eax;		// 0
	unsigned int ebx;		// 4
	unsigned int ecx;		// 8
	unsigned int edx;		// 12
	unsigned int esi;		// 16
	unsigned int edi;		// 20
	unsigned int esp;		// 24
	unsigned int ebp;		// 28
	unsigned int eip;		// 32
	unsigned int eflags;	// 36
	unsigned int mxcsr;		// 40
#else
	unsigned long long rax;		// 0
	unsigned long long rbx;		// 8
	unsigned long long rcx;		// 16
	unsigned long long rdx;		// 24
	unsigned long long rsi;		// 32
	unsigned long long rdi;		// 40
	unsigned long long rsp;		// 48
	unsigned long long rbp;		// 56
	unsigned long long rip;		// 64
	unsigned long long rflags;	// 72
	unsigned long long r8;		// 80
	unsigned long long r9;		// 88
	unsigned long long r10;		// 96
	unsigned long long r11;		// 104
	unsigned long long r12;		// 112
	unsigned long long r13;		// 120
	unsigned long long r14;		// 128
	unsigned long long r15;		// 136
	unsigned int mxcsr;			// 144
	unsigned int padding;		// 148
#endif
	unsigned char fpregs[32];		// 44(32) / 152(64)
	unsigned int reserved[18];		// 76(32) / 186(64)
}	ctx_regs_t;


#else
#error Architecture not supported !!
#endif


//=====================================================================
// ctx_context_t
//=====================================================================
typedef struct _ctx_context_t {
	struct _ctx_regs_t regs;
	struct _ctx_context_t *link;
	char *stack;
	int stack_size;
}	ctx_context_t;



#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// interfaces
//---------------------------------------------------------------------

int ctx_getcontext(ctx_context_t *ctx);

int ctx_setcontext(const ctx_context_t *ctx);

int ctx_makecontext(ctx_context_t *ctx, int (*fun)(void*args), void *args);

int ctx_swapcontext(ctx_context_t *save, const ctx_context_t *target);


#ifdef __cplusplus
}
#endif




#endif


