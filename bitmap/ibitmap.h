/**********************************************************************
 *
 * ibitmap.h - self contained bitmap implementation
 *
 * interfaces:
 *
 * init    - initialize bitmap object from existing memory
 * new     - create a new bitmap and return the struct address
 * delete  - free the bitmap 
 * blit    - copy the speciafied rectangle from one bitmap to another
 * setmask - set the color key for transparent blit (IBLIT_MASK on)
 * fill    - fill a rectange in the bitmap
 * stretch - scale blit
 *
 * the history of this file:
 *
 * Feb.  7 2003    skywind    created including create/release/blit
 * Dec. 15 2004    skywind    new fill / stretch
 * Aug. 12 2007    skywind    get rid of crt dependence
 * Aug. 13 2007    skywind    new ibitmap_blend 
 *
 **********************************************************************/

#ifndef _IBITMAP_H_
#define _IBITMAP_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_NOT_STDDEF_H
#include <stddef.h>
#endif

#if !HAVE_NOT_LIMITS_H
#include <limits.h>
#endif


/**********************************************************************
 * 32 BITS UINT/INT DEFINITION 
 **********************************************************************/
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(__UINT32_TYPE__) && defined(__UINT32_TYPE__)
	typedef __UINT32_TYPE__ ISTDUINT32;
	typedef __INT32_TYPE__ ISTDINT32;
#elif defined(__UINT_FAST32_TYPE__) && defined(__INT_FAST32_TYPE__)
	typedef __UINT_FAST32_TYPE__ ISTDUINT32;
	typedef __INT_FAST32_TYPE__ ISTDINT32;
#elif defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
	defined(_M_AMD64)
	typedef unsigned int ISTDUINT32;
	typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
	defined(__i386) || defined(_M_X86)
	typedef unsigned long ISTDUINT32;
	typedef long ISTDINT32;
#elif defined(__MACOS__)
	typedef UInt32 ISTDUINT32;
	typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
	#include <sys/types.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
	#include <sys/inttypes.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
	typedef unsigned __int32 ISTDUINT32;
	typedef __int32 ISTDINT32;
#elif defined(__GNUC__) && (__GNUC__ > 3)
	#include <stdint.h>
	typedef uint32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#else 
#if UINT_MAX == 0xFFFFU
	typedef unsigned long ISTDUINT32; 
	typedef long ISTDINT32;
#else
	typedef unsigned int ISTDUINT32;
	typedef int ISTDINT32;
#endif
#endif
#endif


/**********************************************************************
 * UINT / INT
 **********************************************************************/
#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char IUINT8;
#endif

#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef signed char IINT8;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef signed short IINT16;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 IUINT32;
#endif



/**********************************************************************
 * LSB / MSB
 **********************************************************************/
#ifndef ISYSTEM_CPU_MSB
	#ifdef _BIG_ENDIAN_
		#if _BIG_ENDIAN_
			#define ISYSTEM_CPU_MSB		1
		#endif
	#endif
	#ifndef ISYSTEM_CPU_MSB
		#if defined(__hppa__) || \
			defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
			(defined(__MIPS__) && defined(__MISPEB__)) || \
			defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
			defined(__sparc__) || defined(__powerpc__) || \
			defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
			#define ISYSTEM_CPU_MSB		1
		#endif
	#endif
	#ifndef ISYSTEM_CPU_MSB
		#define ISYSTEM_CPU_MSB		0
	#endif
#endif

#ifndef ASSERTION
#define ASSERTION(x) ((void)0)
#endif


/**********************************************************************
 * IBITMAP
 **********************************************************************/

struct IBITMAP
{
	unsigned int w;             /* width of the bitmap */
	unsigned int h;             /* height of the bitmap */
	IUINT32 pitch;              /* scanline size in bytes */
	IUINT32 mask;               /* mask color / key color */
	unsigned char *pixel;       /* color bits ptr */
	unsigned char bpp;          /* color depth: 8/16/24/32 */
	unsigned char fmt;          /* color format */
};

typedef struct IBITMAP IBITMAP;


/**********************************************************************
 * MACROS
 **********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#define IBLIT_MASK     1    /* blit mode: enable transparent blit */
