//=====================================================================
// 
// cprofile.c -- light weight profiling tool
//
// NOTE:
// for more information, please see the readme file
//
//=====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "cprofile.h"

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#if ((!defined(_M_PPC)) && (!defined(_M_PPC_BE)) && (!defined(_XBOX)))
#include <mmsystem.h>
#define CHAVE_QPC
#define CHAVE_MMSYSTEM
#define CHAVE_CLOCK
#else
#define CHAVE_MMSYSTEM
#define CHAVE_CLOCK
#endif
#elif defined(__unix)
#include <unistd.h>
#include <sys/time.h>
#define CHAVE_TIMEOFDAY
#define CHAVE_CLOCK
#else
#define CHAVE_CLOCK
#endif


//---------------------------------------------------------------------
// inline 定义
//---------------------------------------------------------------------
#ifndef INLINE
#ifdef __GNUC__

#if __GNUC_MINOR__ >= 1  && __GNUC_MINOR__ < 4
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#ifndef inline
#define inline INLINE
#endif


//---------------------------------------------------------------------
// 平台判断
//---------------------------------------------------------------------
#if (defined(_WIN32) && !defined(WIN32))
	#define WIN32 _WIN32
#elif (defined(WIN32) && !defined(_WIN32))
	#define _WIN32 WIN32
#endif

#if (defined(_WIN32) && !defined(_MSC_VER))
	#ifndef __i386__
	#define __i386__
	#endif
#elif defined(_MSC_VER)
	#if (defined(_M_IX86) && !defined(__i386__))
	#define __i386__
	#endif
#endif

#ifndef __i386__
	#if (defined(__386__) || defined(__I386__) || _M_IX86)
	#define __i386__
	#endif
#endif

#if (defined(__i386__) && !defined(__I386__))
	#define __I386__
#endif

#if (defined(__unix)) && (!defined(CHAVE_QPC) && defined(__i386__))
	#define CHAVE_QPC
#endif


/*-------------------------------------------------------------------*/
/* C99 Compatible                                                    */
/*-------------------------------------------------------------------*/
#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#endif
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif

#ifdef _BSD_SOURCE
#undef _BSD_SOURCE
#endif

#ifdef __BSD_VISIBLE
#undef __BSD_VISIBLE
#endif

#define _GNU_SOURCE 1
#define _BSD_SOURCE 1
#define __BSD_VISIBLE 1


//=====================================================================
// 全局变量
//=====================================================================
CProfileManager *cprofile_manager = NULL;


//=====================================================================
// 系统时钟
//=====================================================================

#ifdef __i386__
// #define __I_HAVE_386__		// 笔记本上该指令有误
#endif

//---------------------------------------------------------------------
// 取得时钟
//---------------------------------------------------------------------
static inline void cprofile_ticks(IINT64 *ticks)
{
#if defined(CHAVE_QPC)
	#if defined(__GNUC__) && defined(__I_HAVE_386__)
		__asm__ __volatile__ ("\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"movl %0, %%ecx\n"
			".byte 0x0f\n"
			".byte 0x31\n"
			"movl %%eax, 0(%%ecx)\n"
			"movl %%edx, 4(%%ecx)\n"
			"popl %%ecx\n"
			"popl %%edx\n"
			:"=m"(ticks) :
			:"memory", "eax", "edx", "ecx");
	#elif defined(_MSC_VER) && defined(__I_HAVE_386__)
	__asm {
		push edx
		push ecx;
		mov ecx, ticks;
		_emit 0x0f;
		_emit 0x31;
		mov [ecx], eax;
		mov [ecx + 4], edx;
		pop ecx;
		pop edx;
	}
	#elif defined(_WIN32)
	LARGE_INTEGER lint;
	QueryPerformanceCounter(&lint);
	*ticks = lint.QuadPart;
	#else
	struct timeval tv;
	IINT64 value;
	gettimeofday(&tv, NULL);
	value = tv.tv_sec;
	value *= 1000000;
	value = (value * 1000000) + tv.tv_usec;
	*ticks = value;
	#endif

#elif defined(CHAVE_TIMEOFDAY)
	struct timeval tv;
	IINT64 value;
	gettimeofday(&tv, NULL);
	value = tv.tv_sec;
	value *= 1000000;
	value = (value * 1000000) + tv.tv_usec;
	*ticks = value;

#elif defined(CHAVE_MMSYSTEM)
	static unsigned long last = 0, high = 0;
	unsigned long current;
	IINT64 value;
	current = timeGetTime();
	if (current < last) high++;
	last = current;
	value = high;
	value <<= 32;
	value += current;
	*ticks = value;

#elif defined(CHAVE_CLOCK)
	static unsigned long last = 0, high = 0;
	unsigned long current;
	IINT64 value;
	current = clock();
	if (current < last) high++;
	last = current;
	value = high;
	value <<= 32;
	value += current;
	*ticks = value;
#endif
}



