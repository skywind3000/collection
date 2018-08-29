#include "ilzw.h"


//---------------------------------------------------------------------
// inline predefinition
//---------------------------------------------------------------------
#ifndef INLINE
#ifdef __GNUC__

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
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
// macros
//---------------------------------------------------------------------
#define ilzw_table(lzw, i, j) ((lzw)->table[(i) * 2 + (j)])

#define ilzw_base(lzw, i) ilzw_table(lzw, i, 0)
#define ilzw_size(lzw, i) ilzw_table(lzw, i, 1)
#define ilzw_newc(lzw, i) (lzw->newc[i])
#define ilzw_next(lzw, i) (lzw->next[i])

#define ilzw_hash_first(c) ((unsigned long)(c))
#define ilzw_hash_next0(h, c) (h ^ ((h << 5) + (h >> 2) + (unsigned long)c))
#define ilzw_hash_next(h, c) ilzw_hash_next0((h), (c))



//---------------------------------------------------------------------
// get bits
//---------------------------------------------------------------------
static inline int ilzw_bits_get(iLzwDesc *lzw, int nbits)
{
	int bitcount, bitdata, mask, data, c, i;
	bitcount = lzw->inbits;
	bitdata = lzw->indata;
	for (data = 0, i = 0; nbits > 0; ) {
		if (bitcount == 0) {
			if (lzw->inpos >= lzw->inmax) {
				lzw->error = 1;
				return -1;
			}
			bitdata = lzw->inptr[lzw->inpos];
			lzw->inpos++;
			bitcount = 8;
		}
		c = (nbits >= bitcount)? bitcount : nbits;
		mask = (1L << c) - 1;
		data |= (bitdata & mask) << i;
		bitdata >>= c;
		i += c;
		nbits -= c;
		bitcount -= c;
	}
	lzw->inbits = bitcount;
	lzw->indata = bitdata;
	return data;
}

//---------------------------------------------------------------------
// put bits
//---------------------------------------------------------------------
static inline int ilzw_bits_put(iLzwDesc *lzw, int data, int nbits)
{
	int bitcount, bitdata, mask, c;
	if (data < 0 || nbits <= 0) {
		if (lzw->outbits > 0) {
			if (lzw->outpos >= lzw->outmax) {
				lzw->error = 2;
				return -1;
			}
			lzw->outptr[lzw->outpos] = (unsigned char)(lzw->outdata);
			lzw->outpos++;
			lzw->outbits = 0;
			lzw->outdata = 0;
		}
		return 0;
	}
	bitcount = lzw->outbits;
	bitdata = lzw->outdata;
	for (; nbits > 0; ) {
		c = 8 - bitcount;
		if (nbits < c) c = nbits;
		mask = ((1L << c) - 1) & data;
		data >>= c;
		bitdata |= mask << bitcount;
		bitcount += c;
		nbits -= c;
		if (bitcount >= 8) {
			if (lzw->outpos >= lzw->outmax) {
				lzw->error = 2;
				return -1;
			}
			lzw->outptr[lzw->outpos] = (unsigned char)bitdata;
			lzw->outpos++;
			bitcount = 0;
			bitdata = 0;
		}
	}
	lzw->outbits = bitcount;
	lzw->outdata = bitdata;
	return 0;
}

//---------------------------------------------------------------------
// flush bits
//---------------------------------------------------------------------
static inline int ilzw_bits_flush(iLzwDesc *lzw)
{
	ilzw_bits_put(lzw, -1, 0);
	return 0;
}

//---------------------------------------------------------------------
// clear dictionary
//---------------------------------------------------------------------
static void ilzw_clear_table(iLzwDesc *lzw)
{
	int i;
	lzw->empty_string = lzw->cc + 2;
	lzw->curr_bit_size = lzw->bit_size + 1;
	lzw->bit_overflow = 0;
	if (lzw->mode == ILZW_MODE_COMPRESS) {
		for (i = 0; i < ILZW_TABSIZE; i++) {
			lzw->hash[i] = -1;
		}
	}
}