#define IBLIT_HFLIP    2    /* flip horizon */
#define IBLIT_VFLIP    4    /* flip vertical */
#define IBLIT_NC       8    /* disable clip (careful) */
#define IBLIT_SRCOVER  16   /* SRCOVER */

#define _ibitmap_ptr(b, y) (((b)->pixel) + (y) * ((long)((b)->pitch)))
#define _ibitmap_ptr_8(b, y)  ((IUINT8*)_ibitmap_ptr(b, y))
#define _ibitmap_ptr_16(b, y) ((IUINT16*)_ibitmap_ptr(b, y))
#define _ibitmap_ptr_32(b, y) ((IUINT32*)_ibitmap_ptr(b, y))


#define _ipixel_get_8(ptr, offset)  (((const IUINT8 *)(ptr))[offset])
#define _ipixel_get_16(ptr, offset) (((const IUINT16*)(ptr))[offset])
#define _ipixel_get_32(ptr, offset) (((const IUINT32*)(ptr))[offset])

#define _ipixel_set_8(ptr, off, c)  (((IUINT8 *)(ptr))[off] = (IUINT8)(c))
#define _ipixel_set_16(ptr, off, c) (((IUINT16*)(ptr))[off] = (IUINT16)(c))
#define _ipixel_set_32(ptr, off, c) (((IUINT32*)(ptr))[off] = (IUINT32)(c))

#if ISYSTEM_CPU_MSB
#define _ipixel_get_24(ptr, offset) \
	( (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[0]) << 16 ) | \
	  (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[1]) <<  8 ) | \
	  (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[2]) <<  0 ))
#define _ipixel_set_24(ptr, off, c)  do { \
		((IUINT8*)(ptr))[(off) * 3 + 0] = (IUINT8) (((c) >> 16) & 0xff); \
		((IUINT8*)(ptr))[(off) * 3 + 1] = (IUINT8) (((c) >>  8) & 0xff); \
		((IUINT8*)(ptr))[(off) * 3 + 2] = (IUINT8) (((c) >>  0) & 0xff); \
	}   while (0)
#else
#define _ipixel_get_24(ptr, offset) \
	( (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[0]) <<  0 ) | \
	  (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[1]) <<  8 ) | \
	  (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[2]) << 16 ))
#define _ipixel_set_24(ptr, off, c) do { \
		((IUINT8*)(ptr))[(off) * 3 + 0] = (IUINT8) (((c) >>  0) & 0xff); \
		((IUINT8*)(ptr))[(off) * 3 + 1] = (IUINT8) (((c) >>  8) & 0xff); \
		((IUINT8*)(ptr))[(off) * 3 + 2] = (IUINT8) (((c) >> 16) & 0xff); \
	}   while (0)
#endif

#define _ipixel_get(nbits, ptr, off) _ipixel_get_##nbits(ptr, off)
#define _ipixel_set(nbits, ptr, off, c) _ipixel_set_##nbits(ptr, off, c)

#define _ibitmap_fast_div_255(x) (((x) + (((x) + 257) >> 8)) >> 8)


/**********************************************************************
 * Bitmap Interface
 **********************************************************************/


/* initialize IBITMAP struct with parameters */
void ibitmap_init(IBITMAP *bmp, int w, int h, long pitch, int bpp, void *ps);


/* _ibitmap_malloc/_ibitmap_free must be set before this */
IBITMAP *ibitmap_new(int w, int h, int bpp);

/* _ibitmap_malloc/_ibitmap_free must be set before this */
void ibitmap_delete(IBITMAP *bmp);


/*
 * ibitmap_clip - clip the rectangle from the src clip and dst clip then
 * caculate a new rectangle which is shared between dst and src cliprect:
 * clipdst  - dest clip array (left, top, right, bottom)
 * clipsrc  - source clip array (left, top, right, bottom)
 * (x, y)   - dest position
 * rectsrc  - source rect
 * flip     - check IBLIT_HFLIP or IBLIT_VFLIP or 0
 * return zero for successful, return non-zero if there is no shared part
 */
int ibitmap_clip(const int *clipdst, const int *clipsrc, int *x, int *y,
	int *rectsrc, int flip);




