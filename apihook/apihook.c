//=====================================================================
// 
// apihook.c - win32 api hook help
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "apihook.h"

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#endif



//=====================================================================
// 公共定义
//=====================================================================

// 指针强制转换
#define MakePtr(cast, ptr, offset) (cast)((size_t)(ptr) + (size_t)(offset))


//=====================================================================
// 方法一：修改 IAT方式HOOK，可以 Hook任何从 DLL import进来的函数，但
// 不能 Hook通过 GetProcAddress得到地址的函数。
//=====================================================================

//---------------------------------------------------------------------
// 返回指向 PE头的指针
//---------------------------------------------------------------------
PIMAGE_NT_HEADERS PEHeaderFromHModule(HMODULE hModule)
{
	PIMAGE_DOS_HEADER pDOSHeader = 0;
	PIMAGE_NT_HEADERS pNTHeader = 0;
    
	pDOSHeader = (PIMAGE_DOS_HEADER)hModule;

	if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	pNTHeader = (PIMAGE_NT_HEADERS)((char*)hModule + pDOSHeader->e_lfanew);

	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return NULL;

    return pNTHeader;
}


//------------------------------------------------------------------
// 重定向 IAT，参考 MSDN:
// Builds stubs for and redirects the IAT for one DLL (pImportDesc)
// Based on DelayLoadProfileDLL.CPP, by Matt Pietrek for MSJ.
//------------------------------------------------------------------
int ApiHookRedirectIAT(	const char *DLLName, 
						struct ApiHookFunction* FunHooks, 
						PIMAGE_IMPORT_DESCRIPTOR pImportDesc, 
						PVOID pBaseLoadAddr)
{
    PIMAGE_THUNK_DATA pIAT;				// Ptr to import address table
    PIMAGE_THUNK_DATA pINT;				// Ptr to import names table
    PIMAGE_THUNK_DATA pIteratingIAT;
	DWORD flOldProtect, flNewProtect, flDontCare;
	MEMORY_BASIC_INFORMATION mbi;
	unsigned cFuncs = 0;

    // Figure out which OS platform we're on
    OSVERSIONINFO osvi; 
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    // If no import names table, we can't redirect this, so bail
    if (pImportDesc->OriginalFirstThunk == 0)
        return -1;

    pIAT = MakePtr(PIMAGE_THUNK_DATA, pBaseLoadAddr, 
		pImportDesc->FirstThunk);
    pINT = MakePtr(PIMAGE_THUNK_DATA, pBaseLoadAddr,
		pImportDesc->OriginalFirstThunk);

    // Count how many entries there are in this IAT.  Array is 0 terminated
    for (pIteratingIAT = pIAT, cFuncs = 0; pIteratingIAT->u1.Function; ) {
        cFuncs++;
        pIteratingIAT++;
    }

    if (cFuncs == 0)  // If no imported functions, we're done!
        return -2;

    // These next few lines ensure that we'll be able to modify the IAT,
    // which is often in a read-only section in the EXE.

    // Get the current protection attributes                            
    VirtualQuery(pIAT, &mbi, sizeof(mbi));
    
    // remove ReadOnly and ExecuteRead attributes, add on ReadWrite flag
    flNewProtect = mbi.Protect;
    flNewProtect &= ~(PAGE_READONLY | PAGE_EXECUTE_READ);
    flNewProtect |= (PAGE_READWRITE);
    
    if (!VirtualProtect(pIAT, sizeof(PVOID) * cFuncs,
                        flNewProtect, &flOldProtect)) {
        return -3;
    }

	// Scan through the IAT, completing the stubs and redirecting the IAT
	// entries to point to the stubs
	for (pIteratingIAT = pIAT; pIteratingIAT->u1.Function; pIteratingIAT++)
	{
		void *HookFn = 0;  // Set to either the SFunctionHook or pStubs.

		if (!IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))   // import by name
		{
			PIMAGE_IMPORT_BY_NAME pImportName = MakePtr(
				PIMAGE_IMPORT_BY_NAME,
				pBaseLoadAddr, pINT->u1.AddressOfData);

			// Iterate through the hook functions, searching for this import.
			struct ApiHookFunction* FHook = FunHooks;

			while (FHook->Name) {
				if (lstrcmpiA(FHook->Name, (char*)pImportName->Name) == 0) {
					OutputDebugStringA("Hooked function: ");
					OutputDebugStringA((const char*)pImportName->Name);
					OutputDebugStringA("\n");
					// Save the old function in the SFunctionHook structure 
					// and get the new one.
					FHook->OrigFn = (void*)(pIteratingIAT->u1.Function);
					HookFn = FHook->HookFn;
					break;
				}
				FHook++;
			}
		}

		// Replace the IAT function pointer if we have a hook.
		if (HookFn) {
			// Cheez-o hack to see if what we're importing is code or data.
			// If it's code, we shouldn't be able to write to it
			if (IsBadWritePtr((PVOID)pIteratingIAT->u1.Function, 1)) {
				#if !(defined(WIN64) || defined(_WIN64))
				pIteratingIAT->u1.Function = (DWORD)(HookFn);
				#elif defined(_MSC_VER)
				pIteratingIAT->u1.Function = (unsigned __int64)(HookFn);
				#else
				pIteratingIAT->u1.Function = (unsigned long long)(HookFn);
				#endif
			}
			else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
				// Special hack for Win9X, which builds stubs for imported
				// functions in system DLLs (Loaded above 2GB). These stubs 
				// are writeable,so we have to explicitly check for this case
				if (pIteratingIAT->u1.Function > 0x80000000) {
					#if !(defined(WIN64) || defined(_WIN64))
					pIteratingIAT->u1.Function = (DWORD)(HookFn);
					#elif defined(_MSC_VER)
					pIteratingIAT->u1.Function = (unsigned __int64)(HookFn);
					#else
					pIteratingIAT->u1.Function = (unsigned long long)(HookFn);
					#endif
				}
			}
		}

		pINT++;             // Advance to next INT entry
	}

	// Put the page attributes back the way they were.
	VirtualProtect(pIAT, sizeof(PVOID) * cFuncs, flOldProtect, &flDontCare);

	return 0;
}