//---------------------------------------------------------------------
// initialize lzw
//---------------------------------------------------------------------
void ilzw_init(iLzwDesc *lzw, int mode, int bits)
{
	int i;
	lzw->inptr = NULL;
	lzw->outptr = NULL;
	lzw->inbits = 0;
	lzw->indata = 0;
	lzw->outbits = 0;
	lzw->outdata = 0;
	lzw->inpos = 0;
	lzw->inmax = 0;
	lzw->outpos = 0;
	lzw->outmax = 0;
	lzw->mode = mode;
	lzw->bit_size = (bits < 2)? 2 : ((bits > 8)? 8 : bits);
	lzw->cc = (1 << lzw->bit_size);
	for (i = 0; i < lzw->cc; i++) {
		ilzw_base(lzw, i) = -1;
		ilzw_size(lzw, i) = 1;
		ilzw_newc(lzw, i) = (unsigned char)i;
		ilzw_next(lzw, i) = -1;
	}
	ilzw_clear_table(lzw);
}

// get string 
static void ilzw_get_string(iLzwDesc *lzw, int num)
{
	unsigned char *string = lzw->string;
	int i;
	if (num < lzw->cc) {
		lzw->string_length = 1;
		string[0] = ilzw_newc(lzw, num);
	}	else {
		i = ilzw_size(lzw, num);
		lzw->string_length = i;
		for (; i > 0; ) {
			i--;
			string[i] = ilzw_newc(lzw, num);
			num = ilzw_base(lzw, num);
		}
	}
}

// output string
static int ilzw_output_string(iLzwDesc *lzw)
{
	size_t canwrite = lzw->outmax - lzw->outpos;
	size_t string_length = (size_t)lzw->string_length;
	if (string_length == 0) return -1;
	if (string_length > canwrite) {
		lzw->error = 3;
		return -2;
	}
	memcpy(lzw->outptr + lzw->outpos, lzw->string, string_length);
	lzw->outpos += string_length;
	return 0;
}

//---------------------------------------------------------------------
// LZW 单帧压缩
// lzw      - 压缩字典结构
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
//---------------------------------------------------------------------
long ilzw_frame_deflate(iLzwDesc *lzw, const unsigned char *in, long insize, 
	unsigned char *out, long outsize)
{
	unsigned char *lptr = (unsigned char*)in;
	long string_buffer, length, hash, i;
	int written, codenum, mask;
	short *next = lzw->hash;

	#define ilzw_write(lzw, c) ilzw_bits_put(lzw, c, lzw->curr_bit_size)

	lzw->error = 0;
	written = 0;
	mask = (1 << lzw->bit_size) - 1;

	lzw->outptr = (unsigned char*)out;
	lzw->outpos = 0;
	lzw->outmax = outsize;

	if (insize == 0) {
		return 0;
	}

	string_buffer = 0;

	for (length = 0; insize > 0; insize--, lptr++) {
		int c = lptr[0] & mask;
		length++;
		if (length == 1) {
			string_buffer = c;
			hash = c;
			continue;
		}

		hash = ((string_buffer << (lzw->bit_size - 1)) | c) & ILZW_MASK;
		length++;

		for (i = next[hash]; i >= 0; i = ilzw_next(lzw, i)) {
			if (ilzw_base(lzw, i) != (unsigned int)string_buffer) continue;
			if (ilzw_newc(lzw, i) == (unsigned char)c) break;
		}

		if (i >= 0) {
			string_buffer = i;
			written = 0;
		}	else {
			ilzw_write(lzw, string_buffer);
			codenum = lzw->empty_string++;
			written = 1;
			if (codenum < ILZW_TABSIZE) {
				ilzw_base(lzw, codenum) = (unsigned short)string_buffer;
				ilzw_newc(lzw, codenum) = c;
				ilzw_size(lzw, codenum) = ilzw_size(lzw, codenum) + 1;
				ilzw_next(lzw, codenum) = next[hash];
				next[hash] = codenum;
			}

			string_buffer = c;
			length = 1;
	
			if (codenum >= ILZW_TABSIZE) {
				ilzw_write(lzw, string_buffer);
				ilzw_write(lzw, lzw->cc);
				ilzw_clear_table(lzw);
				for (i = 0; i < ILZW_TABSIZE; i++) next[i] = -1;
				length = 0;
				codenum = 0;
			}

			if (codenum >= (1 << lzw->curr_bit_size)) 
				lzw->curr_bit_size++;
		}
	}

	ilzw_write(lzw, string_buffer);
	ilzw_write(lzw, lzw->cc + 1);
	ilzw_write(lzw, -1);

	if (lzw->outpos < lzw->outmax) {
		lzw->outptr[lzw->outpos] = 0;
		lzw->outpos++;
	}

	return (long)lzw->outpos;
}


