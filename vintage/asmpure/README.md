Introduction
============
Asmpure is a reimplementation and an enhancement of [SoftWire](http://gna.org/projects/softwire) in C for compiling assembly code. It can be used in projects to generate x86 machine code at run-time as an alternative to self-modifying code. Scripting languages might also benefit by using Asmpure as a JIT-compiler back-end. It also allows to eliminate jumps for variables which are temporarily constant during run-time, like for efficient graphics processing by constructing an optimised pipeline. Because of its possibility for 'instruction rewiring' by run-time conditional compilation, I named it "Asmpure". It is targeted only at developers with a good knowledge of C++ and x86 assembly. 

Examples
========

CrossProduct
------------
```cpp
/*
void CrossProduct(float *V0, float *V1, float *V2)
{
        V2[0] = V0[1] * V1[2] - V0[2] * V1[1];
        V2[1] = V0[2] * V1[0] - V0[0] * V1[2];
        V2[2] = V0[0] * V1[1] - V0[1] * V1[0];
}*/

const char *CrossProductAsm = 
"    mov        ecx, [esp+8]\n"
"    mov        eax, [esp+4]\n"
"    mov        edx, [esp+12]\n"
"\n"    
"    fld        DWORD [ecx+8]\n"
"    fmul       DWORD [eax+4]\n"
"    fld        DWORD [eax+8]\n"
"    fmul       DWORD [ecx+4]\n"
"    fsubp      st1, st0\n"
"    fstp       DWORD [edx]\n"
"\n"    
"    fld        DWORD [eax+8]\n"
"    fmul       DWORD [ecx]\n"
"    fld        DWORD [eax]\n"
"    fmul       DWORD [ecx+8]\n"
"    fsubp      st1, st0\n"
"    fstp       DWORD [edx+4]\n"
"\n"    
"    fld        DWORD [eax]\n"
"    fmul       DWORD [ecx+4]\n"
"    fld        DWORD [ecx]\n"
"    fmul       DWORD [eax+4]\n"
"    fsubp      st1, st0\n"
"    fstp       DWORD [edx+8]\n"
"    \n"
"    ret\n";


void testCrossProduct(void)
{
        CAssembler *casm;
        int size, c;

        void (*CrossProductPtr)(float*, float*, float*);

        // create assembler
        casm = casm_create();

        // append assembly source
        casm_source(casm, CrossProductAsm);

        // calculate size
        size = casm_compile(casm, NULL, 0);

        if (size < 0) {
                printf("compile error: %s\n", casm->error);
                casm_release(casm);
                return;
        }

        CrossProductPtr = (void (*)(float*, float*, float*))malloc(size);

        casm_compile(casm, (unsigned char*)CrossProductPtr, size);

        printf("==================== Cross Product ====================\n");

        casm_dumpinst(casm, stdout);

        printf("\nExecute code (y/n)?\n\n");

        do
        {
                c = getch();
        }
        while(c != 'y' && c != 'n');

        if(c == 'y')
        {
                float V0[3] = {1, 0, 0};
                float V1[3] = {0, 1, 0};
                float V2[3];

                CrossProductPtr(V0, V1, V2);

                printf("output: (%.3f, %.3f, %.3f)\n\n", V2[0], V2[1], V2[2]);
        }

        free(CrossProductPtr);
        casm_release(casm);
}
```

*output: (0.000, 0.000, 1.000) *

Hello World
-----------
```cpp
const char *HelloWorldAsm = 
"    mov     eax,  [esp+8]\n"
"    push    eax\n"
"    call    DWORD [esp+8]\n"
"    pop     ecx\n"
"    ret\n";

void testHelloWorld(void)
{
        CAssembler *casm;
        int size, c;

        void (*HelloWorldPtr)(void*, const char*);

        // create assembler
        casm = casm_create();

        // append assembly source
        casm_source(casm, HelloWorldAsm);

        // calculate size
        size = casm_compile(casm, NULL, 0);

        if (size < 0) {
                printf("compile error: %s\n", casm->error);
                casm_release(casm);
                return;
        }

        HelloWorldPtr = (void (*)(void*, const char*))malloc(size);

        casm_compile(casm, (unsigned char*)HelloWorldPtr, size);

        printf("==================== Hello World ====================\n");

        casm_dumpinst(casm, stdout);

        printf("\nExecute code (y/n)?\n\n");

        do
        {
                c = getch();
        }
        while(c != 'y' && c != 'n');

        if(c == 'y')
        {
                HelloWorldPtr((void*)printf, "Hello, World !!\n");
        }

        free(HelloWorldPtr);
        casm_release(casm);
}
```
*output: Hello, World !! *

Alpha Blend
-----------
```cpp
const char *AlphaBlendAsm = 
"PROC C1:DWORD, C2:DWORD, A:DWORD\n"
"    movd mm0, A\n"
"    punpcklwd mm0, mm0\n"
"    punpckldq mm0, mm0\n"
"    pcmpeqb mm7, mm7\n"
"    psubw mm7, mm0\n"
"    \n"
"    punpcklbw mm1, C1\n"
"    psrlw mm1, 8\n"
"    punpcklbw mm2, C2\n"
"    psrlw mm2, 8\n"
"    \n"
"    pmullw mm1, mm7\n"
"    pmullw mm2, mm0\n"
"    paddw mm1, mm2\n"
"    \n"
"    psrlw mm1, 8\n"
"    packuswb mm1, mm1\n"
"    movd eax, mm1\n"
"    emms\n"
"    ret\n"
"ENDP\n";


void testAlphaBlend(void)
{
        CAssembler *casm;
        int c;

        int (*AlphaBlendPtr)(int, int, int);

        // create assembler
        casm = casm_create();

        // append assembly source
        casm_source(casm, AlphaBlendAsm);


        AlphaBlendPtr = (int (*)(int, int, int))casm_callable(casm, NULL);

        if (AlphaBlendPtr == NULL) {
                printf("error: %s\n", casm->error);
                casm_release(casm);
                return;
        }

        printf("==================== Alpha Blend ====================\n");

        casm_dumpinst(casm, stdout);

        printf("\nExecute code (y/n)?\n\n");

        do
        {
                c = getch();
        }
        while(c != 'y' && c != 'n');

        if(c == 'y')
        {
                int x = AlphaBlendPtr(0x00FF00FF, 0xFF00FF00, 128);
                printf("output: %.8X\n\n", x);
        }

        free(AlphaBlendPtr);
        casm_release(casm);
}
```
*output: 7f7f7f7f*

