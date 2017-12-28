IFDEF RAX

PUBLIC ctx_getcontext, ctx_setcontext, ctx_swapcontext

.code

ctx_getcontext PROC
	mov    rax,rcx;
	mov    [rax + 24], rdx;
	mov    [rax],rax
	mov    [rax+8],rbx
	mov    [rax+16],rcx
	mov    [rax+32],rsi
	mov    [rax+40],rdi
	lea    rdx,[rsp+8]
	mov    [rax+48],rdx
	mov    [rax+56],rbp
	mov    rdx,[rsp+0]
	mov    [rax+64],rdx
	pushfq
	pop    rdx
	mov    [rax+72],rdx
	mov    [rax+80],r8
	mov    [rax+88],r9
	mov    [rax+96],r10
	mov    [rax+104],r11
	mov    [rax+112],r12
	mov    [rax+120],r13
	mov    [rax+128],r14
	mov    [rax+136],r15
	mov    rdx,[rax+24]
	stmxcsr [rax+144]
	fnstenv [rax+152]
	fldenv [rax+152]
	xor    rax,rax
	ret    

ctx_getcontext ENDP

ctx_setcontext PROC
	mov    rax,rcx
	mov    rbx,QWORD PTR [rax+8]
	mov    rcx,QWORD PTR [rax+16]
	mov    rdx,QWORD PTR [rax+24]
	mov    rsi,QWORD PTR [rax+32]
	mov    rdi,QWORD PTR [rax+40]
	mov    rsp,QWORD PTR [rax+48]
	mov    rbp,QWORD PTR [rax+56]
	mov    rdx,QWORD PTR [rax+64]
	push   rdx
	mov    rdx,QWORD PTR [rax+72]
	push   rdx
	popfq
	mov    r8,QWORD PTR [rax+80]
	mov    r9,QWORD PTR [rax+88]
	mov    r10,QWORD PTR [rax+96]
	mov    r11,QWORD PTR [rax+104]
	mov    r12,QWORD PTR [rax+112]
	mov    r13,QWORD PTR [rax+120]
	mov    r14,QWORD PTR [rax+128]
	mov    r15,QWORD PTR [rax+136]
	mov    rdx,QWORD PTR [rax+24]
	ldmxcsr DWORD PTR [rax+144]
	fldenv [rax+152]
	mov    rax,QWORD PTR [rax]
	ret    
ctx_setcontext ENDP

ctx_swapcontext PROC
	mov    rax, rcx;
	mov    [rax], rax
	mov    [rax+8],rbx
	mov    [rax+16],rcx
	mov    [rax+24],rdx;
	mov    [rax+32],rsi
	mov    [rax+40],rdi
	lea    rdx,[rsp+8]
	mov    [rax+48],rdx
	mov    [rax+56],rbp
	mov    rdx,[rsp+0]
	mov    [rax+64],rdx
	pushfq
	pop    rdx
	mov    [rax+72],rdx
	mov    [rax+80],r8
	mov    [rax+88],r9
	mov    [rax+96],r10
	mov    [rax+104],r11
	mov    [rax+112],r12
	mov    [rax+120],r13
	mov    [rax+128],r14
	mov    [rax+136],r15
	mov    rdx,[rax+24]
	stmxcsr [rax+144]
	fnstenv [rax+152]

	mov	   rax, rdx
	mov    rbx,QWORD PTR [rax+8]
	mov    rcx,QWORD PTR [rax+16]
	mov    rdx,QWORD PTR [rax+24]
	mov    rsi,QWORD PTR [rax+32]
	mov    rdi,QWORD PTR [rax+40]
	mov    rsp,QWORD PTR [rax+48]
	mov    rbp,QWORD PTR [rax+56]
	mov    rdx,QWORD PTR [rax+64]
	push   rdx
	mov    rdx,QWORD PTR [rax+72]
	push   rdx
	popfq
	mov    r8,QWORD PTR [rax+80]
	mov    r9,QWORD PTR [rax+88]
	mov    r10,QWORD PTR [rax+96]
	mov    r11,QWORD PTR [rax+104]
	mov    r12,QWORD PTR [rax+112]
	mov    r13,QWORD PTR [rax+120]
	mov    r14,QWORD PTR [rax+128]
	mov    r15,QWORD PTR [rax+136]
	mov    rdx,QWORD PTR [rax+24]
	ldmxcsr DWORD PTR [rax+144]
	fldenv [rax+152]
	mov    rax,QWORD PTR [rax]
	ret
ctx_swapcontext ENDP

ENDIF

END