//---------------------------------------------------------------------
// LZW 单帧解压
// lzw      - 解压字典结构
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
//---------------------------------------------------------------------
long ilzw_frame_inflate(iLzwDesc *lzw, const unsigned char *in, long insize, 
	unsigned char *out, long outsize)
{
	unsigned char *string = lzw->string;
	long retval, old, i;

	#define ilzw_getcode(lzw) { \
		lzw->code = ilzw_bits_get(lzw, lzw->curr_bit_size); }

	lzw->inptr = (unsigned char*)in;
	lzw->inpos = 0;
	lzw->inmax = (size_t)insize;
	lzw->outptr = (unsigned char*)out;
	lzw->outpos = 0;
	lzw->outmax = (size_t)outsize;
	lzw->error = 0;

	if (insize == 0) {
		return 0;
	}

	ilzw_getcode(lzw);
	if (lzw->code == lzw->cc) {
		ilzw_clear_table(lzw);
		ilzw_getcode(lzw);
	}
	if (lzw->code < 0) 
		return -1;

	if (lzw->code == lzw->cc + 1) {
		return 0;
	}

	ilzw_get_string(lzw, lzw->code);
	ilzw_output_string(lzw);
	old = lzw->code;

	for (retval = 0; ; ) {
		ilzw_getcode(lzw);
		if (lzw->code == lzw->cc + 1) {
			retval = (long)lzw->outpos;
			break;
		}
		if (lzw->code < 0) {
			retval = -1;
			break;
		}
		if (lzw->code == lzw->cc) {
			ilzw_clear_table(lzw);
			ilzw_getcode(lzw);
			if (lzw->code < 0) {
				retval = -1;
				break;
			}
			ilzw_get_string(lzw, lzw->code);
			ilzw_output_string(lzw);
			old = lzw->code;
		}
		else {
			if (lzw->code < lzw->empty_string) {
				ilzw_get_string(lzw, lzw->code);
			}	else {
				ilzw_get_string(lzw, old);
				string[ilzw_size(lzw, old)] = string[0];
				lzw->string_length++;
			}
			ilzw_output_string(lzw);
			if (lzw->bit_overflow == 0) {
				i = lzw->empty_string;
				ilzw_base(lzw, i) = (short)old;
				ilzw_size(lzw, i) = ilzw_size(lzw, old) + 1;
				ilzw_newc(lzw, i) = string[0];
				ilzw_next(lzw, i) = -1;
				lzw->empty_string++;
				if (lzw->empty_string == (1 << lzw->curr_bit_size))
					lzw->curr_bit_size++;
				if (lzw->curr_bit_size > ILZW_MAXBITS) {
					lzw->curr_bit_size = ILZW_MAXBITS;
					lzw->bit_overflow = 1;
				}
			}
			old = lzw->code;
		}
	}

	return retval;
}

/* encode auto size unsigned integer */
unsigned char *ilzw_encodeu(unsigned char *ptr, unsigned long v)
{
	unsigned char *p = (unsigned char*)ptr;
	p[0] = (unsigned char)(v & 0x7f);
	if (v <= 0x7ful)
		return ptr + 1;
	p[0] |= 0x80;
	p[1] = (unsigned char)((v >> 7) & 0x7f);
	if (v <= 0x3ffful)
		return ptr + 2;
	p[1] |= 0x80;
	p[2] = (unsigned char)((v >> 14) & 0x7f);
	if (v <= 0x1ffffful) 
		return ptr + 3;
	p[2] |= 0x80;
	p[3] = (unsigned char)((v >> 21) & 0x7f);
	if (v <= 0xffffffful) 
		return ptr + 4;
	p[3] |= 0x80;
	p[4] = (unsigned char)((v >> 28) & 0x7f);
	return ptr + 5;
}

/* decode auto size unsigned integer */
const unsigned char *ilzw_decodeu(const unsigned char *ptr, unsigned long *v)
{
	unsigned char *p = (unsigned char*)ptr;
	unsigned long value = 0;
	value |= ((unsigned long)(p[0] & 0x7f));
	if (p[0] & 0x80) {
		p++;
		value |= ((unsigned long)(p[0] & 0x7f)) << 7;
		if (p[0] & 0x80) {
			p++;
			value |= ((unsigned long)(p[0] & 0x7f)) << 14;
			if (p[0] & 0x80) {
				p++;
				value |= ((unsigned long)(p[0] & 0x7f)) << 21;
				if (p[0] & 0x80) {
					p++;
					value |= ((unsigned long)(p[0] & 0x7f)) << 28;
				}
			}
		}
	}
	*v = value;
	p++;
	return p;
}