//---------------------------------------------------------------------
// Name - DLL名称, 比如 "kernel32.dll"
// Hook - hook数组. Name项为NULL代表结束
// 返回零为成功，其他值为错误。
//---------------------------------------------------------------------
int ApiHookInstall(const char *Name, struct ApiHookFunction *Hook)
{
	HMODULE hModEXE;
	PIMAGE_NT_HEADERS pExeNTHdr;
	DWORD importRVA;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	int i;

	hModEXE = GetModuleHandle(0);
	pExeNTHdr = PEHeaderFromHModule(hModEXE);

	if (pExeNTHdr == NULL) 
		return -100;

	importRVA = pExeNTHdr->OptionalHeader.DataDirectory
				[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    if (!importRVA)
        return -200;

	// Setup Check Flag
	for (i = 0; Hook[i].Name != NULL; i++) {
		Hook[i].OrigFn = NULL;
	}

	// Convert imports RVA to a usable pointer
	pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, hModEXE, importRVA);

	while (pImportDesc->FirstThunk) {
		PSTR pszImportModuleName = MakePtr(PSTR, hModEXE, pImportDesc->Name);

		if (lstrcmpiA(pszImportModuleName, Name) == 0) {
			OutputDebugStringA("Found ");
			OutputDebugStringA(Name);
			OutputDebugStringA("...\n");
			return ApiHookRedirectIAT(Name, Hook, pImportDesc,
				(PVOID)hModEXE);
		}
		
		pImportDesc++;
	}
	
	return -300;
}


//---------------------------------------------------------------------
// Name - DLL名称, 比如 "kernel32.dll"
// Hook - hook数组. Name项为NULL代表结束
// 返回零为成功，其他值为错误。
//---------------------------------------------------------------------
int ApiHookRemove(const char *Name, const struct ApiHookFunction *Hook)
{
	struct ApiHookFunction *NewHook;
	int retval = 0;
	int count = 0;
	int i;

	for (count = 0; Hook[count].Name != NULL; count++);

	if (count == 0) return 0;

	NewHook = (struct ApiHookFunction*)malloc(
					sizeof(struct ApiHookFunction) * (count + 1));

	if (NewHook == NULL) return -1000;

	for (i = 0; i < count; i++) {
		NewHook[i].Name = Hook[i].Name;
		NewHook[i].HookFn = Hook[i].OrigFn;
		if (NewHook[i].HookFn == NULL) retval = -2000;
	}

	NewHook[i].Name = NULL;

	if (retval == 0) {
		retval = ApiHookInstall(Name, NewHook);
	}

	free(NewHook);

	return retval;
}


//=====================================================================
// 方法二：修改代码
//=====================================================================