//---------------------------------------------------------------------
// 取得频率
//---------------------------------------------------------------------
static inline IINT64 _itimeofday(void)
{
	IINT64 sec, usec, val;
	#if defined(__unix)
	struct timeval time;
	gettimeofday(&time, NULL);
	sec = time.tv_sec;
	usec = time.tv_usec;
	#elif defined(_WIN32)
	static long mode = 0, addsec = 0;
	BOOL retval;
	static IINT64 freq = 1;
	IINT64 qpc;
	if (mode == 0) {
		retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	retval = retval * 2;
	sec = qpc / freq + addsec;
	usec = ((qpc % freq) * 1000000 / freq);
	#endif
	val = sec * 1000000 + usec;
	return val;
}


//---------------------------------------------------------------------
// 取得频率
//---------------------------------------------------------------------
static inline float cprofile_frequence(void)
{
#if defined(CHAVE_QPC)
	#if defined(_WIN32) && (!defined(__i386__))
	static IINT64 _cpurate = 0;
	static float _cpufreq = -1.0f;
	if (_cpurate == 0) {
		IINT64 currrate = 0;
		QueryPerformanceFrequency((LARGE_INTEGER*)&currrate);
		_cpurate = currrate;
		_cpufreq = (float)currrate;
	}
	return _cpufreq;
	#elif defined(_WIN32) && defined(__i386__)
	static IINT64 startup, current;
	static IINT64 t1, t2, dt1, dt2;
	static IINT64 _cpurate = 0;
	static float _cpufreq;
	if (_cpurate == 0) {
		startup = _itimeofday();
		startup = _itimeofday();
		startup = _itimeofday();
		cprofile_ticks(&t1);
		Sleep(1000);
	}
	current = _itimeofday();
	current = _itimeofday();
	current = _itimeofday();
	cprofile_ticks(&t2);
	dt1 = t2 - t1;
	dt2 = current - startup;
	_cpurate = dt1 * 1000000 / dt2;
	_cpufreq = (float)_cpurate;
	return _cpufreq;
	#endif

#elif defined(CHAVE_TIMEOFDAY)
	return 1000000;

#elif defined(CHAVE_MMSYSTEM)
	return 1000;

#elif defined(CHAVE_CLOCK)
	return CLK_TCK;

#endif
	return 1000;
}


//=====================================================================
// 节点管理
//=====================================================================

//---------------------------------------------------------------------
// 初始化节点
//---------------------------------------------------------------------
CProfileNode *cprofile_node_new(const char *name, CProfileNode *parent)
{
	CProfileNode *node;
	node = (CProfileNode*)malloc(sizeof(CProfileNode));
	if (node == NULL) return NULL;
	node->name = name;
	node->starttime = 0;
	node->totaltime = 0;
	node->childtime = 0;
	node->totalcall = 0;
	node->reference = 0;
	node->srcfile = "";
	node->funname = "";
	node->srcline = 0;
	node->parent = parent;
	node->child = NULL;
	node->sibling = NULL;
	return node;
}


//---------------------------------------------------------------------
// 删除节点
//---------------------------------------------------------------------
void cprofile_node_delete(CProfileNode *node)
{
	if (node->sibling) {
		cprofile_node_delete(node->sibling);
		node->sibling = NULL;
	}
	if (node->child) {
		cprofile_node_delete(node->child);
		node->child = NULL;
	}
	node->name = NULL;
	free(node);
}


//---------------------------------------------------------------------
// 复位节点
//---------------------------------------------------------------------
void cprofile_node_reset(CProfileNode *node)
{
	node->starttime = 0;
	node->totaltime = 0;
	node->childtime = 0;
	node->totalcall = 0;
	node->reference = 0;

	if (node->child) {
		cprofile_node_reset(node->child);
	}

	if (node->sibling) {
		cprofile_node_reset(node->sibling);
	}
}


//---------------------------------------------------------------------
// 节点开始计时
//---------------------------------------------------------------------
void cprofile_node_start(CProfileNode *node)
{
	node->totalcall++;
	if (node->reference++ == 0) {
		cprofile_ticks(&node->starttime);
	}
}


//---------------------------------------------------------------------
// 节点结束计时
//---------------------------------------------------------------------
int cprofile_node_stop(CProfileNode *node)
{
	if (--node->reference == 0) {
		IINT64 current;
		cprofile_ticks(&current);
		node->totaltime += current - node->starttime;
		if (node->parent) {
			node->parent->childtime += current - node->starttime;
		}
	}
	return (node->reference == 0) ? 1 : 0;
}


//---------------------------------------------------------------------
// 节点取子节点
//---------------------------------------------------------------------
CProfileNode *cprofile_node_getsub(CProfileNode *node, const char *name)
{
	CProfileNode *child;

	for (child = node->child; child != NULL; child = child->sibling) {
		if (child->name == name) 
			return child;
	}
	
	child = cprofile_node_new(name, node);
	if (child == NULL) return NULL;

	child->sibling = node->child;
	node->child = child;

	return child;
}


//=====================================================================
// 评测管理
//=====================================================================

//---------------------------------------------------------------------
// 删除评测
//---------------------------------------------------------------------
void cprofile_manager_delete(CProfileManager *manager)
{
	if (manager) {
		if (manager->root) {
			cprofile_node_delete(manager->root);
		}
		free(manager);
	}
}


//---------------------------------------------------------------------
// 创建评测
//---------------------------------------------------------------------
CProfileManager *cprofile_manager_new(void)
{
	CProfileManager *manager;

	manager = (CProfileManager*)malloc(sizeof(CProfileManager));
	if (manager == NULL) return NULL;

	manager->root = cprofile_node_new("__CROOT__", NULL);

	if (manager->root == NULL) {
		cprofile_manager_delete(manager);
		return NULL;
	}

	manager->current = manager->root;
	manager->eparent = NULL;
	manager->echild = NULL;
	manager->resettime = 0;
	manager->framecount = 0;

	cprofile_ticks(&manager->resettime);
	manager->root->starttime = manager->resettime;

	return manager;
}


//---------------------------------------------------------------------
// 开始评测
//---------------------------------------------------------------------
void cprofile_manager_start(CProfileManager *manager, const char *name)
{
	if (name != manager->current->name) {
		manager->current = cprofile_node_getsub(manager->current, name);
		assert(manager->current);
	}
	cprofile_node_start(manager->current);
}


//---------------------------------------------------------------------
// 离开评测
//---------------------------------------------------------------------
void cprofile_manager_stop(CProfileManager *manager)
{
	IINT64 totaltime, delta;
	totaltime = manager->current->totaltime;
	cprofile_node_stop(manager->current);
	delta = manager->current->totaltime - totaltime;
	if (manager->current->parent) {
		manager->current = manager->current->parent;
	}
	if (manager->current == manager->root) {
		manager->root->totaltime += delta;
	}
}


//---------------------------------------------------------------------
// 复位评测
//---------------------------------------------------------------------
void cprofile_manager_reset(CProfileManager *manager)
{
	cprofile_node_reset(manager->root);
	manager->framecount = 0;
	cprofile_ticks(&manager->resettime);
}


//---------------------------------------------------------------------
// 增加帧数
//---------------------------------------------------------------------
void cprofile_manager_incframe(CProfileManager *manager)
{
	manager->framecount++;
}

//---------------------------------------------------------------------
// 离开评测
//---------------------------------------------------------------------
float cprofile_manager_lifetime(CProfileManager *manager)
{
	IINT64 current;
	cprofile_ticks(&current);
	current -= manager->resettime;
	return (float)current / cprofile_frequence();
}


//---------------------------------------------------------------------
// 浏览：第一个节点
//---------------------------------------------------------------------
int cprofile_manager_first(CProfileManager *manager)
{
	if (manager->eparent == NULL) {
		manager->eparent = manager->root;
	}
	manager->echild = manager->eparent->child;
	if (manager->echild == NULL) return -1;

	return 0;
}


//---------------------------------------------------------------------
// 浏览：下一个节点
//---------------------------------------------------------------------
int cprofile_manager_next(CProfileManager *manager)
{
	if (manager->echild == NULL) return -1;
	if (manager->echild->sibling == NULL) return -2;
	manager->echild = manager->echild->sibling;
	return 0;
}

//---------------------------------------------------------------------
// 浏览：返回父节点
//---------------------------------------------------------------------
int cprofile_manager_enter_parent(CProfileManager *manager)
{
	if (manager->eparent->parent == NULL) 
		return -1;
	manager->echild = manager->eparent;
	manager->eparent = manager->eparent->parent;
	return 0;
}

//---------------------------------------------------------------------
// 浏览：进入子节点
//---------------------------------------------------------------------
int cprofile_manager_enter_child(CProfileManager *manager)
{
	if (manager->echild == NULL) {
		return -1;
	}
	manager->eparent = manager->echild;
	manager->echild = manager->eparent->child;
	return 0;
}

//---------------------------------------------------------------------
// 进入根节点
//---------------------------------------------------------------------
int cprofile_manager_enter_root(CProfileManager *manager)
{
	manager->eparent = manager->root;
	manager->echild = manager->root->child;
	if (manager->echild == NULL) 
		return -1;
	return 0;
}

//---------------------------------------------------------------------
// 显示资料
//---------------------------------------------------------------------
const char *cprofile_manager_view(CProfileManager *manager,
	float *totaltime, int *times, float *parenttime)
{
	CProfileNode *node;
	float _parenttime;
	float _totaltime;

	if (manager->echild == NULL) {
		return NULL;
	}

	node = manager->echild;

	_parenttime = (node->parent)? node->parent->totaltime : 0.0f;
	_totaltime = (float)node->totaltime;

	if (totaltime) *totaltime = _totaltime / cprofile_frequence();
	if (times) *times = node->totalcall;
	if (parenttime && node->parent) 
		*parenttime = _parenttime / cprofile_frequence();

	return node->name;
}


//---------------------------------------------------------------------
// 初始化计时器
//---------------------------------------------------------------------
void cprofile_manager_init(void)
{
	if (cprofile_manager == NULL) {
		cprofile_manager = cprofile_manager_new();
		assert(cprofile_manager);
	}
}


//=====================================================================
// 标准C接口
//=====================================================================

//---------------------------------------------------------------------
// 开始一个名称为 name的时间统计
//---------------------------------------------------------------------
void cprofile_start(const char *name, const char *srcname, int lineno)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	cprofile_manager_start(cprofile_manager, name);
	cprofile_manager->current->srcfile = srcname;
	cprofile_manager->current->srcline = lineno;
}

