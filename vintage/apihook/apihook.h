//=====================================================================
// 
// apihook.h - win32 api hook help
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __APIHOOK_H__
#define __APIHOOK_H__

#if defined(WIN32) && (!defined(_WIN32))
	#define _WIN32
#endif

#ifndef _WIN32
	#error the file must be compiled under windows
#endif


#define WIN32_LEAN_AND_MEAN
#include <windows.h>


//---------------------------------------------------------------------
// ApiHookFunction 
//---------------------------------------------------------------------
struct ApiHookFunction
{
	const char *Name;	// 函数名称，比如："DirectDrawCreateEx"
	void *HookFn;		// Hook函数地址。
	void *OrigFn;		// 原函数地址，ApiHookInstall时生成。
};



#ifdef __cplusplus
extern "C" {
#endif


//=====================================================================
// 方法一：修改 IAT方式HOOK，可以 Hook任何从 DLL import进来的函数，但
// 不能 Hook通过 GetProcAddress得到地址的函数。
//=====================================================================

// Name - DLL名称, 比如 "kernel32.dll"
// Hook - hook数组. Name项为NULL代表结束
// 返回零为成功，其他值为错误。
int ApiHookInstall(const char *Name, struct ApiHookFunction *Hook);

// Name - DLL名称, 比如 "kernel32.dll"
// Hook - hook数组. Name项为NULL代表结束
// 返回零为成功，其他值为错误。
int ApiHookRemove(const char *Name, const struct ApiHookFunction *Hook);


//=====================================================================
// 方法二：修改代码
//=====================================================================

// 用法: OrigFn = ApiHookDetour(LoadLibrary(hDLL, "n"), hookProc, 64)
// 说明：分配内存，复制 src指向的源函数处 len个字节的代码，后改写src处
// 代码前5个字节为 "jmp 新函数" 指令，并返刚才保存复制代码的那块新内存
void *ApiHookDetour(void *src, const void *dst, int len);


// 用法: ApiHookRetour(LoadLibrary(hDLL, "n"), OrigFn, 64)
// 注意: ApiHookRetour以后, 不能调用 ApiHookDetour返回的 OrigFn 地址了
// 因为该地址指向的代码已经被释放。
int ApiHookRetour(void *src, void *restore, int len);



//=====================================================================
// 方法三：改写指令
//=====================================================================
struct ApiHookCode
{
	unsigned char *address;
	unsigned int codelen;
	unsigned char newfun[16];
	unsigned char srcfun[16];
};

void ApiHookCodeInit(struct ApiHookCode *code, void *SrcFn, void *NewFn);

void ApiHookCodeOn(struct ApiHookCode *code);

void ApiHookCodeOff(struct ApiHookCode *code);


//=====================================================================
// 方法四：Hook GetProcAddress然后再判断是否是需要 HOOK的东西。
//=====================================================================
typedef FARPROC (WINAPI *ApiHookProcGPA)(HMODULE hModule, LPCSTR lpProcName);

// 初始化替换 GetProcAddress
// NewGetProcAddress 为 NULL时将还原本来的 GetProcAddress
int ApiHookSetGPA(ApiHookProcGPA NewGetProcAddress);

// 调用原来的 GetProcAddress
FARPROC GetProcAddress_Orignal(HMODULE hModule, LPCSTR lpProcName);


//=====================================================================
// 高级接口
//=====================================================================

// 单独 HookIAT一个函数
void *ApiHookIAT(const char *DllName, const char *FunName, void *HookFn);

// 取得 COM对象的方法
#define APIHOOKCOM(obj, lvl, idx) ((void**)(*((char**)(obj) + (lvl))))[idx]


// HOOK COM接口，返回老接口
void *ApiHookCOM(void *comobj, int nFunIndex, void *NewFun);


// 注入 Dll到远程进程 (Unicode)
// ProcessId - 远程进程id
// LibFile   - DLL名称
// th        - 返回远程加载DLL线程的 Handle
// wait      - 是否等待远程Dll加载成功（0否，1是），即加载并 DllMain返回
int ApiHookInjectLibW(DWORD ProcessId, PCWSTR LibFile, HANDLE *th, int wait);


// 注入 Dll到远程进程 (Ansi)
// ProcessId - 远程进程id
// LibFile   - DLL名称
// th        - 返回远程加载DLL线程的 Handle
// wait      - 是否等待远程Dll加载成功（0否，1是），即加载并 DllMain返回
int ApiHookInjectLibA(DWORD ProcessId, PCSTR LibFile, HANDLE *th, int wait);



//=====================================================================
// 特殊HOOK：综合方法Hook D3D8, D3D9, DDRAW, DINPUT
//=====================================================================

// 特殊HOOK: 创建完 D3D接口后会调用该函数，并将该函数返回值返回。
typedef void* (*AdapterDirect3DCreate8_t)(void *obj, UINT sdkver);
typedef void* (*AdapterDirect3DCreate9_t)(void *obj, UINT sdkver);


int ApiHook_D3D8_Set(AdapterDirect3DCreate8_t adapter);


int ApiHook_D3D9_Set(AdapterDirect3DCreate9_t adapter);




//=====================================================================
// 日志输出
//=====================================================================
void ApiHookLogSet(const char *fname);

void ApiHookLog(const char *fmt, ...);



#ifdef __cplusplus
}
#endif

#endif