//---------------------------------------------------------------------
// 用法: OrigFn = ApiHookDetour(LoadLibrary(hDLL, "n"), hookProc, 64)
// 说明：分配内存，复制 src指向的源函数处 len个字节的代码，后改写src处
// 代码前5个字节为 "jmp 新函数" 指令，并返刚才保存复制代码的那块新内存
//---------------------------------------------------------------------
void *ApiHookDetour(void *src, const void *dst, int len)
{
	BYTE *jmp;
	DWORD dwback;

	// 分配内存
	jmp = (BYTE*)VirtualAlloc(NULL, len + 13, MEM_COMMIT, 
								PAGE_EXECUTE_READWRITE);

	if (jmp == NULL) return NULL;

	// 设置标志，用来判断是否二次 Retour
	jmp[0] = 'H';
	jmp[1] = 'O';
	jmp[2] = 'O';
	jmp[3] = 'K';

	*(size_t*)(jmp + 4) = (size_t)jmp;

	jmp += 8;

	// 拷贝代码
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwback);
	memcpy(jmp, src, len);
	jmp += len;

	((BYTE*)jmp)[0] = 0xE9;
	*(DWORD*)((char*)jmp + 1) = (DWORD)((char*)src - (char*)jmp + len) - 5;

	// 设置新指令：jmp dst
	((BYTE*)src)[0] = 0xE9;
	*(DWORD*)((char*)src + 1) = (DWORD)((char*)dst - (char*)src) - 5;

	// 页面权限恢复
	VirtualProtect(src, len, dwback, &dwback);

	jmp -= len;

	return jmp;
}


//---------------------------------------------------------------------
// 用法: ApiHookRetour(LoadLibrary(hDLL, "n"), OrigFn, 64)
// 注意: ApiHookRetour以后, 不能调用 ApiHookDetour返回的 OrigFn 地址了
// 因为该地址指向的代码已经被释放。
//---------------------------------------------------------------------
int ApiHookRetour(void *src, void *restore, int len)
{
	BYTE *jmp;
	DWORD dwback;

	jmp = (BYTE*)((BYTE*)restore - 8);

	// 检测1
	if (jmp[0] != 'H' || jmp[1] != 'O' || jmp[2] != 'O' || jmp[3] != 'K')
		return -1;

	// 检测2
	if (*(size_t*)(jmp + 4) != (size_t)jmp)
		return -2;

	if (!VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwback))
		return -3;

	if (!memcpy(src, restore, len))
		return -4;

	((BYTE*)restore)[0] = 0xE9;
	*(size_t*)((char*)restore + 1) = 
		(size_t)((char*)src - (char*)restore) - 5;
	
	// 用 INT3指令填充内存块，避免被错误调用
	memset(jmp, 0xCC, 8 + len + 5);
	VirtualFree((LPVOID)jmp, 0, MEM_RELEASE);

	if (!VirtualProtect(src, len, dwback, &dwback))
		return -3;

	return 0;
}


//=====================================================================
// 方法三：改写指令
//=====================================================================
void ApiHookCodeInit(struct ApiHookCode *code, void *SrcFn, void *NewFn)
{
	code->address = (unsigned char*)SrcFn;

	if (sizeof(void*) == 4) {		// 32位版本
		code->codelen = 5;
		code->newfun[0] = 0xE9;
		*(unsigned long*)(code->newfun + 1) = (unsigned long)(
			(char*)NewFn - (char*)SrcFn - 5);
	}	
	else {							// 64位版本
		code->codelen = 14;
		code->newfun[0] = 0xff;
		code->newfun[1] = 0x25;
		code->newfun[2] = 0;
		code->newfun[3] = 0;
		code->newfun[4] = 0;
		code->newfun[5] = 0;
		*(size_t*)(code->newfun + 6) = (size_t)((char*)NewFn);
	}

	#if defined(_WIN32) || defined(WIN32) || \
		defined(_WIN64) || defined(WIN64)
	{
		DWORD dwback;
		VirtualProtect(code->address, code->codelen, 
			PAGE_EXECUTE_READWRITE, &dwback);
		memcpy(code->srcfun, code->address, code->codelen);
		VirtualProtect(code->address, code->codelen, 
			dwback, &dwback);
	}
	#else
	{
		memcpy(code->srcfun, code->address, code->codelen);
	}
	#endif
}