//---------------------------------------------------------------------
// 结束上一个时间统计
//---------------------------------------------------------------------
void cprofile_stop(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	cprofile_manager_stop(cprofile_manager);
}

//---------------------------------------------------------------------
// 复位统计器
//---------------------------------------------------------------------
void cprofile_reset(int needclean)
{
	if (needclean) {
		cprofile_manager_delete(cprofile_manager);
		cprofile_manager = NULL;
	}
	if (cprofile_manager == NULL) { 
		cprofile_manager_init();
	}
	cprofile_manager_reset(cprofile_manager);
}

//---------------------------------------------------------------------
// 统计：取得各项数据
//---------------------------------------------------------------------
const char *cprofile_view(float *totaltime, int *times, float *parenttime)
{
	const char *name = NULL;
	if (cprofile_manager == NULL) cprofile_manager_init();
	name = cprofile_manager_view(cprofile_manager, totaltime, times,
		parenttime);
	return name;
}

//---------------------------------------------------------------------
// 浏览：回到根节点
//---------------------------------------------------------------------
void cprofile_root(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	cprofile_manager->eparent = cprofile_manager->root;
	cprofile_manager->echild = cprofile_manager->root->child;
}

//---------------------------------------------------------------------
// 浏览：第一个节点
//---------------------------------------------------------------------
int cprofile_first(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	return cprofile_manager_first(cprofile_manager);
}