// LZW 压缩帧（不复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_deflate(iLzwDesc *lzw, const void *in, long insize, 
	void *out, long outsize)
{
	unsigned char *input = (unsigned char*)in;
	unsigned char *output = (unsigned char*)out;
	long size, intlen, i;
	size = ilzw_frame_deflate(lzw, input, insize, output, outsize);
	if (insize == 0) return 0;
	if (size < 0 || size + 8 >= outsize) 
		return -1;
	if (size < insize) {
		unsigned long length = (unsigned long)size;
		output += size;
		length = length * 2 + 0;
		intlen = (long)(ilzw_encodeu(output, length) - output);
		output -= size;
		for (i = size - 1; i >= 0; i--) 
			output[i + intlen] = output[i];
		ilzw_encodeu(output, length);
	}	else {
		unsigned long length = (unsigned long)insize;
		length = length * 2 + 1;
		intlen = (long)(ilzw_encodeu(output, length) - output);
		for (i = insize - 1; i >= 0; i--) 
			output[i + intlen] = input[i];
		size = insize;
	}
	return size + intlen;
}


// LZW 解压帧（不复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_inflate(iLzwDesc *lzw, const void *in, long insize, 
	void *out, long outsize)
{
	const unsigned char *input = (const unsigned char*)in;
	unsigned char *output = (unsigned char*)out;
	unsigned long length;
	long size, i;
	if (insize == 0) return 0;
	input = ilzw_decodeu(input, &length);
	size = (long)(length / 2);
	if (input + size > (unsigned char*)in + insize) {
		return -1;
	}
	if ((length & 1) == 0) {
		size = ilzw_frame_inflate(lzw, input, size, output, outsize);
		if (size < 0) 
			return -2;
	}	else {
		if (outsize < size)
			return -3;
		for (i = size - 1; i >= 0; i--) 
			output[i] = input[i];
	}
	return size;
}


// LZW 复位字典
void ilzw_reset(iLzwDesc *lzw)
{
	ilzw_clear_table(lzw);
}


// LZW 低层次压缩（每次复位字典，线程不安全）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// workmem  - 外部提供大小为 sizeof(workmem)的工作内存
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_compress_1(const void *in, long insize, void *out, long outsize,
	void *workmem)
{
	const unsigned char *input = (const unsigned char*)in;
	unsigned char *output = (unsigned char*)out;
	long retval, bits, i, max;
	iLzwDesc *lzw = (iLzwDesc*)workmem;
	if (workmem == NULL) {
		return sizeof(iLzwDesc);
	}
	for (i = 0, max = 0; i < insize; i++) 
		if (input[i] > max) max = input[i];
	for (bits = 1; bits < 8; bits++) 
		if ((1 << bits) > max) break;

	ilzw_init(lzw, ILZW_MODE_COMPRESS, bits);

	if (outsize < 1) 
		return -1;

	output[0] = (unsigned char)bits;
	retval = ilzw_deflate(lzw, in, insize, output + 1, outsize - 1);

	if (retval < 0)
		return -2;

	return retval + 1;
}

// LZW 简易解压（每次复位字典，线程安全）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// workmem  - 外部提供大小为 sizeof(workmem)的工作内存
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_decompress_1(const void *in, long insize, void *out, long outsize,
	void *workmem)
{
	const unsigned char *input = (const unsigned char*)in;
	iLzwDesc *lzw = (iLzwDesc*)workmem;
	long bits;

	if (workmem == NULL) {
		return sizeof(iLzwDesc);
	}

	if (insize < 1)
		return 0;

	bits = input[0];

	ilzw_init(lzw, ILZW_MODE_DECOMPRESS, bits);

	return ilzw_inflate(lzw, input + 1, insize - 1, out, outsize);
}


// LZW 简易压缩（每次复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_compress(const void *in, long insize, void *out, long outsize)
{
	iLzwDesc lzw;
	if (out == NULL) return ilzw_deflate_maxsize(insize);
	return ilzw_compress_1(in, insize, out, outsize, &lzw);
}

// LZW 简易解压（每次复位字典）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小
// 返回压缩后的大小，如果小于零代表输出内存不够 
long ilzw_decompress(const void *in, long insize, void *out, long outsize)
{
	iLzwDesc lzw;
	return ilzw_decompress_1(in, insize, out, outsize, &lzw);
}


#ifdef _MSC_VER
#pragma warning(disable:4267)
#endif