void ApiHookCodeOn(struct ApiHookCode *code)
{
	#if defined(_WIN32) || defined(WIN32) || \
		defined(_WIN64) || defined(WIN64)
	DWORD dwback;
	VirtualProtect(code->address, code->codelen, 
		PAGE_EXECUTE_READWRITE, &dwback);
	memcpy(code->address, code->newfun, code->codelen);
	VirtualProtect(code->address, code->codelen, dwback, &dwback);
	#else
	memcpy(code->address, code->newfun, code->codelen);
	#endif
}

void ApiHookCodeOff(struct ApiHookCode *code)
{
	#if defined(_WIN32) || defined(WIN32) || \
		defined(_WIN64) || defined(WIN64)
	DWORD dwback;
	VirtualProtect(code->address, code->codelen, 
		PAGE_EXECUTE_READWRITE, &dwback);
	memcpy(code->address, code->srcfun, code->codelen);
	VirtualProtect(code->address, code->codelen, dwback, &dwback);
	#else
	memcpy(code->address, code->srcfun, code->codelen);
	#endif
}


//=====================================================================
// 方法四：Hook GetProcAddress然后再判断是否是需要 HOOK的东西。
//=====================================================================
static struct ApiHookFunction ApiHookGPA[2] = {
	{ "GetProcAddress", NULL, NULL },
	{ NULL, NULL, NULL },
};

// 初始化替换 GetProcAddress
// NewGetProcAddress 为 NULL时将还原本来的 GetProcAddress
int ApiHookSetGPA(ApiHookProcGPA NewGetProcAddress)
{
	int retval;

	if (ApiHookGPA[0].OrigFn != NULL) {
		ApiHookRemove("kernel32.dll", ApiHookGPA);
		ApiHookGPA[0].OrigFn = NULL;
		ApiHookGPA[0].HookFn = NULL;
	}

	if (NewGetProcAddress == NULL)
		return 0;

	ApiHookGPA[0].HookFn = (void*)NewGetProcAddress;
	ApiHookGPA[0].OrigFn = NULL;

	retval = ApiHookInstall("kernel32.dll", ApiHookGPA);

	if (retval != 0) {
		ApiHookGPA[0].OrigFn = NULL;
		ApiHookGPA[0].HookFn = NULL;
		return retval;
	}

	return 0;
}


// 调用原来的 GetProcAddress
FARPROC GetProcAddress_Orignal(HMODULE hModule, LPCSTR lpProcName)
{
	if (ApiHookGPA[0].OrigFn != NULL) {
		ApiHookProcGPA proc = (ApiHookProcGPA)ApiHookGPA[0].OrigFn;
		return proc(hModule, lpProcName);
	}
	return GetProcAddress(hModule, lpProcName);
}



//=====================================================================
// 高级接口
//=====================================================================

// 单独 HookIAT一个函数
void *ApiHookIAT(const char *DllName, const char *FunName, void *HookFn)
{
	struct ApiHookFunction hook[2];

	hook[0].Name = FunName;
	hook[0].HookFn = HookFn;
	hook[1].Name = NULL;
	hook[1].HookFn = NULL;
	hook[1].OrigFn = NULL;

	if (ApiHookInstall(DllName, hook) != 0)
		return NULL;

	return hook[0].OrigFn;
}


// HOOK COM接口，返回老接口
void *ApiHookCOM(void *comobj, int nFunIndex, void *NewFun)
{
	DWORD dwDummy;
	char *ptr;
	char *org;

	ptr = *((char**)comobj);

	VirtualProtect(ptr + nFunIndex * sizeof(FARPROC), 
		sizeof(FARPROC), PAGE_EXECUTE_READWRITE, &dwDummy);

	memcpy(&org, ptr + nFunIndex * sizeof(FARPROC), sizeof(FARPROC));
	memcpy(ptr + nFunIndex * sizeof(FARPROC), &NewFun, sizeof(FARPROC));

	VirtualProtect(ptr + nFunIndex * sizeof(FARPROC),
		sizeof(FARPROC), dwDummy, &dwDummy);

	return org;
}


// 设置调试权限
BOOL ApiHook_EnableDebugPrivilege(void)
{
	HANDLE hToken;
	HANDLE handle;
	BOOL OK = FALSE;
	handle = GetCurrentProcess();
	if (OpenProcessToken(handle, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, 
			&tp.Privileges[0].Luid)) {
			ApiHookLog("[ERROR] can not lookup privilege value");
		}
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), 
			NULL, NULL)) {
			ApiHookLog("[ERROR] can not adjust privilege value");
		}
		OK = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return OK;
}