//---------------------------------------------------------------------
// 浏览：下一个节点
//---------------------------------------------------------------------
int cprofile_next(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	return cprofile_manager_next(cprofile_manager);
}

//---------------------------------------------------------------------
// 浏览：进入父节点
//---------------------------------------------------------------------
int cprofile_enter_parent(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	return cprofile_manager_enter_parent(cprofile_manager);
}

//---------------------------------------------------------------------
// 浏览：进入子节点
//---------------------------------------------------------------------
int cprofile_enter_child(void)
{
	if (cprofile_manager == NULL) cprofile_manager_init();
	return cprofile_manager_enter_child(cprofile_manager);
}



//=====================================================================
// 结果显示
//=====================================================================
#ifdef _MSC_VER
	#if _MSC_VER >= 1300
		#pragma warning(disable:4996)
		#pragma warning(disable:4267)
	#endif
#endif

//---------------------------------------------------------------------
// 进入路径
//---------------------------------------------------------------------
int cprofile_manager_chdir(CProfileManager *manager, const char *path)
{
	static char line[1025];
	CProfileNode *root, *child;
	char *ptr;
	int size;
	int retval;

	if (!path) return -100;

	strncpy(line, path, 1024);
	size = (int)strlen(line);

	for (size = (int)strlen(line); size > 1; size--) {
		if (line[size - 1] != ' ' && line[size - 1] != '\t') break;
	}

	if (size == 0) return -1;
	line[size] = 0;

	for (ptr = line; ptr[0]; ptr++) {
		if (ptr[0] != ' ' && ptr[0] != '\t') break;
	}

	memmove(line, ptr, size - (ptr - line) + 1);
	size = (int)strlen(line);
	if (size == 0) return -2;
	if (line[0] != '/') return -3;

	//printf("PATH '%s'\n", line);

	root = manager->root;
	child = manager->root->child;

	for (ptr = line, retval = 0; ptr[0]; ) {
		static char name[1025];
		char *pp;

		if (ptr[0] != '/') { 
			retval = -5; 
			break; 
		}

		for (pp = name, ptr = ptr + 1; ptr[0] != '/' && ptr[0]; ) {
			*pp++ = *ptr++;
		}

		*pp++ = '\0';

		if (strlen(name) == 0) break;

		//printf("NAME '%s'\n", name);

		for (child = root->child; child != NULL; child = child->sibling) {
			if (strcmp(child->name, name) == 0) break;
		}

		if (child == 0) { retval = -10; break; }

		root = child;
		child = root->child;
	}

	if (retval == 0) {
		manager->eparent = root;
		manager->echild = child;
	}

	return retval;
}