//---------------------------------------------------------------------
// LZO 实现 - 来自 minilzo
//---------------------------------------------------------------------
static int _ilzo1x_1_do_compress(const unsigned char *in, size_t in_len,
    unsigned char *out, size_t *out_len, void *wrkmem)
{
    const unsigned char * const in_end = in + in_len;
    const unsigned char * const ip_end = in + in_len - 8 - 5;
    const unsigned char **const dict = (const unsigned char **const)wrkmem;
    const unsigned char *ip = in, *ii = ip;
    const unsigned char *end, *m, *m_pos;
    size_t m_off, m_len, dindex;
    unsigned char *op = out;
    ip += 4;
    for (;;) {
    dindex = ((size_t)(0x21 * (((((((size_t)(((ip)+1)[2]) << (6)) ^ 
        ((ip)+1)[1]) << (5)) ^ ((ip)+1)[0]) << 
        (5)) ^ (ip)[0])) >> 5) & ((1u << 14) - 1);
    m_pos = dict[dindex];
    if (m_pos < in)
        goto literal;
    if (ip == m_pos || ((size_t)(ip - m_pos) > 0xbfff))
        goto literal;
    m_off = ip - m_pos;
    if (m_off <= 0x0800 || m_pos[3] == ip[3])
        goto try_match;
    dindex = (dindex & (((1u << 14) - 1) & 0x7ff)) ^ 
        (((((1u << 14) - 1) >> 1) + 1) | 0x1f);
    m_pos = dict[dindex];
    if (m_pos < in)
        goto literal;
    if (ip == m_pos || ((size_t)(ip - m_pos) > 0xbfff))
        goto literal;
    m_off = ip - m_pos;
    if (m_off <= 0x0800 || m_pos[3] == ip[3])
        goto try_match;
    goto literal;
try_match:
    if ((*((const unsigned short *)m_pos)) == 
        (*((const unsigned short *)ip))) {
        if ((m_pos[2] == ip[2]))
        goto match;
    }
literal:
    dict[dindex] = ip;
    ++ip;
    if ((ip >= ip_end))
        break;
    continue;
match:
    dict[dindex] = ip;
    if (ip != ii) {
        size_t t = ip - ii;
        if (t <= 3) {
            op[-2] |= t;
        } else if (t <= 18) {
            *op++ = (t - 3);
        } else {
            size_t tt = t - 18;
            *op++ = 0;
            while (tt > 255) {
                tt -= 255;
                *op++ = 0;
            }
            *op++ = tt;
        }
        do {
            *op++ = *ii++;
        } while (--t > 0);
    }
    ip += 3;
    if (m_pos[3] != *ip++ || m_pos[4] != *ip++
        || m_pos[5] != *ip++ || m_pos[6] != *ip++
        || m_pos[7] != *ip++ || m_pos[8] != *ip++) {
        --ip;
        m_len = ip - ii;
        if (m_off <= 0x0800) {
        m_off -= 1;
        *op++ = (((m_len - 1) << 5)
                | ((m_off & 7) << 2));
        *op++ = (m_off >> 3);
        } else if (m_off <= 0x4000) {
        m_off -= 1;
        *op++ = (32 | (m_len - 2));
        goto m3_m4_offset;
        } else {
        m_off -= 0x4000;
        *op++ = (16 | ((m_off & 0x4000) >> 11)
                | (m_len - 2));
        goto m3_m4_offset;
        }
    } else {
        end = in_end;
        m = m_pos + 8 + 1;
        while (ip < end && *m == *ip) {
        m++;
        ip++;
        }
        m_len = ip - ii;
        if (m_off <= 0x4000) {
        m_off -= 1;
        if (m_len <= 33) {
            *op++ = (32 | (m_len - 2));
        } else {
            m_len -= 33;
            *op++ = 32 | 0;
            goto m3_m4_len;
        }
        } else {
        m_off -= 0x4000;
        if (m_len <= 9) {
            *op++ = (16
                | ((m_off & 0x4000) >> 11)
                | (m_len - 2));
        } else {
            m_len -= 9;
            *op++ = (16
                | ((m_off & 0x4000) >> 11));
m3_m4_len:
            while (m_len > 255) {
            m_len -= 255;
            *op++ = 0;
            }
            *op++ = (m_len);
        }
        }
m3_m4_offset:
        *op++ = ((m_off & 63) << 2);
        *op++ = (m_off >> 6);
    }
    ii = ip;
    if ((ip >= ip_end))
        break;
    }
    *out_len = op - out;
    return (int)(in_end - ii);
}