int ApiHookInjectLibW(DWORD ProcessId, PCWSTR LibFile, HANDLE *th, int wait)
{
	LPTHREAD_START_ROUTINE ThreadRtn;
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	PWSTR LibFileRemote = NULL;
	int cch, cb;

	ApiHook_EnableDebugPrivilege();

	// 打开进程
	hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION | 
		PROCESS_VM_WRITE,
		FALSE, ProcessId);

	if (hProcess == NULL) 
		return -1;

	cch = 1 + lstrlenW(LibFile);
	cb = cch * sizeof(WCHAR);

	// 分配远程内存
	LibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, cb,  MEM_COMMIT, 
		PAGE_READWRITE);

	if (LibFileRemote == NULL) {
		CloseHandle(hProcess);
		return -2;
	}

	// 写入远程字符串
	if (!WriteProcessMemory(hProcess, LibFileRemote, 
		(PVOID)LibFile, cb, NULL)) {
		VirtualFreeEx(hProcess, LibFileRemote, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return -3;
	}

	// 开启远程线程
	ThreadRtn = (LPTHREAD_START_ROUTINE)GetProcAddress(
		GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryW");

	if (ThreadRtn == NULL) {
		VirtualFreeEx(hProcess, LibFileRemote, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return -4;
	}

	// 创建远程线程，调用 LoadLibraryW
	hThread = CreateRemoteThread(hProcess, NULL, 0, 
		ThreadRtn, LibFileRemote, 0, NULL);

	if (hThread == NULL) {
		VirtualFreeEx(hProcess, LibFileRemote, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return -5;
	}

	if (th) th[0] = hThread;

	// 是否等待远程线程结束
	if (wait) {
		WaitForSingleObject(hThread, INFINITE);
		if (th) th[0] = NULL;
	}
	
	return 0;
}


int ApiHookInjectLibA(DWORD ProcessId, PCSTR LibFile, HANDLE *th, int wait)
{
	PWSTR LibFileW;
	int retval;
	LibFileW = (PWSTR)malloc((lstrlenA(LibFile) + 1) * sizeof(WCHAR));
	if (LibFileW == NULL) return -100;
	wsprintfW(LibFileW, L"%S", LibFile);
	retval = ApiHookInjectLibW(ProcessId, LibFileW, th, wait);
	free(LibFileW);
	return retval;
}



//=====================================================================
// 特殊HOOK：综合方法Hook D3D8, D3D9, DDRAW, DINPUT
//=====================================================================
static CRITICAL_SECTION Lock_D3D8;
static CRITICAL_SECTION Lock_D3D9;

static void *Restore_D3D8 = NULL;
static void *Restore_D3D9 = NULL;

typedef void *(WINAPI *ApiHook_Direct3DCreate8_t)(UINT);
typedef void *(WINAPI *ApiHook_Direct3DCreate9_t)(UINT);

static ApiHook_Direct3DCreate8_t OrigDirect3DCreate8 = NULL;
static ApiHook_Direct3DCreate9_t OrigDirect3DCreate9 = NULL;

static AdapterDirect3DCreate8_t AdapterDirect3DCreate8 = NULL;
static AdapterDirect3DCreate9_t AdapterDirect3DCreate9 = NULL;


static void* WINAPI ApiHook_Direct3DCreate8_Imp(UINT SdkVersion)
{
	void *obj;

	EnterCriticalSection(&Lock_D3D8);
	
	ApiHookRetour((char*)OrigDirect3DCreate8, (char*)Restore_D3D8, 5);

	obj = OrigDirect3DCreate8(SdkVersion);

	Restore_D3D8 = ApiHookDetour((char*)OrigDirect3DCreate8, 
		(char*)ApiHook_Direct3DCreate8_Imp, 5);

	LeaveCriticalSection(&Lock_D3D8);

	if (AdapterDirect3DCreate8 != NULL) 
		return AdapterDirect3DCreate8(obj, SdkVersion);

	return obj;
}

static void* WINAPI ApiHook_Direct3DCreate9_Imp(UINT SdkVersion)
{
	void *obj;

	EnterCriticalSection(&Lock_D3D9);
	
	ApiHookRetour((char*)OrigDirect3DCreate9, (char*)Restore_D3D9, 5);

	obj = OrigDirect3DCreate9(SdkVersion);

	Restore_D3D9 = ApiHookDetour((char*)OrigDirect3DCreate9, 
		(char*)ApiHook_Direct3DCreate9_Imp, 5);

	LeaveCriticalSection(&Lock_D3D9);

	if (AdapterDirect3DCreate9 != NULL) 
		return AdapterDirect3DCreate9(obj, SdkVersion);

	return obj;
}

int ApiHook_D3D8_Set(AdapterDirect3DCreate8_t adapter)
{
	static HINSTANCE hDLL = NULL;

	if (hDLL == NULL) {
		hDLL = LoadLibraryA("d3d8.dll");
		if (hDLL == NULL) return -1;
	}

	if (OrigDirect3DCreate8 == NULL) {
		OrigDirect3DCreate8 = (ApiHook_Direct3DCreate8_t)
			GetProcAddress(hDLL, "Direct3DCreate8");
		if (OrigDirect3DCreate8 == NULL) return -2;
	}

	if (Restore_D3D8 != NULL) {
		ApiHookRetour((char*)OrigDirect3DCreate8, (char*)Restore_D3D8, 5);
		Restore_D3D8 = NULL;
		DeleteCriticalSection(&Lock_D3D8);
	}

	if (adapter != NULL) {
		AdapterDirect3DCreate8 = adapter;
		InitializeCriticalSection(&Lock_D3D8);
		Restore_D3D8 = ApiHookDetour((char*)OrigDirect3DCreate8, 
			(char*)ApiHook_Direct3DCreate8_Imp, 5);
	}

	return 0;
}

int ApiHook_D3D9_Set(AdapterDirect3DCreate9_t adapter)
{
	static HINSTANCE hDLL = NULL;

	if (hDLL == NULL) {
		hDLL = LoadLibraryA("d3d9.dll");
		if (hDLL == NULL) return -1;
	}

	if (OrigDirect3DCreate9 == NULL) {
		OrigDirect3DCreate9 = (ApiHook_Direct3DCreate9_t)
			GetProcAddress(hDLL, "Direct3DCreate9");
		if (OrigDirect3DCreate9 == NULL) return -2;
	}

	if (Restore_D3D9 != NULL) {
		ApiHookRetour((char*)OrigDirect3DCreate9, (char*)Restore_D3D9, 5);
		Restore_D3D9 = NULL;
		DeleteCriticalSection(&Lock_D3D9);
	}

	if (adapter != NULL) {
		AdapterDirect3DCreate9 = adapter;
		InitializeCriticalSection(&Lock_D3D9);
		Restore_D3D9 = ApiHookDetour((char*)OrigDirect3DCreate9, 
			(char*)ApiHook_Direct3DCreate9_Imp, 5);
	}

	return 0;
}


//=====================================================================
// 日志输出
//=====================================================================
static FILE *ApiHookLogFile = NULL;
static CRITICAL_SECTION Lock_Log;

void ApiHookLogSet(const char *fname)
{
	static int inited = 0;

	if (inited++ == 0) {
		InitializeCriticalSection(&Lock_Log);
	}

	if (ApiHookLogFile) fclose(ApiHookLogFile);
	ApiHookLogFile = fopen(fname, "a");
	if (ApiHookLogFile == NULL) return;
	fseek(ApiHookLogFile, 0, SEEK_END);
}

void ApiHookLog(const char *fmt, ...)
{
	static time_t tt_save = 0, tt_now = 0;
	static struct tm tm_time, *tmx = &tm_time;
	static char timetxt[64] = "";
	char logbuf[512] = {0};
	va_list va_alist;

	if (ApiHookLogFile == NULL)
		return;

	if (!fmt) { return; }

	EnterCriticalSection(&Lock_Log);

	// 更新时间
	tt_now = time(NULL);
	if (tt_now != tt_save) {
		tt_save = tt_now;
		memcpy(&tm_time, localtime(&tt_now), sizeof(tm_time));		
		sprintf(timetxt, "%04d-%02d-%02d %02d:%02d:%02d", tmx->tm_year + 1900, 
			tmx->tm_mon + 1, tmx->tm_mday, tmx->tm_hour, tmx->tm_min, tmx->tm_sec);
	}

	va_start (va_alist, fmt);
	_vsnprintf (logbuf, sizeof(logbuf), fmt, va_alist);
	va_end (va_alist);
		
	fprintf(ApiHookLogFile, "[%s] %s\n", timetxt, logbuf);
	fflush(ApiHookLogFile);

	LeaveCriticalSection(&Lock_Log);
}