//---------------------------------------------------------------------
// 取得结果
// manager - 评测
// path  - 路径
// count - 结果数量
//---------------------------------------------------------------------
CProfileResult *cprofile_manager_result_ex(CProfileManager *manager, 
	const char *path, int *count)
{
	CProfileResult *result;
	CProfileNode *root, *child;
	int retval, n, i;
	float freq, total;

	retval = cprofile_manager_chdir(manager, path);
	if (count) count[0] = 0;
	if (retval != 0) return NULL;

	root = manager->eparent;

	for (child = root->child, n = 0; child; child = child->sibling) n++;

	if (n == 0) return NULL;

	result = (CProfileResult*)malloc(sizeof(CProfileResult) * (n + 1));
	if (result == NULL) return NULL;

	freq = cprofile_frequence();
	total = manager->root->childtime / freq;

	for (child = root->child, i = n - 1; child; i--) {
		result[i].name = child->name;
		result[i].ncalls = child->totalcall;
		result[i].tottime = (float)child->totaltime / freq;
		result[i].cumtime = (child->totaltime - child->childtime) / freq;
		result[i].per1 = -1;
		result[i].per2 = -1;
		if (result[i].ncalls > 0) {
			result[i].per1 = result[i].tottime / result[i].ncalls;
			result[i].per2 = result[i].cumtime / result[i].ncalls;
		}
		result[i].pct1 = -1;
		result[i].pct2 = -1;
		if (total > 0.0f) {
			result[i].pct1 = result[i].tottime / total;
			result[i].pct2 = result[i].cumtime / total;
		}
		result[i].srcfile = child->srcfile;
		result[i].srcline = child->srcline;
		child = child->sibling;
	}

	result[n].name = NULL;
	result[n].srcfile = NULL;
	result[n].srcline = -1;

	if (count) count[0] = n;

	return result;
}

//---------------------------------------------------------------------
// cprofile_manager_result
//---------------------------------------------------------------------
CProfileResult *cprofile_manager_result(CProfileManager *manager, 
	const char *path, int *count, int recursive)
{
	CProfileResult *result, *record;
	int ncnt, n, i;
	char *basepath;

	result = cprofile_manager_result_ex(manager, path, &ncnt);
	if (recursive == 0 || result == 0) {
		if (count) count[0] = ncnt;
		return result;
	}

	basepath = (char*)malloc(1025);
	if (basepath == NULL) {
		if (count) count[0] = ncnt;
		return result;
	}

	for (i = ncnt - 1; i >= 0; i--) {
		strncpy(basepath, path, 1024);
		n = (int)strlen(basepath);
		if (n > 0) {
			if (basepath[n - 1] != '/') {
				basepath[n] = '/';
				basepath[n + 1] = '\0';
			}
		}
		strncat(basepath, result[i].name, 1024);

		record = cprofile_manager_result(manager, basepath, &n, 1);

		if (record != NULL) {
			CProfileResult *tmp;
			int nsize;
			nsize = ncnt + n + 1;
			tmp = (CProfileResult*)malloc(sizeof(CProfileResult) * nsize);

			if (tmp) {
				memcpy(tmp, result, sizeof(CProfileResult) * ncnt);
				memcpy(tmp + ncnt, record, sizeof(CProfileResult) * n);
				free(result);
				result = tmp;
				ncnt = ncnt + n;
				result[ncnt].name = NULL;
				result[ncnt].srcfile = NULL;
			}
			free(record);
		}
	}

	if (count) count[0] = ncnt;
	free(basepath);

	return result;
}