// LZO 原始压缩
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// wrkmem   - 需要提供的工作内存 （大小为 ILZO_WRKMEM_SIZE）
// 成功返回0，其他是错误
int ilzo1x_1_compress(const unsigned char *in, size_t in_len, 
    unsigned char *out, size_t *out_len, void *wrkmem)
{
    const unsigned char *ii;
    unsigned char *op = out;
    size_t t;
	if (out == NULL) {
		*out_len = (in_len + in_len / 16 + 64 + 3);
	}
    if ((in_len <= 8 + 5)) {
        t = in_len;
    } else {
        t = _ilzo1x_1_do_compress(in, in_len, op, out_len, wrkmem);
        op += *out_len;
    }
    if (t > 0) {
    ii = in + in_len - t;
    if (op == out && t <= 238) {
        *op++ = (17 + t);
    } else if (t <= 3) {
        op[-2] |= t;
    } else if (t <= 18) {
        *op++ = (t - 3);
    } else {
        size_t tt = t - 18;
        *op++ = 0;
        while (tt > 255) {
            tt -= 255;
            *op++ = 0;
        }
        *op++ = tt;
    }
    do {
        *op++ = *ii++;
    } while (--t > 0);
    }
    *op++ = 16 | 1;
    *op++ = 0;
    *op++ = 0;
    *out_len = op - out;
    return 0;
}


