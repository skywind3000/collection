/**********************************************************************
 *
 * ibitmap.c - self contained bitmap implementation
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

#include "ibitmap.h"


/**********************************************************************
 * Default functions can be changed by macros or config.h 
 **********************************************************************/
#ifndef IBM_DEFAULT_MEMCPY
#define IBM_DEFAULT_MEMCPY  NULL
#endif

#ifndef IBM_DEFAULT_MEMSET
#define IBM_DEFAULT_MEMSET  NULL
#endif

#ifndef IBM_DEFAULT_MALLOC
#define IBM_DEFAULT_MALLOC  NULL
#endif

#ifndef IBM_DEFAULT_FREE 
#define IBM_DEFAULT_FREE  NULL
#endif


/**********************************************************************
 * Bitmap Functions
 **********************************************************************/
void* (*_ibitmap_memcpy)(void*, const void*, size_t) = IBM_DEFAULT_MEMCPY;
void* (*_ibitmap_memset)(void*, int, size_t) = IBM_DEFAULT_MEMSET;
void* (*_ibitmap_malloc)(size_t) = IBM_DEFAULT_MALLOC;
void (*_ibitmap_free)(void*) = IBM_DEFAULT_FREE;

void* ibitmap_memcpy(void *dst, const void *src, size_t size)
{
	const char *ss = (const char*)src;
	char *dd = (char*)dst;
	if (_ibitmap_memcpy) {
		return _ibitmap_memcpy(dst, src, size);
	}
	/* no need to unroll, modern CPUs is capable to predict small loops */
	for (; size >= 4; dd += 4, ss += 4, size -= 4) {
		*((IUINT32*)dd) = *((const IUINT32*)ss);
	}
	switch (size) {
	case 3: dd[2] = ss[2];
	case 2: *((IUINT16*)dd) = *((const IUINT16*)ss); break;
	case 1: dd[0] = ss[0]; break;
	}
	return dst;
}

void* ibitmap_memset(void *dst, int ch, size_t size)
{
	IUINT32 cc = (IUINT32)(ch & 0xff);
	unsigned char *dd = (unsigned char*)dst;
	if (_ibitmap_memset) {
		return _ibitmap_memset(dst, ch, size);
	}
	cc = (cc << 24) | (cc << 16) | (cc << 8) | cc;
	for (; size >= 4; dd += 4, size -= 4) {
		*((IUINT32*)dd) = cc;
	}
	switch (size) {
	case 3: dd[2] = (IUINT8)(cc & 0xff);
	case 2: *((IUINT16*)dd) = (IUINT16)(cc & 0xffff); break;
	case 1: dd[0] = (IUINT8)(cc & 0xff);
	}
	return dst;
}




/**********************************************************************
 * BITMAP INTERFACE
 **********************************************************************/

/* initialize IBITMAP struct with parameters */
void ibitmap_init(IBITMAP *bmp, int w, int h, long pitch, int bpp, void *ps)
{
	bmp->w = (unsigned int)w;
	bmp->h = (unsigned int)h;
	bmp->bpp = (unsigned char)bpp;
	bmp->pitch = (IUINT32)pitch;
	bmp->pixel = (unsigned char*)ps;
	bmp->mask = 0;
	bmp->fmt = 0;
}


/* _ibitmap_malloc/_ibitmap_free must be set before this */
IBITMAP *ibitmap_new(int w, int h, int bpp)
{
	int pixelsize = (bpp + 7) / 8;
	size_t pitch = (pixelsize * w + 7) & (~((size_t)7));
	size_t need = (sizeof(IBITMAP) + 3) & (~((size_t)3));
	IBITMAP *bmp;
	void *ptr;

	if (_ibitmap_malloc == NULL) return NULL;
	bmp = (IBITMAP*)_ibitmap_malloc(need + pitch * h + 8);
	ptr = ((char*)((void*)bmp)) + sizeof(IBITMAP);
	ptr = (char*)((((size_t)ptr) + 7) & (~((size_t)7)));
	ibitmap_init(bmp, w, h, (long)pitch, bpp, ptr);
	ibitmap_memset(bmp->pixel, 0, pitch * h);

	return bmp;
}

/* _ibitmap_malloc/_ibitmap_free must be set before this */
void ibitmap_delete(IBITMAP *bmp)
{
	if (_ibitmap_free != NULL) {
		bmp->bpp = 0;
		bmp->w = 0;
		bmp->h = 0;
		bmp->pitch = 0;
		bmp->pixel = NULL;
		_ibitmap_free(bmp);
	}
}


/*
 * ibitmap_clip - clip the rectangle from the src clip and dst clip then
 * caculate a new rectangle which is shared between dst and src cliprect:
 * clipdst  - dest clip array (left, top, right, bottom)
 * clipsrc  - source clip array (left, top, right, bottom)
 * (x, y)   - dest position
 * rectsrc  - source rect
 * clip     - check IBLIT_HFLIP or IBLIT_VFLIP
 * return zero for successful, return non-zero if there is no shared part
 */