/*
 * ibitmap_blit - blit from source bitmap to dest bitmap
 * returns zero for successful, others for error    
 * dst       - dest bitmap to draw on
 * x, y      - target position of dest bitmap to draw on
 * src       - source bitmap 
 * sx, sy    - source rectangle position in source bitmap
 * w, h      - source rectangle width and height in source bitmap
 * mode      - flags of IBLIT_NC, IBLIT_MASK, IBLIT_HFLIP, IBLIT_VFLIP...
 */
int ibitmap_blit(IBITMAP *dst, int x, int y, const IBITMAP *src, 
		int sx, int sy, int w, int h, int mode);


/*
 * ibitmap_setmask - change mask(colorkey) of the bitmap
 * when blit with IBLIT_MASK, this value can be used as the key color to 
 * transparent. you can change bmp->mask directly without calling it.
 */
int ibitmap_setmask(IBITMAP *bmp, IUINT32 mask);


/* ibitmap_fill - fill the rectangle with given color 
 * returns zero for successful, others for error
 * dst     - dest bitmap to draw on
 * dx, dy  - target position of dest bitmap to draw on
 * w, h    - width and height of the rectangle to be filled
 * col     - indicate the color to fill the rectangle
 */
int ibitmap_fill(IBITMAP *dst, int dx, int dy, int w, int h, IUINT32 col);




/* ibitmap_stretch - copies a bitmap from a source rectangle into a 
 * destination rectangle, stretching or compressing the bitmap to fit 
 * the dimensions of the destination rectangle
 * returns zero for successful, others for invalid rectangle
 * dst       - dest bitmap to draw on
 * dx, dy    - target rectangle position of dest bitmap to draw on
 * dw, dh    - target rectangle width and height in dest bitmap
 * src       - source bitmap 
 * sx, sy    - source rectangle position in source bitmap
 * sw, sh    - source rectangle width and height in source bitmap
 * mode      - flags of IBLIT_MASK, IBLIT_HFLIP, IBLIT_VFLIP...
 * it uses bresenham like algorithm instead of fixed point or indexing 
 * to avoid integer size overflow and memory allocation, just use it 
 * when you don't have a stretch function.
 */
int ibitmap_stretch(IBITMAP *dst, int dx, int dy, int dw, int dh,
	const IBITMAP *src, int sx, int sy, int sw, int sh, int mode);


/**********************************************************************
 * Bitmap Functions
 **********************************************************************/
extern void* (*_ibitmap_memcpy)(void*, const void*, size_t);
extern void* (*_ibitmap_memset)(void*, int, size_t);
extern void* (*_ibitmap_malloc)(size_t);   /* initialize to NULL */
extern void (*_ibitmap_free)(void*);      /* initialize to NULL */

typedef int (*iBitmapBlit)(void *dst, long dpitch, int dx, const void *src, 
	long spitch, int sx, int w, int h, IUINT32 mask, int flip);

typedef int (*iBitmapBlend)(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, int alpha, int flip);

void ibitmap_set_fn(int bpp, int ismask, iBitmapBlit proc);

iBitmapBlit ibitmap_get_fn(int bpp, int ismask);


/**********************************************************************
 * Blending
 **********************************************************************/


/* ibitmap_blend - blend to destination (bpp=32 is required)
 * dst       - dest bitmap to draw on
 * x, y      - target position of dest bitmap to draw on
 * src       - source bitmap 
 * sx, sy    - source rectangle position in source bitmap
 * w, h      - source rectangle width and height in source bitmap
 * mode      - flags of IBLIT_SRCOVER, IBLIT_HFLIP, IBLIT_VFLIP...
 */
int ibitmap_blend(IBITMAP *dst, int x, int y, const IBITMAP *src,
		int sx, int sy, int w, int h, int alpha, int mode);


/**********************************************************************
 * Utilities
 **********************************************************************/

void ibitmap_set_pixel(IBITMAP *bmp, int x, int y, IUINT32 color);

IUINT32 ibitmap_get_pixel(IBITMAP *bmp, int x, int y);

void ibitmap_line(IBITMAP *bmp, int x1, int y1, int x2, int y2, IUINT32 col);


/* get bmp file info, data is the bmp file content in memory */
int ibitmap_bmp_info(const void *data, int *w, int *h, int *bpp);

/* read the bmp file into IBITMAP, returns zero for success */
int ibitmap_bmp_read(const void *data, IBITMAP *bmp, IUINT8 *palette);





#ifdef __cplusplus
}
#endif



#endif