// LZO 原始解压
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// 成功返回0，其他是错误
int ilzo1x_decompress_safe(const unsigned char *in, size_t in_len,
    unsigned char *out, size_t *out_len)
{
    const unsigned char * const ip_end = in + in_len;
    unsigned char * const op_end = out + *out_len;
    const unsigned char *ip = in, *m_pos;
    unsigned char *op = out;
    size_t t;
    *out_len = 0;
    if (*ip > 17) {
    t = *ip++ - 17;
    if (t < 4)
        goto match_next;
    if (((size_t)(op_end - op) < (t)))
        goto output_overrun;
    if (((size_t)(ip_end - ip) < (t + 1)))
        goto input_overrun;
    do {
        *op++ = *ip++;
    } while (--t > 0);
    goto first_literal_run;
    }
    while ((ip < ip_end)) {
    t = *ip++;
    if (t >= 16)
        goto match;
    if (t == 0) {
        if (((size_t)(ip_end - ip) < (1)))
            goto input_overrun;
        while (*ip == 0) {
            t += 255;
            ip++;
            if (((size_t)(ip_end - ip) < (1)))
                goto input_overrun;
        }
        t += 15 + *ip++;
    }
    if (((size_t)(op_end - op) < (t + 3)))
        goto output_overrun;
    if (((size_t)(ip_end - ip) < (t + 4)))
        goto input_overrun;
    *(unsigned char*)((char*)op + 0) = 
        *(unsigned char*)((char*)ip + 0); 
    *(unsigned char*)((char*)op + 1) = 
        *(unsigned char*)((char*)ip + 1); 
    *(unsigned char*)((char*)op + 2) = 
        *(unsigned char*)((char*)ip + 2); 
    *(unsigned char*)((char*)op + 3) = 
        *(unsigned char*)((char*)ip + 3); 
    op += 4;
    ip += 4;
    if (--t > 0) {
        if (t >= 4) {
            do {
                *(unsigned char*)((char*)op + 0) = 
                *(unsigned char*)((char*)ip + 0);
                *(unsigned char*)((char*)op + 1) = 
                *(unsigned char*)((char*)ip + 1);
                *(unsigned char*)((char*)op + 2) = 
                *(unsigned char*)((char*)ip + 2);
                *(unsigned char*)((char*)op + 3) = 
                *(unsigned char*)((char*)ip + 3);
                op += 4;
                ip += 4;
                t -= 4;
            } while (t >= 4);
            if (t > 0) {
                do {
                    *op++ = *ip++;
                } while (--t > 0);
            }
        } else {
            do {
                *op++ = *ip++;
            } while (--t > 0);
        }
    }
first_literal_run:
    t = *ip++;
    if (t >= 16)
        goto match;
    m_pos = op - (1 + 0x0800);
    m_pos -= t >> 2;
    m_pos -= *ip++ << 2;
    if ((m_pos < out || m_pos >= op))
        goto lookbehind_overrun;
    if (((size_t)(op_end - op) < (3)))
        goto output_overrun;
    *op++ = *m_pos++;
    *op++ = *m_pos++;
    *op++ = *m_pos;
    goto match_done;
    do {
match:
        if (t >= 64) {
        m_pos = op - 1;
        m_pos -= (t >> 2) & 7;
        m_pos -= *ip++ << 3;
        t = (t >> 5) - 1;
        if ((m_pos < out || m_pos >= op))
            goto lookbehind_overrun;
        if (((size_t)(op_end - op) < (t + 3 - 1)))
            goto output_overrun;
        goto copy_match;
        } else if (t >= 32) {
        t &= 31;
        if (t == 0) {
            if (((size_t)(ip_end - ip) < (1)))
                goto input_overrun;
            while (*ip == 0) {
              t += 255;
              ip++;
              if (((size_t)(ip_end - ip) < (1)))
                 goto input_overrun;
            }
            t += 31 + *ip++;
        }
        m_pos = op - 1;
        m_pos -= (*(unsigned short*)(ip)) >> 2;
        ip += 2;
        } else if (t >= 16) {
        m_pos = op;
        m_pos -= (t & 8) << 11;
        t &= 7;
        if (t == 0) {
            if (((size_t)(ip_end - ip) < (1)))
                goto input_overrun;
            while (*ip == 0) {
            t += 255;
            ip++;
            if (((size_t)(ip_end - ip) < (1)))
                goto input_overrun;
            }
            t += 7 + *ip++;
        }
        m_pos -= (*(unsigned short*)(ip)) >> 2;
        ip += 2;
        if (m_pos == op)
            goto eof_found;
        m_pos -= 0x4000;
        } else {
        m_pos = op - 1;
        m_pos -= t >> 2;
        m_pos -= *ip++ << 2;
        if ((m_pos < out || m_pos >= op))
            goto lookbehind_overrun;
        if (((size_t)(op_end - op) < (2)))
            goto output_overrun;
        *op++ = *m_pos++;
        *op++ = *m_pos;
        goto match_done;
        }
        if ((m_pos < out || m_pos >= op))
        goto lookbehind_overrun;
        if (((size_t)(op_end - op) < (t + 3 - 1)))
        goto output_overrun;
        if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4) {
        *(unsigned char*)((char*)op + 0) = 
            *(unsigned char*)((char*)m_pos + 0); 
        *(unsigned char*)((char*)op + 1) = 
            *(unsigned char*)((char*)m_pos + 1); 
        *(unsigned char*)((char*)op + 2) = 
            *(unsigned char*)((char*)m_pos + 2); 
        *(unsigned char*)((char*)op + 3) = 
            *(unsigned char*)((char*)m_pos + 3);
        op += 4;
        m_pos += 4;
        t -= 4 - (3 - 1);
        do {
            *(unsigned char*)((char*)op + 0) = 
            *(unsigned char*)((char*)m_pos + 0);
            *(unsigned char*)((char*)op + 1) = 
            *(unsigned char*)((char*)m_pos + 1);
            *(unsigned char*)((char*)op + 2) =
            *(unsigned char*)((char*)m_pos + 2); 
            *(unsigned char*)((char*)op + 3) = 
            *(unsigned char*)((char*)m_pos + 3);
            op += 4;
            m_pos += 4;
            t -= 4;
        } while (t >= 4);
        if (t > 0)
            do {
                *op++ = *m_pos++;
            } while (--t > 0);
        } else {
copy_match:
            *op++ = *m_pos++;
            *op++ = *m_pos++;
            do {
                *op++ = *m_pos++;
            } while (--t > 0);
        }
match_done:
        t = ip[-2] & 3;
        if (t == 0)
            break;
match_next:
        if (((size_t)(op_end - op) < (t)))
            goto output_overrun;
        if (((size_t)(ip_end - ip) < (t + 1)))
            goto input_overrun;
        *op++ = *ip++;
        if (t > 1) {
            *op++ = *ip++;
            if (t > 2)
                *op++ = *ip++;
        }
        t = *ip++;
    } while (ip < ip_end);
    }
    *out_len = op - out;
    return 1;
eof_found:
    *out_len = op - out;
    return (ip == ip_end ? 0 : (ip < ip_end ? 2 : 3));
input_overrun:
    *out_len = op - out;
    return 3;
output_overrun:
    *out_len = op - out;
    return 4;
lookbehind_overrun:
    *out_len = op - out;
    return 5;
}


// LZO 简易压缩（自动分配临时工作内存）
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// 成功返回0，其他是错误
long ilzo_compress(const void *in, long insize, void *out, long outsize)
{
	size_t size;
	char wrkmem[ILZO_WRKMEM_SIZE];
	if (out == NULL) return (insize + insize / 8 + 128);
	if (ilzo1x_1_compress((const unsigned char*)in, (size_t)insize, 
		(unsigned char*)out, &size, wrkmem) != 0) 
		return -1;
	return (long)size;
}