int ibitmap_clip(const int *clipdst, const int *clipsrc, int *x, int *y,
	int *rectsrc, int clip)
{
	int dcl = clipdst[0];       /* dest clip: left     */
	int dct = clipdst[1];       /* dest clip: top      */
	int dcr = clipdst[2];       /* dest clip: right    */
	int dcb = clipdst[3];       /* dest clip: bottom   */
	int scl = clipsrc[0];       /* source clip: left   */
	int sct = clipsrc[1];       /* source clip: top    */
	int scr = clipsrc[2];       /* source clip: right  */
	int scb = clipsrc[3];       /* source clip: bottom */
	int dx = *x;                /* dest x position     */
	int dy = *y;                /* dest y position     */
	int sl = rectsrc[0];        /* source rectangle: left   */
	int st = rectsrc[1];        /* source rectangle: top    */
	int sr = rectsrc[2];        /* source rectangle: right  */
	int sb = rectsrc[3];        /* source rectangle: bottom */
	int hflip, vflip;
	int w, h, d;
	
	hflip = (clip & IBLIT_HFLIP)? 1 : 0;
	vflip = (clip & IBLIT_VFLIP)? 1 : 0;

	if (dcr <= dcl || dcb <= dct || scr <= scl || scb <= sct) 
		return -1;

	if (sr <= scl || sb <= sct || sl >= scr || st >= scb) 
		return -2;

	/* check dest clip: left */
	if (dx < dcl) {
		d = dcl - dx;
		dx = dcl;
		if (!hflip) sl += d;
		else sr -= d;
	}

	/* check dest clip: top */
	if (dy < dct) {
		d = dct - dy;
		dy = dct;
		if (!vflip) st += d;
		else sb -= d;
	}

	w = sr - sl;
	h = sb - st;

	if (w < 0 || h < 0) 
		return -3;

	/* check dest clip: right */
	if (dx + w > dcr) {
		d = dx + w - dcr;
		if (!hflip) sr -= d;
		else sl += d;
	}

	/* check dest clip: bottom */
	if (dy + h > dcb) {
		d = dy + h - dcb;
		if (!vflip) sb -= d;
		else st += d;
	}

	if (sl >= sr || st >= sb) 
		return -4;

	/* check source clip: left */
	if (sl < scl) {
		d = scl - sl;
		sl = scl;
		if (!hflip) dx += d;
	}

	/* check source clip: top */
	if (st < sct) {
		d = sct - st;
		st = sct;
		if (!vflip) dy += d;
	}

	if (sl >= sr || st >= sb) 
		return -5;

	/* check source clip: right */
	if (sr > scr) {
		d = sr - scr;
		sr = scr;
		if (hflip) dx += d;
	}

	/* check source clip: bottom */
	if (sb > scb) {
		d = sb - scb;
		sb = scb;
		if (vflip) dy += d;
	}

	if (sl >= sr || st >= sb) 
		return -6;

	*x = dx;
	*y = dy;

	rectsrc[0] = sl;
	rectsrc[1] = st;
	rectsrc[2] = sr;
	rectsrc[3] = sb;

	return 0;
}


/**********************************************************************
 * blit functions
 **********************************************************************/
iBitmapBlit ibitmap_lut_fns[4][2] = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };

const int ibitmap_lut_bpp[33] = { -1, -1, -1, -1, -1, -1, -1, -1, 
	3, -1, -1, -1, -1, -1, -1, 2, 2, -1, -1, -1, -1, -1, -1, -1, 1, -1, 
	-1, -1, -1, -1, -1, -1, 0 };



void ibitmap_set_fn(int bpp, int ismask, iBitmapBlit proc)
{
	if (bpp >= 8 && bpp <= 32) {
		int x = ibitmap_lut_bpp[bpp];
		if (x >= 0) {
			ibitmap_lut_fns[x][ismask? 1 : 0] = proc;
		}
	}
}


iBitmapBlit ibitmap_get_fn(int bpp, int ismask)
{
	if (bpp >= 8 && bpp <= 32) {
		int x = ibitmap_lut_bpp[bpp];
		if (x >= 0) {
			return ibitmap_lut_fns[x][ismask? 1 : 0];
		}
	}
	return NULL;
}


/**********************************************************************
 * standard blit
 **********************************************************************/