//---------------------------------------------------------------------
// 取得结果
// path  - 路径
// count - 结果数量
// sort  - 排序方式 0不排序，其他：
//         sort = 1 (按名称排序)     sort = -1 (名称倒序)
//         sort = 2 (按ncalls排序)   sort = -2 (ncalls倒序)
//         sort = 3 (按tottime排序)  sort = -3 (tottime倒序)
//         sort = 4 (按per1排序)     sort = -4 (per1倒序)
//         sort = 5 (按pct1排序)     sort = -5 (pct1倒序)
//         sort = 6 (按cumtime排序)  sort = -6 (percent倒序)
//         sort = 7 (按per2排序)     sort = -7 (per2倒序)
//         sort = 8 (按pct2排序)     sort = -8 (pct2倒序)
//---------------------------------------------------------------------
void cprofile_result_sort(CProfileResult *result, int sort)
{
	int count, i, j;

	for (count = 0; result[count].name != NULL; count++);

	#define CPROFILE_CMP(x, y) ( ((x)==(y))? (0): ( ((x)>(y))? (1):(-1) ) )

	if ((sort >= 1 && sort <= 8) || (sort <= -1 && sort >= -8)) {
		CProfileResult *r1, *r2, tmp;
		int mode = (sort > 0) ? (sort) : (-sort);
		for (j = 0; j < count - 1; j++) {
			r1 = &result[j];
			for (i = j + 1; i < count; i++) {
				int k = 0;
				r2 = &result[i];
				switch (mode)
				{
				case 1: k = strcmp(r1->name, r2->name); break;
				case 2: k = CPROFILE_CMP(r1->ncalls, r2->ncalls); break;
				case 3: k = CPROFILE_CMP(r1->tottime, r2->tottime); break;
				case 4: k = CPROFILE_CMP(r1->per1, r2->per1); break;
				case 5: k = CPROFILE_CMP(r1->pct1, r2->pct1); break;
				case 6: k = CPROFILE_CMP(r1->cumtime, r2->cumtime); break;
				case 7: k = CPROFILE_CMP(r1->per2, r2->per2); break;
				case 8: k = CPROFILE_CMP(r1->pct2, r2->pct2); break;
				}
				if (sort < 0) k = -k;
				if (k > 0) {
					tmp = *r2;
					*r2 = *r1;
					*r1 = tmp;
				}
			}
		}
	}
	#undef CPROFILE_CMP
}