// LZO 简易解压
// in       - 输入数据指针
// insize   - 输入数据大小
// out      - 输出数据指针
// outsize  - 输出数据大小 (指针，将会返回输出数据大小）
// 成功返回0，其他是错误
long ilzo_decompress(const void *in, long insize, void *out, long outsize)
{
	size_t size = (size_t)outsize;
	int hr;
	hr = ilzo1x_decompress_safe((const unsigned char*)in, (size_t)insize, 
		(unsigned char*)out, &size);
	if (hr != 0) {
		return -1;
	}
	return (long)size;
}


//---------------------------------------------------------------------
// 简单封装
//---------------------------------------------------------------------
struct iPkMethodTable 
{
	iCompressProc compress;
	iDecompressProc decompress;
};

static struct iPkMethodTable iPkMethods[16] = {
	{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
	{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
	{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
	{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },
};


// LZW / LZO 压缩
// method = ILZX_METHOD_LZW / ILZX_METHOD_LZO
// in - 输入数据指针
// insize - 输入数据大小
// out - 输出数据指针，out为 NULL时返回压缩后输出最大大小，便于安排缓存
// outsize - 输出数据大小
// 返回压缩后数据实际大小，如果 outsize不够，返回 -1，其他错误 < 0
long ipk_compress(int method, const void *in, long insize, 
	void *out, long outsize, int level)
{
	unsigned char *output = (unsigned char*)out;
	unsigned char *p = output;
	long retval = -20;
	long headlen;

	if (insize == 0) return 0;

	if (out == NULL) {
		unsigned char head[32];
		long needed;
		p = ilzw_encodeu(head, insize);
		headlen = (long)(p - head);
		if (method == 0) {
			needed = ilzw_deflate_maxsize(insize) + headlen;
		}
		else if (method == 1) {
			needed = ilzo_compress(in, insize, NULL, 0) + headlen;
		}
		else {
			iCompressProc compressor = iPkMethods[method].compress;
			if (compressor == NULL) return -20;
			needed = compressor(in, insize, NULL, outsize, level) + headlen;
		}
		if (needed < 32) needed = 32;
		return needed;
	}

	if (outsize < 32) return -10;
	if (method < 0 || method >= 16) return -11;

	p = ilzw_encodeu(p, insize);
	headlen = (long)(p - output);

	if (method == 0) {
		retval = ilzw_compress(in, insize, p, outsize - headlen);
	}
	else if (method == 1) {
		retval = ilzo_compress(in, insize, p, outsize - headlen);
	}
	else {
		iCompressProc compressor = iPkMethods[method].compress;
		if (compressor == NULL) return -20;
		retval = compressor(in, insize, NULL, outsize - headlen, level);
	}
	if (retval < 0) {
		return retval;
	}

	p += retval;

	return (long)(p - output);
}


// LZW / LZO 解压缩
// method = ILZX_METHOD_LZW / ILZX_METHOD_LZO
// in - 输入数据指针
// insize - 输入数据大小
// out - 输出数据指针，如果 out == NULL，将直接返回解压后数据的大小
// outsize - 输出数据大小
// 返回解压后数据实际大小，如果 outsize不够，返回 -1，其他错误返回 < 0
long ipk_decompress(int method, const void *in, long insize, void *out,
	long outsize)
{
	const unsigned char *input = (const unsigned char*)in;
	const unsigned char *p = input;
	unsigned long xsize;
	long headlen;
	long retval;

	if (insize == 0) return 0;
	if (method < 0 || method >= 16) return -11;

	p = ilzw_decodeu(p, &xsize);

	headlen = (long)(p - input);

	if (out == NULL) {
		return (long)xsize;
	}

	if (outsize < (long)xsize) {
		return -1;
	}

	retval = -2;
	outsize = (long)xsize;

	if (method == 0) {
		retval = ilzw_decompress(p, insize - headlen, out, outsize);
	}
	else if (method == 1) {
		retval = ilzo_decompress(p, insize - headlen, out, outsize);
	}
	else {
		iDecompressProc decompressor = iPkMethods[method].decompress;
		if (decompressor == NULL) return -20;
		retval = decompressor(p, insize - headlen, out, outsize);
	}

	if (retval < 0) {
		return retval;
	}

	return xsize;
}


// 安装新的压缩/解压器
int ipk_install_method(int method, iCompressProc p1, iDecompressProc p2)
{
	if (method <= 1 || method >= 16) return -1;
	iPkMethods[method].compress = p1;
	iPkMethods[method].decompress = p2;
	return 0;
}