int ibitmap_blit_norm(int bpp, void *dbits, long dpitch, int dx, const
	void *sbits, long spitch, int sx, int w, int h, IUINT32 mask, int flip)
{
	int pixelsize = (bpp + 7) >> 3;
	int y, x, x1, x2, sx0, sxd, endx;
	iBitmapBlit blit;

	x = ibitmap_lut_bpp[bpp];
	blit = ibitmap_lut_fns[x][0];

	if (blit) {
		int c = blit(dbits, dpitch, dx, sbits, spitch, sx, w, h, mask, flip);
		if (c == 0) {
			return 0;
		}
	}

	if (flip & IBLIT_VFLIP) { 
		sbits = (const IUINT8*)sbits + spitch * (h - 1); 
		spitch = -spitch; 
	} 

	switch (pixelsize) {
	case 1:
		if ((flip & IBLIT_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				ibitmap_memcpy((IUINT8*)dbits + dx, 
					(const IUINT8*)sbits + sx, size); 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx + w - 1; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 2:
		if ((flip & IBLIT_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				ibitmap_memcpy((IUINT16*)dbits + dx, 
					(const IUINT16*)sbits + sx, size); 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx + w - 1; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 3:
		if (flip & IBLIT_HFLIP) { 
			sx0 = sx + w - 1; 
			sxd = -1; 
		}	else { 
			sx0 = sx; 
			sxd = 1; 
		} 
		endx = dx + w; 
		for (y = 0; y < h; y++) { 
			IUINT32 cc; 
			for (x1 = dx, x2 = sx0; x1 < endx; x1++, x2 += sxd) { 
				cc = _ipixel_get(24, sbits, x2); 
				_ipixel_set(24, dbits, x1, cc); 
			} 
			dbits = (IUINT8*)dbits + dpitch; 
			sbits = (const IUINT8*)sbits + spitch; 
		} 
		break;

	case 4:
		if ((flip & IBLIT_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				ibitmap_memcpy((IUINT32*)dbits + dx, 
					(const IUINT32*)sbits + sx, size);
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx + w - 1; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;
	}

	return mask - mask;
}



/**********************************************************************
 * transparent blit
 **********************************************************************/
int ibitmap_blit_mask(int bpp, void *dbits, long dpitch, int dx, const
	void *sbits, long spitch, int sx, int w, int h, IUINT32 mask, int flip)
{
	int pixelsize = (bpp + 7) >> 3;
	int y, x1, x2, sx0, sxd, endx;
	iBitmapBlit blit;

	y = ibitmap_lut_bpp[bpp];
	blit = ibitmap_lut_fns[y][1];

	if (blit) {
		int c = blit(dbits, dpitch, dx, sbits, spitch, sx, w, h, mask, flip);
		if (c == 0) {
			return 0;
		}
	}

	if (flip & IBLIT_VFLIP) {
		sbits = (const IUINT8*)sbits + spitch * (h - 1);
		spitch = -spitch;
	}

	switch (pixelsize) {
	case 1:
		if ((flip & IBLIT_HFLIP) == 0) { 
			IUINT8 cmask = (IUINT8)(mask & 0xff);
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				IUINT8 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT8 cmask = (IUINT8)(mask & 0xff);
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx + w - 1; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				IUINT8 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 2:
		if ((flip & IBLIT_HFLIP) == 0) { 
			IUINT16 cmask = (IUINT16)(mask & 0xffff);
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				IUINT16 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT16 cmask = (IUINT16)(mask & 0xffff);
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx + w - 1; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				IUINT16 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 3:
		if (flip & IBLIT_HFLIP) { 
			sx0 = sx + w - 1; 
			sxd = -1; 
		}	else { 
			sx0 = sx; 
			sxd = 1; 
		} 
		endx = dx + w; 
		for (y = 0; y < h; y++) { 
			IUINT32 cc; 
			for (x1 = dx, x2 = sx0; x1 < endx; x1++, x2 += sxd) { 
				cc = _ipixel_get(24, sbits, x2); 
				if (cc != mask) _ipixel_set(24, dbits, x1, cc); 
			} 
			dbits = (IUINT8*)dbits + dpitch; 
			sbits = (const IUINT8*)sbits + spitch; 
		} 
		break;

	case 4:
		if ((flip & IBLIT_HFLIP) == 0) { 
			IUINT32 cmask = (IUINT32)mask;
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				IUINT32 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT32 cmask = (IUINT32)mask;
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx + w - 1; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				IUINT32 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;
	}

	return 0;
}


/**********************************************************************
 * BLIT ROUTINE
 **********************************************************************/

/*
 * ibitmap_blit - blit from source bitmap to dest bitmap
 * returns zero for successful, others for error    
 * dst       - dest bitmap to draw on
 * x, y      - target position of dest bitmap to draw on
 * src       - source bitmap 
 * sx, sy    - source rectangle position in source bitmap
 * w, h      - source rectangle width and height in source bitmap
 * mode      - blit flag bits (IBLIT_CLIP, IBLIT_MASK)
 */
int ibitmap_blit(IBITMAP *dst, int x, int y, 
	const IBITMAP *src, int sx, int sy, int w, int h, int mode)
{
	const void *sbits;
	void *dbits;

	/* check whether parametes is error */
	if (src == NULL || dst == NULL) return -1;
	if (src->bpp != dst->bpp) return -2;

	/* check whether need to clip rectangle */
	if ((mode & IBLIT_NC) == 0) {
		int clipdst[4], clipsrc[4], rect[4], r;
		clipdst[0] = 0;
		clipdst[1] = 0;
		clipdst[2] = (int)dst->w;
		clipdst[3] = (int)dst->h;
		clipsrc[0] = 0;
		clipsrc[1] = 0;
		clipsrc[2] = (int)src->w;
		clipsrc[3] = (int)src->h;
		rect[0] = sx;
		rect[1] = sy;
		rect[2] = sx + (int)w;
		rect[3] = sy + (int)h;
		r = ibitmap_clip(clipdst, clipsrc, &x, &y, rect, mode);
		if (r != 0) return 0;
		sx = rect[0];
		sy = rect[1];
		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	/* prepare variables */
	sbits = _ibitmap_ptr(src, sy);
	dbits = _ibitmap_ptr(dst, y);

	if ((mode & IBLIT_MASK) == 0) {
		ibitmap_blit_norm(src->bpp, dbits, (long)dst->pitch, x, sbits, 
				(long)src->pitch, sx, w, h, src->mask, mode);
	}	
	else {
		ibitmap_blit_mask(src->bpp, dbits, (long)dst->pitch, x, sbits, 
				(long)src->pitch, sx, w, h, src->mask, mode);
	}

	return 0;
}



/**********************************************************************
 * Stretch Bitmap
 **********************************************************************/

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
	const IBITMAP *src, int sx, int sy, int sw, int sh, int mode)
{
	int dstwidth, dstheight, dstwidth2, dstheight2, srcwidth2, srcheight2;
	int werr, herr, incx, incy, i, j, nbytes; 
	static unsigned long endian = 0x11223344;
	unsigned long mask, key24;
	long longsize, isize;

	/* check whether parametes is error */
	ASSERTION(src && dst);
	ASSERTION(src->bpp == dst->bpp);

	if (src->bpp != dst->bpp)
		return -10;

	if (dw == sw && dh == sh) {
		return ibitmap_blit(dst, dx, dy, src, sx, sy, sw, sh, mode);
	}

	if (dx < 0 || dx + dw > (int)dst->w || dy < 0 || dy + dh > (int)dst->h ||
		sx < 0 || sx + sw > (int)src->w || sy < 0 || sy + sh > (int)src->h ||
		sh <= 0 || sw <= 0 || dh <= 0 || dw <= 0) 
		return -20;

	dstwidth = dw;
	dstheight = dh;
	dstwidth2 = dw * 2;
	dstheight2 = dh * 2;
	srcwidth2 = sw * 2;
	srcheight2 = sh * 2;

	if (mode & IBLIT_VFLIP) sy = sy + sh - 1, incy = -1;
	else incy = 1;

	herr = srcheight2 - dstheight2;
	nbytes = (src->bpp + 7) / 8;

	isize = sizeof(int);
	longsize = sizeof(long);
	mask = (unsigned long)src->mask;

	for (j = 0; j < dstheight; j++) {
		const IUINT8 *srcrow = _ibitmap_ptr(src, sy);
		IUINT8 *dstrow = _ibitmap_ptr(dst, dy);
		const IUINT8 *srcpix = srcrow + nbytes * sx;
		IUINT8 *dstpix = dstrow + nbytes * dx;
		incx = nbytes;
		if (mode & IBLIT_HFLIP) {
			srcpix += (sw - 1) * nbytes;
			incx = -nbytes;
		}
		werr = srcwidth2 - dstwidth2;

		switch (nbytes)
		{
		case 1:
			{
				unsigned char mask8;
				if ((mode & IBLIT_MASK) == 0) {
					for (i = dstwidth; i > 0; i--) {
						*dstpix++ = *srcpix;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}   else {
					mask8 = (unsigned char)(src->mask & 0xff);
					for (i = dstwidth; i > 0; i--) {
						if (*srcpix != mask8) *dstpix = *srcpix;
						dstpix++;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}
			}
			break;

		case 2:
			{
				unsigned short mask16;
				if ((mode & IBLIT_MASK) == 0) {
					for (i = dstwidth; i > 0; i--) {
						*((unsigned short*)dstpix) = 
											*((unsigned short*)srcpix);
						dstpix += 2;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}   else {
					mask16 = (unsigned short)(src->mask & 0xffff);
					for (i = dstwidth; i > 0; i--) {
						if (*((unsigned short*)srcpix) != mask16) 
							*((unsigned short*)dstpix) = 
											*((unsigned short*)srcpix);
						dstpix += 2;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}
			}
			break;
		
		case 3:
			if (((unsigned char*)&endian)[0] != 0x44) key24 = mask;
			else key24 = ((mask & 0xFFFF) << 8) | ((mask >> 16) & 0xFF);
			if ((mode & IBLIT_MASK) == 0) {
				for (i = dstwidth; i > 0; i--) {
					dstpix[0] = srcpix[0];
					dstpix[1] = srcpix[1];
					dstpix[2] = srcpix[2];
					dstpix += 3;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			else if (longsize == 4) {
				unsigned long longmask, k;
				longmask = key24 & 0xffffff;
				for (i = dstwidth; i > 0; i--) {
					k = (((unsigned long)(*(unsigned short*)srcpix)) << 8);
					if ((k | srcpix[2]) != longmask) {
						dstpix[0] = srcpix[0];
						dstpix[1] = srcpix[1];
						dstpix[2] = srcpix[2];
					}
					dstpix += 3;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			else if (isize == 4) {
				unsigned int imask, k;
				imask = key24 & 0xffffff;
				for (i = dstwidth; i > 0; i--) {
					k = (((unsigned int)(*(unsigned short*)srcpix)) << 8);
					if ((k | srcpix[2]) != imask) {
						dstpix[0] = srcpix[0];
						dstpix[1] = srcpix[1];
						dstpix[2] = srcpix[2];
					}
					dstpix += 3;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			break;

		case 4:
			{
				IUINT32 maskint = src->mask;
				if ((mode & IBLIT_MASK) == 0) {
					for (i = dstwidth; i > 0; i--) {
						*((IUINT32*)dstpix) = *((IUINT32*)srcpix);
						dstpix += 4;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}   else {
					for (i = dstwidth; i > 0; i--) {
						if (*((IUINT32*)srcpix) != maskint) 
							*((IUINT32*)dstpix) = *((IUINT32*)srcpix);
						dstpix += 4;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}
			}    
			break;
		}

		while (herr >= 0) {
			sy += incy, herr -= dstheight2;
		}

		herr += srcheight2; 
		dy++;
	}

	return 0;
}



/**********************************************************************
 * fill rectangle
 **********************************************************************/

/* ibitmap_fill - fill the rectangle with given color 
 * returns zero for successful, others for error
 * dst     - dest bitmap to draw on
 * dx, dy  - target position of dest bitmap to draw on
 * w, h    - width and height of the rectangle to be filled
 * col     - indicate the color to fill the rectangle
 */
int ibitmap_fill(IBITMAP *dst, int dx, int dy, int w, int h, IUINT32 col)
{
	int pixsize;
	long delta;
	long pitch;
	char *pixel;
	int i;

	if (dx >= (long)dst->w || dx + w <= 0 || w < 0) return 0;
	if (dy >= (long)dst->h || dy + h <= 0 || h < 0) return 0;
	if (dx < 0) w += dx, dx = 0;
	if (dy < 0) h += dy, dy = 0;
	if (dx + w >= (long)dst->w) w = (long)dst->w - dx;
	if (dy + h >= (long)dst->h) h = (long)dst->h - dy;

	pitch = (long)dst->pitch;

	/* get pixel size */
	pixsize = (dst->bpp + 7) >> 3;

	/* choose linear offset */
	switch (pixsize) {
	case 1: delta = dx; break;
	case 2: delta = (dx << 1);  break;
	case 3: delta = (dx << 1) + dx; break;
	case 4: delta = (dx << 2); break;
	default: delta = dx * pixsize; break;
	}

	/* get the first scanlines of the bitmap */
	pixel = (char*)_ibitmap_ptr(dst, dy) + delta;

	switch (pixsize)
	{
	/* fill for 8 bits color depth */
	case 1:
		for (; h; h--) {
			ibitmap_memset(pixel, col & 0xff, w);
			pixel += pitch;
		}
		break;

	/* fill for 15/16 bits color depth */
	case 2:
		{
			IUINT16 cc = (unsigned short)(col & 0xffff);
			IUINT32 kk = (((IUINT32)cc) << 16) | cc;
			for (; h; h--) {
				IUINT32 *k32 = (IUINT32*)pixel;
				for (i = w >> 1; i; i--) *k32++ = kk;
				if (w & 1) *(IUINT16*)k32 = cc;
				pixel += pitch;
			}
		}
		break;
	
	/* fill for 24 bits color depth */
	case 3:
		{
			IUINT8 c1, c2, c3;
			#if ISYSTEM_CPU_MSB
				c1 = (unsigned char)((col >> 16) & 0xff);
				c2 = (unsigned char)((col >> 8) & 0xff);
				c3 = (unsigned char)(col & 0xff);
			#else
				c1 = (unsigned char)(col & 0xff);
				c2 = (unsigned char)((col >> 8) & 0xff);
				c3 = (unsigned char)((col >> 16) & 0xff);
			#endif
			for (; h; h--) {
				IUINT8 *ptr = (IUINT8*)pixel;
				for (i = w; i; i--) {
					*ptr++ = c1;
					*ptr++ = c2;
					*ptr++ = c3;
				}
				pixel += pitch;
			}
		}
		break;

	/* fill for 32 bits color depth */
	case 4:
		for (; h; h--) {
			IUINT32 *k32 = (IUINT32*)pixel;
			for (i = w; i; i--) *k32++ = col;
			pixel += pitch;
		}
		break;
	}
	return 0;
}


/**********************************************************************
 * Blending
 **********************************************************************/
static iBitmapBlend _ibitmap_blend_argb = NULL;
static iBitmapBlend _ibitmap_blend_prgb = NULL;

int ibitmap_blending(void *dbits, long dpitch, int dx, const void *sbits,
	long spitch, int sx, int w, int h, int alpha, int mode)
{
	IINT32 sa, sr, sg, sb, da, dr, dg, db;
	int x, c;
	iBitmapBlend blend = _ibitmap_blend_argb;

	if (alpha <= 0) return 0;

	if (mode & IBLIT_SRCOVER) {
		blend = _ibitmap_blend_prgb;
	}

	if (blend) {
		c = blend(dbits, dpitch, dx, sbits, spitch, sx, w, h, alpha, mode);
		if (c == 0) return 0;
	}

	if (mode & IBLIT_VFLIP) {
		sbits = (const IUINT8*)sbits + spitch * (h - 1);
		spitch = -spitch;
	}

	alpha = (alpha > 255)? 255 : alpha;

	/* premultiplied src over */
	#define _IBLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da) do { \
			IUINT32 SA = 255 - (sa); \
			(dr) = (dr) * SA; \
			(dg) = (dg) * SA; \
			(db) = (db) * SA; \
			(da) = (da) * SA; \
			(dr) = _ibitmap_fast_div_255(dr) + (sr); \
			(dg) = _ibitmap_fast_div_255(dg) + (sg); \
			(db) = _ibitmap_fast_div_255(db) + (sb); \
			(da) = _ibitmap_fast_div_255(da) + (sa); \
		}	while (0)

	/* blend onto a normal surface (with alpha channel) */
	#define _IBLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da) do { \
			IINT32 XA = (255 - (da)) * (sa); \
			IINT32 FA = (da) + _ibitmap_fast_div_255(XA); \
			IINT32 SA = (sa); \
			SA = (FA != 0)? ((SA * 255) / FA) : (0); \
			(da) = FA; \
			XA = 255 - SA; \
			(dr) = ((IINT32)(sr)) * SA + ((IINT32)(dr)) * XA; \
			(dg) = ((IINT32)(sg)) * SA + ((IINT32)(dg)) * XA; \
			(db) = ((IINT32)(sb)) * SA + ((IINT32)(db)) * XA; \
			(dr) = _ibitmap_fast_div_255(dr); \
			(dg) = _ibitmap_fast_div_255(dg); \
			(db) = _ibitmap_fast_div_255(db); \
		}	while (0)

	#define _IBLEND_DISASM_8888(x, a, b, c, d) do { \
            (a) = ((x) >> 24) & 0xff; \
            (b) = ((x) >> 16) & 0xff; \
            (c) = ((x) >>  8) & 0xff; \
            (d) = ((x) >>  0) & 0xff; \
        }   while (0)

	#define _IBLEND_ASM_8888(a, b, c, d) ((IUINT32)( \
            ((IUINT32)(a) << 24) | \
            ((IUINT32)(b) << 16) | \
            ((IUINT32)(c) <<  8) | \
            ((IUINT32)(d) <<  0)))

	if ((mode & IBLIT_HFLIP) == 0) {
		#define _IBLEND_LOOP_START do { \
			for (; h > 0; h--) { \
				const IUINT32 *src = ((const IUINT32*)sbits) + sx; \
				IUINT32 *dst = ((IUINT32*)dbits) + dx; \
				for (x = w; x > 0; x--, src++, dst++) { \
					_IBLEND_DISASM_8888(src[0], sa, sr, sg, sb); \
					if (sa == 0) continue; \
					if (sa + alpha == 510) { dst[0] = src[0]; continue; } \
					_IBLEND_DISASM_8888(dst[0], da, dr, dg, db); 
		#define _IBLEND_LOOP_END \
					dst[0] = _IBLEND_ASM_8888(da, dr, dg, db); \
				} \
				dbits = (char*)dbits + dpitch; \
				sbits = (const char*)sbits + spitch; \
			} \
		}	while (0)
		if ((mode & IBLIT_SRCOVER) == 0) {
			if (alpha >= 255) {
				_IBLEND_LOOP_START;
				_IBLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}	else {
				_IBLEND_LOOP_START;
				sa *= alpha;
				sa = _ibitmap_fast_div_255(sa);
				_IBLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}
		}
		else {
			if (alpha >= 255) {
				_IBLEND_LOOP_START;
				_IBLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}	else {
				_IBLEND_LOOP_START;
				sr *= alpha;
				sg *= alpha;
				sb *= alpha;
				sa *= alpha;
				sr = _ibitmap_fast_div_255(sr);
				sg = _ibitmap_fast_div_255(sg);
				sb = _ibitmap_fast_div_255(sb);
				sa = _ibitmap_fast_div_255(sa);
				_IBLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}
		}
		#undef _IBLEND_LOOP_START
		#undef _IBLEND_LOOP_END
	}
	else {
		#define _IBLEND_LOOP_START do { \
			for (; h > 0; h--) { \
				const IUINT32 *src = ((const IUINT32*)sbits) + sx + w - 1;\
				IUINT32 *dst = ((IUINT32*)dbits) + dx; \
				for (x = w; x > 0; x--, src--, dst++) { \
					_IBLEND_DISASM_8888(src[0], sa, sr, sg, sb); \
					if (sa == 0) continue; \
					if (sa + alpha == 510) { dst[0] = src[0]; continue; } \
					_IBLEND_DISASM_8888(dst[0], da, dr, dg, db); 
		#define _IBLEND_LOOP_END \
					dst[0] = _IBLEND_ASM_8888(da, dr, dg, db); \
				} \
				dbits = (char*)dbits + dpitch; \
				sbits = (const char*)sbits + spitch; \
			} \
		}	while (0)
		if ((mode & IBLIT_SRCOVER) == 0) {
			if (alpha >= 255) {
				_IBLEND_LOOP_START;
				_IBLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}	else {
				_IBLEND_LOOP_START;
				sa *= alpha;
				sa = _ibitmap_fast_div_255(sa);
				_IBLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}
		}
		else {
			if (alpha >= 255) {
				_IBLEND_LOOP_START;
				_IBLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}	else {
				_IBLEND_LOOP_START;
				sr *= alpha;
				sg *= alpha;
				sb *= alpha;
				sa *= alpha;
				sr = _ibitmap_fast_div_255(sr);
				sg = _ibitmap_fast_div_255(sg);
				sb = _ibitmap_fast_div_255(sb);
				sa = _ibitmap_fast_div_255(sa);
				_IBLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da);
				_IBLEND_LOOP_END;
			}
		}
		#undef _IBLEND_START
		#undef _IBLEND_END
	}

	return 0;
}



/**********************************************************************
 * Blending
 **********************************************************************/

/* ibitmap_blend - blend to destination
 * dst       - dest bitmap to draw on
 * x, y      - target position of dest bitmap to draw on
 * src       - source bitmap 
 * sx, sy    - source rectangle position in source bitmap
 * w, h      - source rectangle width and height in source bitmap
 * mode      - flags of IBLIT_SRCOVER, IBLIT_HFLIP, IBLIT_VFLIP...
 */
int ibitmap_blend(IBITMAP *dst, int x, int y, const IBITMAP *src,
		int sx, int sy, int w, int h, int alpha, int mode)
{
	const void *sbits;
	void *dbits;

	/* check whether parametes is error */
	if (src == NULL || dst == NULL) return -1;
	if (src->bpp != dst->bpp || src->bpp != 32) return -2;

	/* check whether need to clip rectangle */
	if ((mode & IBLIT_NC) == 0) {
		int clipdst[4], clipsrc[4], rect[4], r;
		clipdst[0] = 0;
		clipdst[1] = 0;
		clipdst[2] = (int)dst->w;
		clipdst[3] = (int)dst->h;
		clipsrc[0] = 0;
		clipsrc[1] = 0;
		clipsrc[2] = (int)src->w;
		clipsrc[3] = (int)src->h;
		rect[0] = sx;
		rect[1] = sy;
		rect[2] = sx + (int)w;
		rect[3] = sy + (int)h;
		r = ibitmap_clip(clipdst, clipsrc, &x, &y, rect, mode);
		if (r != 0) return 0;
		sx = rect[0];
		sy = rect[1];
		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	/* prepare variables */
	sbits = _ibitmap_ptr(src, sy);
	dbits = _ibitmap_ptr(dst, y);

	ibitmap_blending(dbits, (long)dst->pitch, x, sbits, (long)src->pitch, 
			sx, w, h, alpha, mode);

	return 0;
}



/**********************************************************************
 * Utilities
 **********************************************************************/

void ibitmap_set_pixel(IBITMAP *bmp, int x, int y, IUINT32 color)
{
	unsigned char *ptr;
	int pixsize;
	if ((unsigned int)x >= bmp->w || (unsigned int)y >= bmp->h)
		return;
	pixsize = (bmp->bpp + 7) >> 3;
	ptr = _ibitmap_ptr(bmp, y);	
	switch (pixsize) {
	case 1:
		_ipixel_set_8(ptr, x, color);
		break;
	case 2:
		_ipixel_set_16(ptr, x, color);
		break;
	case 3:
		_ipixel_set_24(ptr, x, color);
		break;
	case 4:
		_ipixel_set_32(ptr, x, color);
		break;
	}
}


IUINT32 ibitmap_get_pixel(IBITMAP *bmp, int x, int y)
{
	unsigned char *ptr;
	IUINT32 c = 0;
	int pixsize;
	if ((unsigned int)x >= bmp->w || (unsigned int)y >= bmp->h)
		return 0;
	pixsize = (bmp->bpp + 7) >> 3;
	ptr = _ibitmap_ptr(bmp, y);	
	switch (pixsize) {
	case 1: c = _ipixel_get_8(ptr, x); break;
	case 2: c = _ipixel_get_16(ptr, x); break;
	case 3: c = _ipixel_get_24(ptr, x); break;
	case 4: c = _ipixel_get_32(ptr, x); break;
	}
	return c;
}


void ibitmap_line(IBITMAP *bmp, int x1, int y1, int x2, int y2, IUINT32 col)
{
	int x, y;
	if (x1 == x2 && y1 == y2) {
		ibitmap_set_pixel(bmp, x1, y1, col);
		return;
	}	else if (x1 == x2) {
		int inc = (y1 <= y2)? 1 : -1;
		for (y = y1; y != y2; y += inc) ibitmap_set_pixel(bmp, x1, y, col);
		ibitmap_set_pixel(bmp, x2, y2, col);
	}	else if (y1 == y2) {
		int inc = (x1 <= x2)? 1 : -1;
		for (x = x1; x != x2; x += inc) ibitmap_set_pixel(bmp, x, y1, col);
		ibitmap_set_pixel(bmp, x2, y2, col);
	}	else {
		int dx = (x1 < x2)? x2 - x1 : x1 - x2;
		int dy = (y1 < y2)? y2 - y1 : y1 - y2;
		int rem = 0;
		if (dx >= dy) {
			if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; x <= x2; x++) {
				ibitmap_set_pixel(bmp, x, y, col);
				rem += dy;
				if (rem >= dx) {
					rem -= dx;
					y += (y2 >= y1)? 1 : -1;
					ibitmap_set_pixel(bmp, x, y, col);
				}
			}
			ibitmap_set_pixel(bmp, x2, y2, col);
		}	else {
			if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; y <= y2; y++) {
				ibitmap_set_pixel(bmp, x, y, col);
				rem += dx;
				if (rem >= dy) {
					rem -= dy;
					x += (x2 >= x1)? 1 : -1;
					ibitmap_set_pixel(bmp, x, y, col);
				}
			}
			ibitmap_set_pixel(bmp, x2, y2, col);
		}
	}
}


static const char *ibitmap_decode_16u(const char *p, IUINT16 *w)
{
#if ISYSTEM_CPU_MSB
	*w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
	*w = *(const unsigned short*)p;
#endif
	p += 2;
	return p;
}

/* decode 32 bits unsigned int (lsb) */
static const char *ibitmap_decode_32u(const char *p, IUINT32 *l)
{
#if ISYSTEM_CPU_MSB
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	*l = *(const IUINT32*)p;
#endif
	p += 4;
	return p;
}


static int ibitmap_bmp_routine(const void *buffer, int *w, int *h, int *bpp, 
		void *bits, long pitch, unsigned char *palette)
{
	struct _MyBITMAPINFOHEADER { 
		IUINT32	biSize; 
		IUINT32	biWidth; 
		IINT32	biHeight; 
		IUINT16 biPlanes; 
		IUINT16 biBitCount;
		IUINT32	biCompression; 
		IUINT32	biSizeImage; 
		IUINT32	biXPelsPerMeter; 
		IUINT32	biYPelsPerMeter; 
		IUINT32	biClrUsed; 
		IUINT32	biClrImportant; 
	}	InfoHeader; 

	char FileHeader[14];
	const char *ptr = (const char *)buffer;
	int width, height, depth, y, i;
	long pixelsize, srcpitch, minpitch;
	IUINT32 offset;

	ibitmap_memcpy(FileHeader, ptr, 14);
	ptr += 14;

	if (FileHeader[0] != 0x42 || FileHeader[1] != 0x4d) return -1;

	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biSize);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biWidth);
	ptr = ibitmap_decode_32u(ptr, (IUINT32*)&InfoHeader.biHeight);
	ptr = ibitmap_decode_16u(ptr, &InfoHeader.biPlanes);
	ptr = ibitmap_decode_16u(ptr, &InfoHeader.biBitCount);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biCompression);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biSizeImage);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biXPelsPerMeter);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biYPelsPerMeter);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biClrUsed);
	ptr = ibitmap_decode_32u(ptr, &InfoHeader.biClrImportant);

	if (bits == NULL) {
		*w = InfoHeader.biWidth;
		*h = InfoHeader.biHeight;
		*bpp = InfoHeader.biBitCount;
		return 0;
	}

	ibitmap_decode_32u(FileHeader + 10, &offset);

	if (palette != NULL && InfoHeader.biBitCount == 8) {
		for (i = 0; i < 256; i++) {
			if (ptr >= (const char*)buffer + offset) break;
			palette[0] = *(const unsigned char*)ptr++;
			palette[1] = *(const unsigned char*)ptr++;
			palette[2] = *(const unsigned char*)ptr++;
			palette[3] = 255;
			palette += 4;
			ptr++;
		}
	}

	ptr = (const char*)buffer + offset;

	width = InfoHeader.biWidth;
	height = InfoHeader.biHeight;
	depth = InfoHeader.biBitCount;
	pixelsize = ((depth + 7) / 8);
	srcpitch = (pixelsize * width + 3) & (~3l);
	minpitch = (srcpitch < pitch)? srcpitch : pitch;

	switch (depth) {
	case 8:
	case 15:
	case 16:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + srcpitch * y;
			IUINT8 *dst = ((IUINT8*)bits) + (height - y - 1) * pitch;
			ibitmap_memcpy(dst, src, minpitch);
		}
		break;
	case 24:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + srcpitch * y;
			IUINT8 *dst = ((IUINT8*)bits) + (height - y - 1) * pitch;
			ibitmap_memcpy(dst, src, minpitch);
		}
		break;
	case 32:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + srcpitch * y;
			IUINT8 *dst = ((IUINT8*)bits) + (height - y - 1) * pitch;
			ibitmap_memcpy(dst, src, minpitch);
		}
		break;
	}

	return 0;
}


/* get bmp file info, data is the bmp file content in memory */
int ibitmap_bmp_info(const void *data, int *w, int *h, int *bpp)
{
	return ibitmap_bmp_routine(data, w, h, bpp, NULL, 0, NULL);
}


/* read the bmp file into IBITMAP, returns zero for success */
int ibitmap_bmp_read(const void *data, IBITMAP *bmp, IUINT8 *palette)
{
	int w, h, bpp;
	int hr = ibitmap_bmp_routine(data, &w, &h, &bpp, NULL, 0, NULL);
	if (hr != 0) return hr;
	if ((int)bmp->bpp != bpp) return -2;
	if ((int)bmp->w < w) return -3;
	if ((int)bmp->h < h) return -3;
	ibitmap_bmp_routine(data, &w, &h, &bpp, bmp->pixel, 
			(long)bmp->pitch, palette);
	return 0;
}