//---------------------------------------------------------------------
// 取得文本
//---------------------------------------------------------------------
char *cprofile_result_repr(CProfileResult *result, int mask, int floatsize)
{
	static char line[1025];
	static char name[64];
	static char ncalls[64];
	static char tottime[64];
	static char per1[64];
	static char pct1[64];
	static char cumtime[64];
	static char per2[64];
	static char pct2[64];
	static char desc[256];
	static char fmt1[8];
	static char fmt2[8];
	char *text;
	int size, block, count, i;
	int m1, m2, m3, m4, m5, m6, m7, m8, m9;

	if (result == NULL) return NULL;

	for (count = 0; result[count].name != NULL; count++);

	text = (char*)malloc(1025);
	if (text == NULL) {
		free(result);
		return NULL;
	}

	text[0] = '\0';
	size = 0;
	block = 1024;

	#define cprofile_append(ptr) do { \
		int len = strlen(ptr) + size; \
		int nblock = (len + 255) & (~255); \
		char *newtext = text; \
		if (nblock > block) { \
			newtext = (char*)malloc(nblock + 1); \
			if (newtext != NULL) { \
				strcpy(newtext, text); \
				free(text);	\
				text = newtext; \
				block = nblock; \
			} \
		}	\
		if (newtext) { \
			memcpy(text + size, ptr, len - size + 1); \
			size = len; \
		}	\
	}	while (0)
	
	#define cprofile_desc(i) do { \
		sprintf(name, "%s", result[i].name);	\
		sprintf(ncalls, "%d", result[i].ncalls); \
		sprintf(tottime, fmt1, result[i].tottime); \
		sprintf(per1, fmt1, result[i].per1); \
		sprintf(pct1, fmt2, result[i].pct1 * 100); \
		sprintf(cumtime, fmt1, result[i].cumtime); \
		sprintf(per2, fmt1, result[i].per2); \
		sprintf(pct2, fmt2, result[i].pct2 * 100); \
		sprintf(desc, "%s:%d", result[i].srcfile, result[i].srcline); \
	}	while (0)
	
	#define cprofile_strjust(text, n, isleft) do { \
		int len, d, i;	\
		len = strlen(text); \
		d = n - len;	\
		if (isleft == 0 && d > 0) { \
			memmove(text + d, text, len + 1); \
			for (i = 0; i < d; i++) text[i] = ' '; \
		}	else if (isleft && d > 0) { \
			for (i = 0; i < d; i++) text[len++] = ' '; \
			text[len] = '\0'; \
		}	\
		len = strlen(text); \
		text[len] = ' '; \
		text[len + 1] = '\0'; \
	}	while (0)
	
	
	m1 = m2 = m5 = m8 = 9;
	m3 = m4 = m6 = m7 = 7;
	m2 = 6; m1 = 4; m5 = m8 = 7; m9 = 14;

	sprintf(fmt1, "%%.%df", floatsize);
	if (floatsize <= 3) strncpy(fmt2, "%.0f%%", 8);
	else strncpy(fmt2, "%.2f%%", 8);
	strncpy(fmt2, "%.2f%%", 8);

	for (i = 0; i < count; i++) {
		int x1, x2, x3, x4, x5, x6, x7, x8, x9;
		cprofile_desc(i);
		x1 = (int)strlen(name);
		x2 = (int)strlen(ncalls);
		x3 = (int)strlen(tottime);
		x4 = (int)strlen(per1);
		x5 = (int)strlen(pct1);
		x6 = (int)strlen(cumtime);
		x7 = (int)strlen(per2);
		x8 = (int)strlen(pct2);
		x9 = (int)strlen(desc);
		m1 = x1 > m1 ? x1 : m1;
		m2 = x2 > m2 ? x2 : m2;
		m3 = x3 > m3 ? x3 : m3;
		m4 = x4 > m4 ? x4 : m4;
		m5 = x5 > m5 ? x5 : m5;
		m6 = x6 > m6 ? x6 : m6;
		m7 = x7 > m7 ? x7 : m7;
		m8 = x8 > m8 ? x8 : m8;
		m9 = x9 > m9 ? x9 : m9;
	}

	#define cprofile_write(mask, t1, t2, t3, t4, t5, t6, t7, t8, t9) do { \
		cprofile_strjust(t1, m1, 1);	\
		cprofile_strjust(t2, m2, 0);	\
		cprofile_strjust(t3, m3, 0);	\
		cprofile_strjust(t4, m4, 0);	\
		cprofile_strjust(t5, m5, 0);	\
		cprofile_strjust(t6, m6, 0);	\
		cprofile_strjust(t7, m7, 0);	\
		cprofile_strjust(t8, m8, 0);	\
		line[0] = '\0';	\
		if (mask & 1) { strcat(line, t1); strcat(line, ""); } \
		if (mask & 2) { strcat(line, t2); strcat(line, ""); } \
		if (mask & 4) strcat(line, t3); \
		if (mask & 8) strcat(line, t4); \
		if (mask & 16) { strcat(line, t5); strcat(line, ""); } \
		if (mask & 32) strcat(line, t6); \
		if (mask & 64) strcat(line, t7); \
		if (mask & 128) { strcat(line, t8); strcat(line, ""); } \
		if (mask & 256) strcat(line, t9); \
		strcat(line, "\n"); \
		cprofile_append(line); \
	}	while (0)
	
	#define cprofile_fill(text, size) do { \
		int i; \
		for (i = 0; i < size; i++) text[i] = '-'; \
		text[size] = '\0'; \
	}	while (0);

	// name  ncalls tottime  per1 pct1 cumtime per2 pct2 filename:line
	strncpy(name, "name", 24);			// 1
	strncpy(ncalls, "ncalls", 24);		// 2
	strncpy(tottime, "tottime", 24);	// 4
	strncpy(per1, "percall", 24);		// 8
	strncpy(pct1, "percent", 24);		// 16
	strncpy(cumtime, "cumtime", 24);	// 32
	strncpy(per2, "percall", 24);		// 64
	strncpy(pct2, "percent", 24);		// 128
	strncpy(desc, "filename:line", 24);	// 256

	cprofile_write(mask, name, ncalls, tottime, per1, pct1, 
		cumtime, per2, pct2, desc);

	cprofile_fill(name, m1);
	cprofile_fill(ncalls, m2);
	cprofile_fill(tottime, m3);
	cprofile_fill(per1, m4);
	cprofile_fill(pct1, m5);
	cprofile_fill(cumtime, m6);
	cprofile_fill(per2, m7);
	cprofile_fill(pct2, m8);
	cprofile_fill(desc, m9);

	#if 0
	cprofile_write(mask, name, ncalls, tottime, per1, pct1,
		cumtime, per2, pct2, desc);
	#endif

	for (i = 0; i < count; i++) {
		cprofile_desc(i);
		cprofile_write(mask, name, ncalls, tottime, per1, pct1, 
			cumtime, per2, pct2, desc);
	}
	
	#undef cprofile_write
	#undef cprofile_desc
	#undef cprofile_strjust
	#undef cprofile_append
	
	return text;
}


//---------------------------------------------------------------------
// 结果全局
//---------------------------------------------------------------------
static int cprofile_result_format = 1;
static int cprofile_result_sortcol = 0;
static int cprofile_result_float = 3;

//---------------------------------------------------------------------
// 取得结果
//---------------------------------------------------------------------
CProfileResult *cprofile_result_get(const char *path, int *count, 
	int sort, int recursion)
{
	CProfileResult *result;
	if (cprofile_manager == NULL) cprofile_manager_init();
	result = cprofile_manager_result(cprofile_manager, path, count, 
		recursion);
	if (result) {
		cprofile_result_sort(result, sort);
	}
	return result;
}

