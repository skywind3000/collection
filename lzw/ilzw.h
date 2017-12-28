#ifndef __ILZW_H__
#define __ILZW_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


//---------------------------------------------------------------------
// GLOBAL
//---------------------------------------------------------------------
#ifndef ILZW_MAXBITS
#define ILZW_MAXBITS	12
#endif

#if ILZW_MAXBITS > 15
#undef ILZW_MAXBITS
#define ILZW_MAXBITS	15
#endif

#define ILZW_TABSIZE	(1L << ILZW_MAXBITS)
#define ILZW_MASK		(ILZW_TABSIZE - 1)


//---------------------------------------------------------------------
// LZW Descriptor
//---------------------------------------------------------------------
struct ILZWDESC
{
	unsigned char *inptr;		// IO - input ptr
	unsigned char *outptr;		// IO - output ptr
	int inbits;					// IO - input cache bits
	int indata;					// IO - input cache data
	int outbits;				// IO - output cache bits
	int outdata;				// IO - output cache data
	size_t inpos;				// IO - input pointer
	size_t inmax;				// IO - input pointer limit
	size_t outpos;				// IO - output pointer
	size_t outmax;				// IO - output pointer limit
	int error;					// IO - error
	int mode;					// lzw - compress or decompress
	int empty_string;			// lzw - empty string pointer 
	int curr_bit_size;			// lzw - current bit size
	int bit_overflow;			// lzw - is table size greater than limit
	int bit_pos;				// lzw - loading bit position
	int data_pos;				// lzw - loading data position
	int data_len;				// lzw - loading data length
	int entire;					// lzw - loading data entire
	int code;					// lzw - code loaded
	int cc;						// lzw - table size 
	int string_length;			// lzw - string length
	int bit_size;				// lzw - bit size
	short hash[ILZW_TABSIZE];	// lzw - string hash entry
	short next[ILZW_TABSIZE];	// lzw - string hash next
	unsigned char string[ILZW_TABSIZE];		// lzw - decode string
	unsigned char newc[ILZW_TABSIZE];		// lzw - new char
	unsigned short table[ILZW_TABSIZE * 2];	// lzw - table base
};

typedef struct ILZWDESC iLzwDesc;


#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
// LZW 压缩算法精简实现4K表格(41K内存)
//---------------------------------------------------------------------
#define ILZW_MODE_COMPRESS		0
#define ILZW_MODE_DECOMPRESS	1

#define ILZW_BITS_DEFAULT	8
#define ILZW_BITS_7			7
#define ILZW_BITS_6			6
#define ILZW_BITS_5			5
#define ILZW_BITS_4			4
#define ILZW_BITS_3			3
#define ILZW_BITS_2			2
#define ILZW_BITS_1			1


// LZW 初始化 
// mode     - 用途，ILZW_MODE_COMPRESS 或者ILZW_MODE_DECOMPRESS
// databits - 数据位数，默认是8(ILZW_BITS_DEFAULT), 
//            如果数据都是英文字母，可以设置成 7位，以此类推
void ilzw_init(iLzwDesc *lzw, int mode, int databits);

// 压缩以后最大会占用多少内存
#define ilzw_deflate_maxsize(size) ((((size) + 2) * ILZW_MAXBITS) / 8 + 10)


// LZW 压缩帧（不复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_deflate(iLzwDesc *lzw, const void *in, long insize, 
	void *out, long outsize);

// LZW 解压帧（不复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_inflate(iLzwDesc *lzw, const void *in, long insize, 
	void *out, long outsize);

// LZW 复位字典
void ilzw_reset(iLzwDesc *lzw);


// LZW 低层次压缩（每次复位字典，线程不安全）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// workmem  - 外部提供大小为 sizeof(workmem)的工作内存
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_compress_1(const void *in, long insize, void *out, long outsize,
	void *workmem);


// LZW 简易解压（每次复位字典，线程安全）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// workmem  - 外部提供大小为 sizeof(workmem)的工作内存
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_decompress_1(const void *in, long insize, void *out, long outsize,
	void *workmem);


// LZW 简易压缩（每次复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_compress(const void *in, long insize, void *out, long outsize);


// LZW 简易解压（每次复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_decompress(const void *in, long insize, void *out, long outsize);



//---------------------------------------------------------------------
// LZO - 来自 minilzo
//---------------------------------------------------------------------

// LZO 原始压缩
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// wrkmem   - 需要提供的工作内存 （大小为 ILZO_WRKMEM_SIZE）
// 成功返回0，其他是错误
int ilzo1x_1_compress(const unsigned char *in, size_t insize, 
    unsigned char *out, size_t *outsize, void *wrkmem);


// LZO 原始解压
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// 成功返回0，其他是错误
int ilzo1x_decompress_safe(const unsigned char *in, size_t insize,
    unsigned char *out, size_t *outsize);


// LZO 工作内存大小
#define ILZO_WRKMEM_SIZE	((1 << 14) * sizeof(char*))


// LZO 简易压缩（自动分配临时工作内存）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 成功返回0，其他是错误
long ilzo_compress(const void *in, long insize, void *out, long outsize);

// LZO 简易解压
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 
// 成功返回0，其他是错误
long ilzo_decompress(const void *in, long insize, void *out, long outsize);




//---------------------------------------------------------------------
// 简单封装
//---------------------------------------------------------------------


// 压缩器：如果 out == NULL，返回压缩后输出最大大小，便于安排缓存
// 返回实际压缩长度，如果 outsize不够，返回 -1，如果压缩错误返回 -2。
typedef long (*iCompressProc)(const void *in, long insize, 
	void *out, long outsize, int level);


// 解压器器：返回实际压缩长度，如果 outsize不够，返回 -1，解压错误返回 -2
typedef long (*iDecompressProc)(const void *in, long insize, 
	void *out, long outsize);


#define IPK_METHOD_LZW		0
#define IPK_METHOD_LZO		1


// LZW / LZO 压缩
// method = ILZX_METHOD_LZW / ILZX_METHOD_LZO
// in - 输入数据指针
// insize - 输入数据大小
// out - 输出数据指针，out为 NULL时返回压缩后输出最大大小，便于安排缓存
// outsize - 输出数据大小
// 返回压缩后数据实际大小，如果 outsize不够，返回 -1，其他错误 < 0
long ipk_compress(int method, const void *in, long insize, 
	void *out, long outsize, int level);


// LZW / LZO 解压缩
// method = ILZX_METHOD_LZW / ILZX_METHOD_LZO
// in - 输入数据指针
// insize - 输入数据大小
// out - 输出数据指针，如果 out == NULL，将直接返回解压后数据的大小
// outsize - 输出数据大小
// 返回解压后数据实际大小，如果 outsize不够，返回 -1，其他错误返回 < 0
long ipk_decompress(int method, const void *in, long insize, void *out,
	long outsize);


// 安装新的压缩/解压器
int ipk_install_method(int method, iCompressProc p1, iDecompressProc p2);


#ifdef __cplusplus
}
#endif

#endif



