//=====================================================================
// 
// cprofile.h -- light weight profiling tool
//
// NOTE:
// for more information, please see the readme file
//
//=====================================================================

#ifndef __CPROFILE_H__
#define __CPROFILE_H__

//---------------------------------------------------------------------
// 预定义 INT64类型
//---------------------------------------------------------------------
#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif


//---------------------------------------------------------------------
// 计时节点：名称，开始时间，子节点等信息
//---------------------------------------------------------------------
typedef struct _CProfileNode
{
	const char *name;					// 名称
	IINT64 starttime;					// 开始时间
	IINT64 totaltime;					// 总计时间
	IINT64 childtime;					// 子孙时间
	int totalcall;						// 总计次数
	int reference;						// 引用次数
	const char *srcfile;				// 文件名称
	const char *funname;				// 函数名称
	int srcline;						// 文件行号
	struct _CProfileNode *parent;		// 当前节点
	struct _CProfileNode *child;		// 儿子节点
	struct _CProfileNode *sibling;		// 兄弟节点
}	CProfileNode;


//---------------------------------------------------------------------
// 计时节点：
//---------------------------------------------------------------------
typedef struct _CProfileManager
{
	CProfileNode *root;					// 开始节点
	CProfileNode *current;				// 当前节点
	CProfileNode *eparent;				// 当前父节点
	CProfileNode *echild;				// 当前子节点
	IINT64 resettime;					// 复位时间
	int framecount;						// 统计次数
}	CProfileManager;


#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
// 标准 C调用接口
//---------------------------------------------------------------------

// 开始一个名称为 name的时间统计
void cprofile_start(const char *name, const char *srcfile, int lineno);

// 结束上一个时间统计
void cprofile_stop(void);

// 复位统计器
void cprofile_reset(int needclean);

// 统计：取得各项数据
const char *cprofile_view(float *totaltime, int *times, float *parenttime);

// 浏览：回到根节点
void cprofile_root(void);

// 浏览：第一个节点
int cprofile_first(void);

// 浏览：下一个节点
int cprofile_next(void);

// 浏览：进入父节点
int cprofile_enter_parent(void);

// 浏览：进入子节点
int cprofile_enter_child(void);



//---------------------------------------------------------------------
// 结果显示
//---------------------------------------------------------------------
typedef struct CProfileResult
{
	const char *name;			// 评测名称
	int ncalls;					// 调用次数
	float tottime;				// 总共时间(包含子调用的时间)
	float per1;					// tottime / ncalls
	float pct1;					// tottime / 经过多久
	float cumtime;				// 总共时间(不包括子调用的时间)
	float per2;					// cumtime / ncalls
	float pct2;					// cumtime / 经过多久
	const char *srcfile;		// 源文件名称
	int srcline;				// 源文件行号
}	CProfileResult;


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
// recursive - 是否递归
CProfileResult *cprofile_result_get(const char *path, int *count, 
	int sort, int recursive);

// 释放结果
void cprofile_result_free(CProfileResult *result);


// 取得文本，用完后用 free释放，以换行符分割每行，path, sort等见上面说明
// rec = 0 不递归，只显示当前层 rec = 1 递归
char *cprofile_result_text(const char *path, int recursive);

// 将情况记录到日志文件, n (日志文件名)
void cprofile_result_log(const char *path, int recursive, const char *log);

// 将情况输出到标准输出
void cprofile_result_print(const char *path, int recursive);


// 设置风格
// format = 0 name ncall tottime percall cumtime percall info
// format = 1 name ncall tottime percent cumtime percent info
// format = 2 name ncall tottime percall percent cumtime percall percent
// format = 3 name ncall tottime percall percent cumtime percall percent info
// sort = CSORT_NONE / CSORT_TOTTIME / CSORT_CUMTIME
// hiprecision = 0   精确到 milisec 0.001s
// hiprecision = 1   精确到 usec    0.000001s
void cprofile_result_style(int format, int sort, int hiprecision);

// 记录全部评测
void cprofile_result_logall(const char *logfile);

// 打印全部评测
void cprofile_result_printall(void);


#define CSORT_NONE      (0)         // 不排序(记录顺序)
#define CSORT_NAME      (1)			// 按名称排序
#define CSORT_NCALLS    (-2)		// 按调用次数多少
#define CSORT_TOTTIME	(-3)		// 按总时间多少
#define CSORT_CUMTIME   (-6)		// 按相对时间多少



#ifdef __cplusplus
}
#endif


//---------------------------------------------------------------------
// 调试宏定义
//---------------------------------------------------------------------
#if defined(_DEBUG) || defined(DEBUG)
	#define CPROFILE_USED	1
#elif defined(PROFILE) || defined(_PROFILE)
	#define CPROFILE_USED	1
#else
	#define CPROFILE_USED	0
#endif


#define _CPROFILE_START(name, file, line) cprofile_start(name, file, line)
#define _CPROFILE_STOP() cprofile_stop()

#if CPROFILE_USED
	#define CPROFILE_START(name) _CPROFILE_START(name, __FILE__, __LINE__)
	#define CPROFILE_STOP() _CPROFILE_STOP()
#else
	#define CPROFILE_START(name)
	#define CPROFILE_STOP()
#endif


//---------------------------------------------------------------------
// C++ 接口
//---------------------------------------------------------------------

#ifdef __cplusplus

struct CProfileSample
{
	CProfileSample(const char *name, const char *fn = NULL, int line = 0) { 
		cprofile_start(name, fn, line); 
	}
	~CProfileSample() { 
		cprofile_stop(); 
	}
};

#define _CPROFILE(x)	CProfileSample __cprofile_x__(x, __FILE__, __LINE__)

#if CPROFILE_USED
	#define CPROFILE(x)		_CPROFILE(x)
#else
	#define CPROFILE(x)		
#endif

#endif


#endif