//---------------------------------------------------------------------
// 释放结果
//---------------------------------------------------------------------
void cprofile_result_free(CProfileResult *result)
{
	assert(result);
	free(result);
}

//---------------------------------------------------------------------
// 显示模式
//---------------------------------------------------------------------
void cprofile_result_style(int fmt, int sort, int hiprecision)
{
	cprofile_result_format = fmt;
	cprofile_result_sortcol = sort;
	cprofile_result_float = hiprecision ? 6 : 3;
}

//---------------------------------------------------------------------
// 取得文本，用完后用 free释放，以换行符分割每行，path, sort见上面说明
//---------------------------------------------------------------------
char *cprofile_result_text(const char *path, int recursive)
{
	CProfileResult *data;
	char *text;
	int format;
	int mask;

	data = cprofile_result_get(path, NULL, 
		cprofile_result_sortcol, recursive);

	if (data == NULL)
		return NULL;

	format = cprofile_result_format;

	if (format == 0) mask = 0x016f;			// tottime percall
	else if (format == 1) mask = 0x01b7;	// tottime percent
	else if (format == 2) mask = 0x00ff; 
	else mask = 0x01ff;

	text = cprofile_result_repr(data, mask, cprofile_result_float);
	free(data);

	return text;
}

//---------------------------------------------------------------------
// 列名称
//---------------------------------------------------------------------
const char *cprofile_column[] = {
	"default", "name", "ncalls", 
	"tottime", "percall:tottime/ncalls", "percent:tottime/last",
	"cumtime", "percall:cumtime/ncalls", "percent:cumtime/last",
	""
};


//---------------------------------------------------------------------
// 将情况记录到日志文件
//---------------------------------------------------------------------
void cprofile_result_log(const char *path, int rec, const char *logfile)
{
	struct tm tm_time, *tmx = &tm_time;
	time_t ttnow;
	char *text;
	char date[32];
	FILE *fp;
	int sort;
	const char *sdesc;

	text = cprofile_result_text(path, rec);
	if (text == NULL) return;

	if ((fp = fopen(logfile, "a")) == NULL) {
		free(text);
		return;
	}

	fseek(fp, 0, SEEK_END);

	ttnow = time(NULL);
	memcpy(&tm_time, localtime(&ttnow), sizeof(tm_time));
	sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", tmx->tm_year + 1900, 
			tmx->tm_mon + 1, tmx->tm_mday, tmx->tm_hour, 
			tmx->tm_min, tmx->tm_sec);

	sort = cprofile_result_sortcol;

	if (sort >= 0 && sort <= 8) sdesc = cprofile_column[sort];
	else if (sort < 0 && sort >= -8) sdesc = cprofile_column[-sort];
	else sdesc = "default";

	fprintf(fp, "+ %s\t: %s  ", date, path);

	if (sort <= 0) fprintf(fp, "(ordered by: %s)\n", sdesc);
	else fprintf(fp, "(ordered by: %s reversed)\n", sdesc);

	fputs(text, fp);
	fprintf(fp, "\n");
	fclose(fp);

	free(text);
}

//---------------------------------------------------------------------
// 将情况输出到标准输出
//---------------------------------------------------------------------
void cprofile_result_print(const char *path, int rec)
{
	struct tm tm_time, *tmx = &tm_time;
	time_t ttnow;
	char date[32];
	char *text;
	int sort;
	const char *sdesc;

	text = cprofile_result_text(path, rec);
	if (text == NULL) {
		printf("cprofile_result_print: invalid path: %s\n", path);
		return;
	}

	ttnow = time(NULL);
	memcpy(&tm_time, localtime(&ttnow), sizeof(tm_time));
	sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", tmx->tm_year + 1900, 
			tmx->tm_mon + 1, tmx->tm_mday, tmx->tm_hour, 
			tmx->tm_min, tmx->tm_sec);

	sort = cprofile_result_sortcol;

	if (sort >= 0 && sort <= 8) sdesc = cprofile_column[sort];
	else if (sort < 0 && sort >= -8) sdesc = cprofile_column[-sort];
	else sdesc = "default";

	fprintf(stdout, "+(%s)\t%s  ", date, path);

	if (sort <= 0) fprintf(stdout, "(ordered by: %s)\n", sdesc);
	else fprintf(stdout, "(ordered by: %s reversed)\n", sdesc);

	fputs(text, stdout);
	fprintf(stdout, "\n");
	fflush(stdout);

	free(text);
}

//---------------------------------------------------------------------
// 记录全部评测
//---------------------------------------------------------------------
void cprofile_result_logall(const char *logfile)
{
	cprofile_result_log("/", 1, logfile);
}

//---------------------------------------------------------------------
// 打印全部评测
//---------------------------------------------------------------------
void cprofile_result_printall(void)
{
	cprofile_result_print("/", 1);
}



