//=====================================================================
//
// asmpure.c - assembly pure compiler
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "asmpure.h"

#ifndef __CENCODING_H__
#define __CENCODING_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

//---------------------------------------------------------------------
// Platform Word Size Detect
//---------------------------------------------------------------------
#if (!defined(__CUINT32_DEFINED)) && (!defined(__CINT32_DEFINED))
#define __CUINT32_DEFINED
#define __CINT32_DEFINED
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) \
	 || defined(__i386__) || defined(__i386) || defined(_M_X86)
	typedef unsigned int cuint32;
	typedef int cint32;
#elif defined(__MACOS__)
	typedef UInt32 cuint32;
	typedef Int32 cint32;
#elif defined(__APPLE__) && defined(__MACH__)
	#include <sys/types.h>
	typedef u_int32_t cuint32;
	typedef int32_t cint32;
#elif defined(__BEOS__)
	#include <sys/inttypes.h>
	typedef u_int32_t cuint32;
	typedef int32_t cint32;
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || \
	defined(__amd64) || defined(_M_IA64) || defined(_M_AMD64)
	typedef unsigned int cuint32;
	typedef int cint32;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	typedef unsigned __int32 cuint32;
	typedef __int32 cint32;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t cuint32;
	typedef int32_t cint32;
#else 
	typedef unsigned long cuint32;     
	typedef long cint32;
#endif
#endif

#ifndef __CINT8_DEFINED
#define __CINT8_DEFINED
typedef char cint8;
#endif

#ifndef __CUINT8_DEFINED
#define __CUINT8_DEFINED
typedef unsigned char cuint8;
#endif

#ifndef __CUINT16_DEFINED
#define __CUINT16_DEFINED
typedef unsigned short cuint16;
#endif

#ifndef __CINT16_DEFINED
#define __CINT16_DEFINED
typedef short CINT16;
#endif

#ifndef __CINT64_DEFINED
#define __CINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 cint64;
#else
typedef long long cint64;
#endif
#endif

#ifndef __CUINT64_DEFINED
#define __CUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 cuint64;
#else
typedef unsigned long long cuint64;
#endif
#endif

#ifndef INLINE
#ifdef __GNUC__

#if __GNUC_MINOE__ >= 1  && __GNUC_MINOE__ < 4
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

typedef cuint8 cbyte;


//---------------------------------------------------------------------
// CReg
//---------------------------------------------------------------------
enum CRegID
{
	REG_UNKNOWN = -1,
	E_AL = 0, E_AX = 0, E_EAX = 0, E_ST0 = 0, E_MM0 = 0, E_XMM0 = 0,
	E_CL = 1, E_CX = 1, E_ECX = 1, E_ST1 = 1, E_MM1 = 1, E_XMM1 = 1,
	E_DL = 2, E_DX = 2, E_EDX = 2, E_ST2 = 2, E_MM2 = 2, E_XMM2 = 2,
	E_BL = 3, E_BX = 3, E_EBX = 3, E_ST3 = 3, E_MM3 = 3, E_XMM3 = 3,
	E_AH = 4, E_SP = 4, E_ESP = 4, E_ST4 = 4, E_MM4 = 4, E_XMM4 = 4,
	E_CH = 5, E_BP = 5, E_EBP = 5, E_ST5 = 5, E_MM5 = 5, E_XMM5 = 5,
	E_DH = 6, E_SI = 6, E_ESI = 6, E_ST6 = 6, E_MM6 = 6, E_XMM6 = 6,
	E_BH = 7, E_DI = 7, E_EDI = 7, E_ST7 = 7, E_MM7 = 7, E_XMM7 = 7,
	E_R0 = 0, E_R1 = 1, E_R2 = 2, E_R3 = 3, E_R4 = 4, E_R5 = 5,
	E_R6 = 6, E_R7 = 7, E_R8 = 8, E_R9 = 9, E_R10 = 10, E_R11 = 11,
	E_R12 = 12, E_R13 = 13, E_R14 = 14, E_R15 = 15
};

enum CSMod
{
	MOD_NO_DISP = 0,
	MOD_BYTE_DISP = 1,
	MOD_DWORD_DISP = 2,
	MOD_REG = 3
};

enum CScale
{
	SCALE_UNKNOWN = 0,
	SCALE_1 = 0,
	SCALE_2 = 1,
	SCALE_4 = 2,
	SCALE_8 = 3
};


//---------------------------------------------------------------------
// CEncoding 
//---------------------------------------------------------------------
struct CEncoding
{
	char *label;
	char *reference;
	char *message;
	char *data;
	int size;
	int align;
	int relative;

	struct {
		unsigned char P1 : 1;
		unsigned char P2 : 1;
		unsigned char P3 : 1;
		unsigned char P4 : 1;
		unsigned char REX : 1;
		unsigned char O3 : 1;
		unsigned char O2 : 1;
		unsigned char O1 : 1;
		unsigned char modRM : 1;
		unsigned char SIB : 1;
		unsigned char D1 : 1;
		unsigned char D2 : 1;
		unsigned char D3 : 1;
		unsigned char D4 : 1;
		unsigned char I1 : 1;
		unsigned char I2 : 1;
		unsigned char I3 : 1;
		unsigned char I4 : 1;		
	}	format;

	unsigned char P1;   // Prefixes
	unsigned char P2;
	unsigned char P3;
	unsigned char P4;

	struct {
		union {
			struct 	{
				unsigned char B : 1;
				unsigned char X : 1;
				unsigned char R : 1;
				unsigned char W : 1;
				unsigned char prefix : 4;
			};
			unsigned char b;
		};
	}	REX;

	unsigned char O1;   // Opcode
	unsigned char O2;
	unsigned char O3;

	struct {
		union {
			struct {
				unsigned char r_m : 3;
				unsigned char reg : 3;
				unsigned char mod : 2;
			};
			unsigned char b;
		};
	}	modRM;

	struct {
		union {
			struct {
				unsigned char base : 3;
				unsigned char index : 3;
				unsigned char scale : 2;
			};
			unsigned char b;
		};
	}	SIB;

	union {
		cint32 displacement;
		struct {
			unsigned char D1;
			unsigned char D2;
			unsigned char D3;
			unsigned char D4;
		};
	};

	union {
		cint32 immediate;
		struct {
			unsigned char I1;
			unsigned char I2;
			unsigned char I3;
			unsigned char I4;
		};
	};
};

typedef struct CEncoding CEncoding;

//---------------------------------------------------------------------
// CEncoding 
//---------------------------------------------------------------------
void cencoding_init(CEncoding *self);
void cencoding_reset(CEncoding *self);
void cencoding_destroy(CEncoding *self);

const char *cencoding_get_label(const CEncoding *self);
const char *cencoding_get_reference(const CEncoding *self);

int cencoding_length(const CEncoding *self);
int cencoding_new_copy(CEncoding *self, const CEncoding *src);

int cencoding_add_prefix(CEncoding *self, unsigned char prefix);
int cencoding_set_immediate(CEncoding *self, int immediate);
int cencoding_set_jump_offset(CEncoding *self, int offset);
void cencoding_set_label(CEncoding *self, const char *label);
void cencoding_set_reference(CEncoding *self, const char *ref);

void cencoding_set_data(CEncoding *self, const void *data, int size);

int cencoding_check_format(const CEncoding *self);
int cencoding_write_code(const CEncoding *self, unsigned char *output);

void cencoding_to_string(const CEncoding *self, char *output);
void cencoding_to_stdout(const CEncoding *self);


#endif



#ifdef _MSC_VER
#pragma warning(disable: 4996)
#pragma warning(disable: 4311)
#endif

void cencoding_reset(CEncoding *self)
{
	if (self->label) free(self->label);
	self->label = NULL;
	if (self->reference) free(self->reference);
	self->reference = NULL;
	if (self->data) free(self->data);
	self->data = NULL;

	self->format.P1 = 0;
	self->format.P2 = 0;
	self->format.P3 = 0;
	self->format.P4 = 0;
	self->format.REX = 0;
	self->format.O3 = 0;
	self->format.O2 = 0;
	self->format.O1 = 0;
	self->format.modRM = 0;
	self->format.SIB = 0;
	self->format.D1 = 0;
	self->format.D2 = 0;
	self->format.D3 = 0;
	self->format.D4 = 0;
	self->format.I1 = 0;
	self->format.I2 = 0;
	self->format.I3 = 0;
	self->format.I4 = 0;

	self->P1 = 0;
	self->P2 = 0;
	self->P3 = 0;
	self->P4 = 0;
	self->REX.b = 0;
	self->O3 = 0;
	self->O2 = 0;
	self->O1 = 0;
	self->modRM.b = 0;
	self->SIB.b = 0;
	self->D1 = 0;
	self->D2 = 0;
	self->D3 = 0;
	self->D4 = 0;
	self->I1 = 0;
	self->I2 = 0;
	self->I3 = 0;
	self->I4 = 0;

	self->immediate = 0;
	self->displacement = 0;
	self->message = (char*)"";
	self->size = 0;
	self->align = 0;
	self->relative = 0;
}

void cencoding_init(CEncoding *self)
{
	self->label = 0;
	self->reference = 0;
	self->data = 0;
	self->size = 0;
	cencoding_reset(self);
	self->O1 = 0xCC;	// breakpoint
	self->format.O1 = 1;	
}

void cencoding_destroy(CEncoding *self)
{
	cencoding_reset(self);
}

const char *cencoding_get_label(const CEncoding *self)
{
	return self->label;
}

const char *cencoding_get_reference(const CEncoding *self)
{
	return self->reference;
}

int cencoding_length(const CEncoding *self)
{
	int length = 0;
	if (self->data && self->size > 0) 
		return self->size;
	if (self->align > 0) 
		return self->align;
	if (self->format.P1)		length++;
	if (self->format.P2)		length++;
	if (self->format.P3)		length++;
	if (self->format.P4)		length++;
	if (self->format.REX)		length++;
	if (self->format.O3)		length++;
	if (self->format.O2)		length++;
	if (self->format.O1)		length++;
	if (self->format.modRM)		length++;
	if (self->format.SIB)		length++;
	if (self->format.D1)		length++;
	if (self->format.D2)		length++;
	if (self->format.D3)		length++;
	if (self->format.D4)		length++;
	if (self->format.I1)		length++;
	if (self->format.I2)		length++;
	if (self->format.I3)		length++;
	if (self->format.I4)		length++;

	return length;
}

int cencoding_new_copy(CEncoding *self, const CEncoding *src)
{
	*self = *src;
	if (src->label) {
		long size = (long)strlen(src->label);
		self->label = (char*)malloc(size + 1);
		assert(self->label);
		memcpy(self->label, src->label, size + 1);
		self->label[size] = 0;
	}
	if (src->reference) {
		long size = (long)strlen(src->reference);
		self->reference = (char*)malloc(size + 1);
		assert(self->reference);
		memcpy(self->reference, src->reference, size + 1);
		self->reference[size] = 0;
	}
	if (src->data) {
		self->data = (char*)malloc(src->size + 1);
		assert(self->data);
		memcpy(self->data, src->data, src->size);
		self->size = src->size;
	}
	return 0;
}

int cencoding_add_prefix(CEncoding *self, cbyte p)
{
	if (!self->format.P1) {
		self->P1 = p;
		self->format.P1 = 1;
	}
	else if (!self->format.P2) {
		self->P2 = p;
		self->format.P2 = 1;
	}
	else if (!self->format.P3) {
		self->P3 = p;
		self->format.P3 = 1;
	}
	else if (!self->format.P4) {
		self->P4 = p;
		self->format.P4 = 1;
	}	else {
		return -1;
	}
	return 0;
}

int cencoding_set_immediate(CEncoding *self, int immediate)
{
	self->immediate = immediate;
	return 0;
}

int cencoding_set_jump_offset(CEncoding *self, int offset)
{
	if ((char)offset != offset && self->format.I2 == 0) {
		self->message = (char*)"Jump offset range too big";
		return -1;
	}
	self->immediate = offset;
	return 0;
}

void cencoding_set_label(CEncoding *self, const char *label)
{
	int size = (int)strlen(label);
	if (self->label) free(self->label);
	self->label = (char*)malloc(size + 1);
	assert(self->label);
	memcpy(self->label, label, size + 1);
}

void cencoding_set_reference(CEncoding *self, const char *ref)
{
	int size = (int)strlen(ref);
	if (self->reference) free(self->reference);
	self->reference = (char*)malloc(size + 1);
	assert(self->reference);
	memcpy(self->reference, ref, size + 1);
}

void cencoding_set_data(CEncoding *self, const void *data, int size)
{
	if (self->data) free(self->data);
	self->data = NULL;
	self->size = 0;
	if (data && size > 0) {
		self->data = (char*)malloc(size + 1);
		assert(self->data);
		self->size = size;
		memcpy(self->data, data, size);
	}
}

int cencoding_check_format(const CEncoding *self)
{
	// Bytes cannot be changed without updating format, 
	// except immediate and displacement
	if ((self->P1 && !self->format.P1) ||
	   (self->P2 && !self->format.P2) ||
	   (self->P3 && !self->format.P3) ||
	   (self->P4 && !self->format.P4) ||
	   (self->REX.b && !self->format.REX) ||
	   (self->O2 && !self->format.O2) ||
	   (self->O1 && !self->format.O1) ||
	   (self->modRM.b && !self->format.modRM) ||
	   (self->SIB.b && !self->format.SIB)) {
		return -1;   
	}

	if ((self->format.P4 && !self->format.P3) ||
	   (self->format.P3 && !self->format.P2) ||
	   (self->format.P2 && !self->format.P1)) {
		return -2;
	}

	if (self->format.O2 &&
	   (self->O2 != 0x0F &&
	    self->O2 != 0xD8 &&
		self->O2 != 0xD9 &&
		self->O2 != 0xDA &&
		self->O2 != 0xDB &&
		self->O2 != 0xDC &&
		self->O2 != 0xDD &&
		self->O2 != 0xDE &&
		self->O2 != 0xDF)) {
		return -3;
	}

	if (self->format.SIB) {
		if(!self->format.modRM) {
			return -4;
		}
		if(self->modRM.r_m != E_ESP) {
			return -5;
		}
	}

	// Byte, word or doubleword
	if ((self->format.D4 && !self->format.D3) ||
	   (self->format.D3 && !self->format.D4) ||
	   (self->format.D3 && !self->format.D2) ||
	   (self->format.D2 && !self->format.D1)) {
		return -6;
	}

	// Byte, word or doubleword
	if ((self->format.I4 && !self->format.I3) ||
	   (self->format.I3 && !self->format.I4) ||
	   (self->format.I3 && !self->format.I2) ||
	   (self->format.I2 && !self->format.I1)) {
		return -7;
	}

	return 0;
}

int cencoding_write_code(const CEncoding *self, unsigned char *output)
{
	unsigned char *start = output;

	#define cencoding_output(b) { if (start) *output = (b); output++; } 

	if (self->data && self->size > 0) {
		if (output) memcpy(output, self->data, self->size);
		return (int)self->size;
	}

	if (self->align > 0) {
		unsigned long linear = (((unsigned long)output) & 0xfffffffful);
		int size = self->align - (linear % self->align);
		for (; size >= 2; size -= 2) {
			cencoding_output(0x66);
			cencoding_output(0x90);
		}
		for (; size > 0; size--) {
			cencoding_output(0x90);
		}
		return (int)(output - start);
	}

	if (self->format.P1)		cencoding_output(self->P1);
	if (self->format.P2)		cencoding_output(self->P2);
	if (self->format.P3)		cencoding_output(self->P3);
	if (self->format.P4)		cencoding_output(self->P4);
	if (self->format.REX)		cencoding_output(self->REX.b);
	if (self->format.O3)		cencoding_output(self->O3);
	if (self->format.O2)		cencoding_output(self->O2);
	if (self->format.O1)		cencoding_output(self->O1);
	if (self->format.modRM)		cencoding_output(self->modRM.b);
	if (self->format.SIB)		cencoding_output(self->SIB.b);
	if (self->format.D1)		cencoding_output(self->D1);
	if (self->format.D2)		cencoding_output(self->D2);
	if (self->format.D3)		cencoding_output(self->D3);
	if (self->format.D4)		cencoding_output(self->D4);
	if (self->format.I1)		cencoding_output(self->I1);
	if (self->format.I2)		cencoding_output(self->I2);
	if (self->format.I3)		cencoding_output(self->I3);
	if (self->format.I4)		cencoding_output(self->I4);

	#undef cencoding_output

	return (int)(output - start);
}


void cencoding_to_string(const CEncoding *self, char *output)
{
	const char *fmt = "0123456789ABCDEF";
	int hr = cencoding_check_format(self);

	assert(hr == 0);

	#define cencoding_format(data) { \
			if (output) { \
				unsigned char ch = (unsigned char)(data & 0xff); \
				*output++ = fmt[ch / 16]; \
				*output++ = fmt[ch % 16]; \
				*output++ = ' '; \
			}	\
		}

	if (self->data) {
		long i;
		for (i = 0; i < self->size; i++) {
			unsigned int bb = (unsigned char)self->data[i];
			cencoding_format(bb);
		}
		*output++ = '\0';
		return;
	}

	if (self->align > 0) {
		*output++ = '\0';
		return;
	}

	if (self->format.P1)		cencoding_format(self->P1);
	if (self->format.P2)		cencoding_format(self->P2);
	if (self->format.P3)		cencoding_format(self->P3);
	if (self->format.P4)		cencoding_format(self->P4);
	if (self->format.REX)		cencoding_format(self->REX.b);
	if (self->format.O3)		cencoding_format(self->O3);
	if (self->format.O2)		cencoding_format(self->O2);
	if (self->format.O1)		cencoding_format(self->O1);
	if (self->format.modRM)		cencoding_format(self->modRM.b);
	if (self->format.SIB)		cencoding_format(self->SIB.b);
	if (self->format.D1)		cencoding_format(self->D1);
	if (self->format.D2)		cencoding_format(self->D2);
	if (self->format.D3)		cencoding_format(self->D3);
	if (self->format.D4)		cencoding_format(self->D4);
	if (self->format.I1)		cencoding_format(self->I1);
	if (self->format.I2)		cencoding_format(self->I2);
	if (self->format.I3)		cencoding_format(self->I3);
	if (self->format.I4)		cencoding_format(self->I4);

	#undef cencoding_format

	*output++ = '\0';
}


void cencoding_to_stdout(const CEncoding *self)
{
	static char text[8192];
	cencoding_to_string(self, text);
	printf("%s\n", text);
}




/*====================================================================*/
/* QUEUE DEFINITION                                                   */
/*====================================================================*/
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

struct IQUEUEHEAD {
	struct IQUEUEHEAD *next, *prev;
};

typedef struct IQUEUEHEAD iqueue_head;


/*--------------------------------------------------------------------*/
/* queue init                                                         */
/*--------------------------------------------------------------------*/
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }
#define IQUEUE_HEAD(name) \
	struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

#define IQUEUE_INIT(ptr) ( \
	(ptr)->next = (ptr), (ptr)->prev = (ptr))

#define IOFFSETOF(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)

#define ICONTAINEROF(ptr, type, member) ( \
		(type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


/*--------------------------------------------------------------------*/
/* queue operation                                                    */
/*--------------------------------------------------------------------*/
#define IQUEUE_ADD(node, head) ( \
	(node)->prev = (head), (node)->next = (head)->next, \
	(head)->next->prev = (node), (head)->next = (node))

#define IQUEUE_ADD_TAIL(node, head) ( \
	(node)->prev = (head)->prev, (node)->next = (head), \
	(head)->prev->next = (node), (head)->prev = (node))

#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

#define IQUEUE_DEL(entry) (\
	(entry)->next->prev = (entry)->prev, \
	(entry)->prev->next = (entry)->next, \
	(entry)->next = 0, (entry)->prev = 0)

#define IQUEUE_DEL_INIT(entry) do { \
	IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0)

#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

#define iqueue_init		IQUEUE_INIT
#define iqueue_entry	IQUEUE_ENTRY
#define iqueue_add		IQUEUE_ADD
#define iqueue_add_tail	IQUEUE_ADD_TAIL
#define iqueue_del		IQUEUE_DEL
#define iqueue_del_init	IQUEUE_DEL_INIT
#define iqueue_is_empty IQUEUE_IS_EMPTY

#define IQUEUE_FOREACH(iterator, head, TYPE, MEMBER) \
	for ((iterator) = iqueue_entry((head)->next, TYPE, MEMBER); \
		&((iterator)->MEMBER) != (head); \
		(iterator) = iqueue_entry((iterator)->MEMBER.next, TYPE, MEMBER))

#define iqueue_foreach(iterator, head, TYPE, MEMBER) \
	IQUEUE_FOREACH(iterator, head, TYPE, MEMBER)

#define iqueue_foreach_entry(pos, head) \
	for( (pos) = (head)->next; (pos) != (head) ; (pos) = (pos)->next )
	

#define __iqueue_splice(list, head) do {	\
		iqueue_head *first = (list)->next, *last = (list)->prev; \
		iqueue_head *at = (head)->next; \
		(first)->prev = (head), (head)->next = (first);		\
		(last)->next = (at), (at)->prev = (last); }	while (0)

#define iqueue_splice(list, head) do { \
	if (!iqueue_is_empty(list)) __iqueue_splice(list, head); } while (0)

#define iqueue_splice_init(list, head) do {	\
	iqueue_splice(list, head);	iqueue_init(list); } while (0)


#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

#endif


//---------------------------------------------------------------------
// CLink
//---------------------------------------------------------------------
struct CLink
{
	struct IQUEUEHEAD head;
	CEncoding encoding;
	unsigned long offset;
	int size;
	int lineno;
};

typedef struct CLink CLink;


//---------------------------------------------------------------------
// CLoader Structure
//---------------------------------------------------------------------
struct CLoader
{
	struct IQUEUEHEAD head;		// link head;
	char *error;
	int errcode;
	int lineno;
	unsigned long linear;
	unsigned char *output;
};

typedef struct CLoader CLoader;


//---------------------------------------------------------------------
// CLink interface
//---------------------------------------------------------------------
static CLink *clink_create(const CEncoding *encoding)
{
	CLink *link;

	link = (CLink*)malloc(sizeof(CLink));
	assert(link);

	iqueue_init(&link->head);
	cencoding_new_copy(&link->encoding, encoding);
	link->offset = 0;
	link->size = 0;

	return link;
}

static void clink_release(CLink *link)
{
	assert(link);
	cencoding_destroy(&link->encoding);
}


//---------------------------------------------------------------------
// CLoader interface
//---------------------------------------------------------------------
CLoader *cloader_create(void)
{
	CLoader *loader;
	loader = (CLoader*)malloc(sizeof(CLoader));
	assert(loader);
	iqueue_init(&loader->head);
	loader->error = (char*)malloc(1024);
	assert(loader->error);
	loader->error[0] = 0;
	loader->errcode = 0;
	loader->linear = 0;
	loader->output = NULL;
	loader->lineno = 0;
	return loader;
}

void cloader_reset(CLoader *loader)
{
	assert(loader);
	while (!iqueue_is_empty(&loader->head)) {
		CLink *link = iqueue_entry(loader->head.next, CLink, head);
		iqueue_del(&link->head);
		clink_release(link);
	}
	loader->error[0] = 0;
	loader->errcode = 0;
	loader->linear = 0;
	loader->output = NULL;
	loader->lineno = 0;
}

void cloader_release(CLoader *loader)
{
	assert(loader);
	cloader_reset(loader);
	if (loader->error) {
		free(loader->error);
		loader->error = NULL;
	}
}

int cloader_new_encoding(CLoader *loader, const CEncoding *encoding)
{
	CLink *link;
	link = clink_create(encoding);
	assert(link);
	link->lineno = ++loader->lineno;
	iqueue_add_tail(&link->head, &loader->head);
	return 0;
}

int cloader_get_codesize(CLoader *loader)
{
	struct IQUEUEHEAD *p;
	int size = 0;
	assert(loader);
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		size += cencoding_length(&link->encoding);
	}
	return size;
}

unsigned long cloader_resolve_label(CLoader *loader, const char *label)
{
	struct IQUEUEHEAD *p;
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		if (cencoding_get_label(encoding)) {
			if (strcmp(encoding->label, label) == 0) {
				return (long)link->offset;
			}
		}
	}
	return 0;
}

int cloader_output(CLoader *loader, unsigned char *output)
{
	struct IQUEUEHEAD *p;
	assert(loader);

	loader->output = output;
	loader->linear = (cuint32)output;

	// encoding instructions
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		int size;
		size = cencoding_write_code(encoding, loader->output);
		link->offset = loader->linear;
		link->size = size;
		loader->linear += size;
		loader->output += size;
	}

	// resolve labels
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		unsigned char *offset = (unsigned char*)link->offset;
		if (cencoding_get_reference(encoding)) {
			const char *label = cencoding_get_reference(encoding);
			long linear = cloader_resolve_label(loader, label);
			if (linear == 0) {
				strncpy(loader->error, "not find label: ", 40);
				strncat(loader->error, label, 100);
				loader->errcode = link->lineno;
				return -1;
			}
			if (encoding->relative == 0) {
				cencoding_set_immediate(encoding, linear);
			}
			else {
				long diff = linear - (link->offset + link->size);
				cencoding_set_jump_offset(encoding, diff);
			}
			cencoding_write_code(encoding, offset);
		}
	}
	
	return 0;
}


void cloader_print(const CLoader *loader)
{
	struct IQUEUEHEAD *p;
	static char line[400];
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		cencoding_to_string(encoding, line);
		printf("%s\n", line);
	}
}



//---------------------------------------------------------------------
// CSpecifierType
//---------------------------------------------------------------------
enum CSpecifierType
{
	CS_UNKNOWN = 0,
	CS_NEAR,
	CS_SHORT = CS_NEAR,
	//	FAR,
	CS_BYTE,
	CS_WORD,
	CS_DWORD,
	CS_QWORD,
	CS_MMWORD = CS_QWORD,
	CS_XMMWORD
};

//---------------------------------------------------------------------
// CSpecifier
//---------------------------------------------------------------------
struct CSpecifier
{
	enum CSpecifierType type;
	const char *notation;
};

typedef struct CSpecifier CSpecifier;


//---------------------------------------------------------------------
// COperandType
//---------------------------------------------------------------------
enum COperandType
{
	O_UNKNOWN	= 0,

	O_VOID	= 0x00000001,

	O_ONE	= 0x00000002,
	O_IMM8	= 0x00000004 | O_ONE,
	O_IMM16	= 0x00000008 | O_IMM8 | O_ONE,
	O_IMM32	= 0x00000010 | O_IMM16 | O_IMM8 | O_ONE,
	O_IMM	= O_IMM32 | O_IMM16 | O_IMM8 | O_ONE,

	O_AL	= 0x00000020,
	O_CL	= 0x00000040,
	O_REG8	= O_CL | O_AL,

	O_AX	= 0x00000080,
	O_DX	= 0x00000100,
	O_CX	= 0x00000200,
	O_REG16	= O_CX | O_DX | O_AX,

	O_EAX		= 0x00000400,
	O_ECX		= 0x00000800,
	O_REG32	= O_ECX | O_EAX,

	// No need to touch these in 32-bit protected mode
	O_CS		= O_UNKNOWN,   
	O_DS		= O_UNKNOWN,
	O_ES		= O_UNKNOWN,
	O_SS		= O_UNKNOWN,
	O_FS		= O_UNKNOWN,
	O_GS		= O_UNKNOWN,
	O_SEGREG	= O_GS | O_FS | O_SS | O_ES | O_DS | O_CS,

	O_ST0		= 0x00001000,
	O_FPUREG	= 0x00002000 | O_ST0,

	// You won't need these in a JIT assembler
	O_CR		= O_UNKNOWN,   
	O_DR		= O_UNKNOWN,
	O_TR		= O_UNKNOWN,

	O_MMREG		= 0x00004000,
	O_XMMREG	= 0x00008000,

	O_REG		=	O_XMMREG | O_MMREG | O_TR | O_DR | O_CR | O_FPUREG | 
					O_SEGREG | O_REG32 | O_REG16 | O_REG8,
	O_MEM8		=	0x00010000,
	O_MEM16		=	0x00020000,
	O_MEM32		=	0x00040000,
	O_MEM64		=	0x00080000,
	O_MEM80		=	O_UNKNOWN,   // Extended double not supported by NT
	O_MEM128	=	0x00100000,
	O_M512B		=	O_UNKNOWN,   // Only for state save/restore instructions
	O_MEM		=	O_M512B | O_MEM128 | O_MEM80 | O_MEM64 | O_MEM32 | 
					O_MEM16 | O_MEM8,
		
	O_XMM32		=	O_MEM32 | O_XMMREG,
	O_XMM64		=	O_MEM64 | O_XMMREG,

	O_R_M8		=	O_MEM8 | O_REG8,
	O_R_M16		=	O_MEM16 | O_REG16,
	O_R_M32		=	O_MEM32 | O_REG32,
	O_R_M64		=	O_MEM64 | O_MMREG,
	O_R_M128	=	O_MEM128 | O_XMMREG,
	O_R_M		=	O_MEM | O_REG,

	O_MOFF8		=	O_UNKNOWN,   // Not supported
	O_MOFF16	=	O_UNKNOWN,   // Not supported
	O_MOFF32	=	O_UNKNOWN   // Not supported
};


//---------------------------------------------------------------------
// COperand
//---------------------------------------------------------------------
struct COperand
{
	enum COperandType type;
	const char *notation;
	union
	{
		cint32 value;			// For immediates
		enum CRegID reg;		// For registers
	};
};

typedef struct COperand COperand;


//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------
extern const CSpecifier cspecifier_set[];
extern const COperand cregister_set[];
extern const COperand csyntax_set[];
extern const COperand CINIT;
extern const COperand CNOT_FOUND;

enum CSpecifierType cspecifier_scan(const char *string);

int coperand_is_subtype_of(const COperand *self, enum COperandType baseType);

int coperand_type_is_void(enum COperandType type);
int coperand_type_is_imm(enum COperandType type);
int coperand_type_is_reg(enum COperandType type);
int coperand_type_is_mem(enum COperandType type);
int coperand_type_is_R_M(enum COperandType type);

int coperand_is_void(const COperand *operand);
int coperand_is_imm(const COperand *operand);
int coperand_is_reg(const COperand *operand);
int coperand_is_mem(const COperand *operand);
int coperand_is_R_M(const COperand *operand);

COperand coperand_scan_reg(const char *string);
enum COperandType coperand_scan_syntax(const char *string);


char *cstring_strip(char *str);
int cstring_strcmp(const char *s1, const char *s2, int caseoff);
char *cstring_strsep(char **stringp, const char *delim);



//---------------------------------------------------------------------
// specifier set
//---------------------------------------------------------------------
const CSpecifier cspecifier_set[] =
{
	{CS_UNKNOWN,	""},
	{CS_NEAR,		"NEAR"},
	{CS_SHORT,		"SHORT"},
//	{FAR,			"FAR"},
	{CS_BYTE,		"BYTE"},
	{CS_WORD,		"WORD"},
	{CS_DWORD,		"DWORD"},
	{CS_QWORD,		"QWORD"},
	{CS_MMWORD,		"MMWORD"},
	{CS_XMMWORD,	"XMMWORD"},
};

enum CSpecifierType cspecifier_scan(const char *string)
{
	if (string) {
		size_t i;
		for(i = 0; i < sizeof(cspecifier_set) / sizeof(CSpecifier); i++) {
			if(cstring_strcmp(string, cspecifier_set[i].notation, 1) == 0) {
				return cspecifier_set[i].type;
			}		
		}
	}
	return CS_UNKNOWN;
}


int coperand_is_subtype_of(const COperand *self, enum COperandType baseType)
{
	return (self->type & baseType) == self->type;
}

int coperand_type_is_void(enum COperandType type)
{
	return type == O_VOID;
}

int coperand_type_is_imm(enum COperandType type)
{
	return (type & O_IMM) == type;
}

int coperand_type_is_reg(enum COperandType type)
{
	return (type & O_REG) == type;
}

int coperand_type_is_mem(enum COperandType type)
{
	return (type & O_MEM) == type;
}

int coperand_type_is_R_M(enum COperandType type)
{
	return (type & O_R_M) == type;
}

int coperand_is_void(const COperand *operand)
{
	return coperand_type_is_void(operand->type);
}

int coperand_is_imm(const COperand *operand)
{
	return coperand_type_is_imm(operand->type);
}

int coperand_is_reg(const COperand *operand)
{
	return coperand_type_is_reg(operand->type);
}

int coperand_is_mem(const COperand *operand)
{
	return coperand_type_is_mem(operand->type);
}

int coperand_is_R_M(const COperand *operand)
{
	return coperand_type_is_R_M(operand->type);
}

const COperand cregister_set[] = 
{
	{O_VOID,		""},

	{O_AL,		"AL", { 0 } },
	{O_CL,		"CL", { 1 } },
	{O_REG8,	"DL", { 2 } },
	{O_REG8,	"BL", { 3 } },
	{O_REG8,	"AH", { 4 } },
	{O_REG8,	"CH", { 5 } },
	{O_REG8,	"DH", { 6 } },
	{O_REG8,	"BH", { 7 } },

	{O_AX,		"AX", { 0 } },
	{O_CX,		"CX", { 1 } },
	{O_DX,		"DX", { 2 } },
	{O_REG16,	"BX", { 3 } },
	{O_REG16,	"SP", { 4 } },
	{O_REG16,	"BP", { 5 } },
	{O_REG16,	"SI", { 6 } },
	{O_REG16,	"DI", { 7 } },

	{O_EAX,		"EAX", { 0 } },
	{O_ECX,		"ECX", { 1 } },
	{O_REG32,	"EDX", { 2 } },
	{O_REG32,	"EBX", { 3 } },
	{O_REG32,	"ESP", { 4 } },
	{O_REG32,	"EBP", { 5 } },
	{O_REG32,	"ESI", { 6 } },
	{O_REG32,	"EDI", { 7 } },

	{O_ES,		"ES", { 0 } },
	{O_CS,		"CS", { 1 } },
	{O_SS,		"SS", { 2 } },
	{O_DS,		"DS", { 3 } },
	{O_FS,		"FS", { 4 } },
	{O_GS,		"GS", { 5 } },

	{O_ST0,		"ST0", { 0 } },
	{O_FPUREG,	"ST1", { 1 } },
	{O_FPUREG,	"ST2", { 2 } },
	{O_FPUREG,	"ST3", { 3 } },
	{O_FPUREG,	"ST4", { 4 } },
	{O_FPUREG,	"ST5", { 5 } },
	{O_FPUREG,	"ST6", { 6 } },
	{O_FPUREG,	"ST7", { 7 } },

	{O_MMREG,	"MM0", { 0 } },
	{O_MMREG,	"MM1", { 1 } },
	{O_MMREG,	"MM2", { 2 } },
	{O_MMREG,	"MM3", { 3 } },
	{O_MMREG,	"MM4", { 4 } },
	{O_MMREG,	"MM5", { 5 } },
	{O_MMREG,	"MM6", { 6 } },
	{O_MMREG,	"MM7", { 7 } },

	{O_XMMREG,	"XMM0", { 0 } },
	{O_XMMREG,	"XMM1", { 1 } },
	{O_XMMREG,	"XMM2", { 2 } },
	{O_XMMREG,	"XMM3", { 3 } },
	{O_XMMREG,	"XMM4", { 4 } },
	{O_XMMREG,	"XMM5", { 5 } },
	{O_XMMREG,	"XMM6", { 6 } },
	{O_XMMREG,	"XMM7", { 7 } }
};

const COperand csyntax_set[] = 
{
	{O_VOID,	""},

	{O_ONE,		"1"},
	{O_IMM,		"imm"},
	{O_IMM8,	"imm8"},
	{O_IMM16,	"imm16"},
	{O_IMM32,	"imm32"},

	{O_AL,		"AL"},
	{O_AX,		"AX"},
	{O_EAX,		"EAX"},
	{O_DX,		"DX"},
	{O_CL,		"CL"},
	{O_CX,		"CX"},
	{O_ECX,		"ECX"},
	{O_CS,		"CS"},
	{O_DS,		"DS"},
	{O_ES,		"ES"},
	{O_SS,		"SS"},
	{O_FS,		"FS"},
	{O_GS,		"GS"},
	{O_ST0,		"ST0"},

	{O_REG8,	"reg8"},
	{O_REG16,	"reg16"},
	{O_REG32,	"reg32"},
	{O_SEGREG,	"segreg"},
	{O_FPUREG,	"fpureg"},
	{O_CR,		"CR0/2/3/4"},
	{O_DR,		"DR0/1/2/3/6/7"},
	{O_TR,		"TR3/4/5/6/7"},
	{O_MMREG,	"mmreg"},
	{O_XMMREG,	"xmmreg"},

	{O_MEM,		"mem"},
	{O_MEM8,	"mem8"},
	{O_MEM16,	"mem16"},
	{O_MEM32,	"mem32"},
	{O_MEM64,	"mem64"},
	{O_MEM80,	"mem80"},
	{O_MEM128,	"mem128"},

	{O_R_M8,	"r/m8"},
	{O_R_M16,	"r/m16"},
	{O_R_M32,	"r/m32"},
	{O_R_M64,	"r/m64"},
	{O_R_M128,	"r/m128"},

	{O_XMM32,	"xmmreg/mem32"},
	{O_XMM32,	"xmmreg/mem64"},
	{O_M512B,	"m512byte"},
	{O_MOFF8,	"memoffs8"},
	{O_MOFF16,	"memoffs16"},
	{O_MOFF32,	"memoffs32"}
};

const COperand CINIT = { O_VOID };
const COperand CNOT_FOUND = { O_UNKNOWN };

COperand coperand_scan_reg(const char *string)
{
	if (string) {
		size_t i;
		for (i = 0; i < sizeof(cregister_set) / sizeof(COperand); i++) {
			if (cstring_strcmp(string, cregister_set[i].notation, 1) == 0) {
				return cregister_set[i];
			}
		}
	}
	return CNOT_FOUND;
}

enum COperandType coperand_scan_syntax(const char *string)
{
	if (string) {
		size_t i;
		for (i = 0; i < sizeof(csyntax_set) / sizeof(COperand); i++) {
			if (cstring_strcmp(string, csyntax_set[i].notation, 1) == 0) {
				return csyntax_set[i].type;
			}
		}
	}
	return O_UNKNOWN;
}


//---------------------------------------------------------------------
// string operation
//---------------------------------------------------------------------
char *cstring_strip(char *str)
{
	int size = (int)strlen(str);
	char *p = str;
	int i;
	while (size > 0) {
		if (!isspace(str[size - 1])) break;
		size--;
	}
	str[size] = '\0';
	while (p[0]) {
		if (!isspace(p[0])) break;
		p++;
	}
	if (p == str) return str;
	for (i = 0; p[i]; i++) str[i] = p[i];
	str[i] = '\0';
	return str;
}

int cstring_strcmp(const char *s1, const char *s2, int caseoff)
{
	const char *p1, *p2, *p3, *p4;
	int k1, k2, i;
	for (p1 = s1; isspace(*p1); p1++);
	for (p2 = s2; isspace(*p2); p2++);
	for (k1 = (int)strlen(p1); k1 > 0; k1--) if (!isspace(p1[k1 - 1])) break;
	for (k2 = (int)strlen(p2); k2 > 0; k2--) if (!isspace(p2[k2 - 1])) break;
	p3 = p1 + k1;
	p4 = p2 + k2;
	for (i = 0; i < k1 && i < k2; i++) {
		char c1 = p1[i];
		char c2 = p2[i];
		if (caseoff && c1 >= 'a' && c1 <= 'z') c1 -= 'a' - 'A';
		if (caseoff && c2 >= 'a' && c2 <= 'z') c2 -= 'a' - 'A';
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
	}
	if (k1 < k2) return -1;
	if (k1 > k2) return 1;
	return 0;
}

char *cstring_strsep(char **stringp, const char *delim)
{
	register char *s;
	register const char *spanp;
	register int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0) s = NULL;
				else s[-1] = 0;
				*stringp = s;
				return tok;
			}
		}	while (sc != 0);
	}
}




//---------------------------------------------------------------------
// CInstructionType
//---------------------------------------------------------------------
enum CInstructionType
{
	CT_CPU_UNKNOWN	= 0x00000000,

	CT_CPU_8086		= 0x00000001,
	CT_CPU_186		= 0x00000002,
	CT_CPU_286		= 0x00000004,
	CT_CPU_386		= 0x00000008,
	CT_CPU_486		= 0x00000010,
	CT_CPU_PENT		= 0x00000020,   // Pentium
	CT_CPU_P6		= 0x00000040,   // Pentium Pro

	CT_CPU_FPU		= 0x00000080,
	CT_CPU_MMX		= 0x00000100,
	CT_CPU_KATMAI	= 0x00000200,
	CT_CPU_SSE		= 0x00000400,

//	CT_CPU_AMD		= 0x00000800,   // AMD specific system calls
	CT_CPU_CYRIX	= 0x00001000,
	CT_CPU_3DNOW	= 0x00002000,
	CT_CPU_ATHLON	= 0x00004000,
//	CT_CPU_SMM		= 0x00008000,   // System Management Mode, standby mode

	CT_CPU_P7		= 0x00010000 | CT_CPU_SSE,
	CT_CPU_WILLAMETTE = CT_CPU_P7,
	CT_CPU_SSE2		= 0x00020000 | CT_CPU_WILLAMETTE,
	CT_CPU_PNI		= 0x00040000,
	CT_CPU_SSE3		= 0x00080000,

// Undocumented, also not supported by inline assembler
//	CT_CPU_UNDOC	= 0x00010000,  
// Priviledged, run-time compiled OS kernel anyone?
//	CT_CPU_PRIV	= 0x00020000    
};


//---------------------------------------------------------------------
// CInstructionSyntax
//---------------------------------------------------------------------
struct CInstSyntax
{
	const char *mnemonic;
	const char *operands;
	const char *encoding;
	int flags;
};

typedef struct CInstSyntax CInstSyntax;


//---------------------------------------------------------------------
// CInstruction
//---------------------------------------------------------------------
struct CInstruction
{
	int syntaxMnemonic : 1;
	int syntaxSpecifier : 1;
	int syntaxFirstOperand : 1;
	int syntaxSecondOperand : 1;
	int syntaxThirdOperand : 1;

	const struct CInstSyntax *syntax;
	enum CSpecifierType specifier;
	enum COperandType firstOperand;
	enum COperandType secondOperand;
	enum COperandType thirdOperand;
	int flags;

	struct CInstruction *next;
};

typedef struct CInstruction CInstruction;


CInstruction *cinst_create(const CInstSyntax *syntax);
void cinst_release(CInstruction *self);

CInstruction *cinst_get_next(CInstruction *self);

void cinst_attach_new(CInstruction *self, const CInstSyntax *instruction);
		
void cinst_reset_match(CInstruction *self);
int cinst_match_syntax(CInstruction *self);
void cinst_match_mnemonic(CInstruction *self, const char *mnemonic);
void cinst_match_specifier(CInstruction *self, enum CSpecifierType sizeSpec);
void cinst_match_first_operand(CInstruction *self, const COperand *operand);
void cinst_match_second_operand(CInstruction *self, const COperand *operand);
void cinst_match_third_operand(CInstruction *self, const COperand *operand);

enum COperandType cinst_getFirstOperand(CInstruction *self);
enum COperandType cinst_getSecondOperand(CInstruction *self);
enum COperandType cinst_getThirdOperand(CInstruction *self);

const char *cinst_getMnemonic(CInstruction *self);
const char *cinst_getOperandSyntax(CInstruction *self);
const char *cinst_getEncoding(CInstruction *self);
		
int cinst_is_32bit(CInstruction *self);



#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

static void cinst_extract_operands(CInstruction *self, const char *syntax);

CInstruction *cinst_create(const CInstSyntax *syntax)
{
	CInstruction *self;
	self = (CInstruction*)malloc(sizeof(CInstruction));
	assert(self);
	self->syntax = syntax;
	cinst_extract_operands(self, syntax->operands);
	self->syntaxMnemonic = 0;
	self->syntaxSpecifier = 0;
	self->syntaxFirstOperand = 0;
	self->syntaxSecondOperand = 0;
	self->syntaxThirdOperand = 0;
	self->flags = syntax->flags;
	self->next = NULL;
	return self;
}

void cinst_release(CInstruction *self)
{
	if (self->next) cinst_release(self->next);
	self->next = NULL;
	memset(self, 0, sizeof(CInstruction));
	free(self);
}

static void cinst_extract_operands(CInstruction *self, const char *syntax)
{
	char *token;
	char *string;
	char *sep;

	assert(syntax && self);

	self->specifier = CS_UNKNOWN;
	self->firstOperand = O_VOID;
	self->secondOperand = O_VOID;
	self->thirdOperand = O_VOID;

	string = strdup(syntax);
	cstring_strip(string);

	sep = string;
	token = cstring_strsep(&sep, " ,");

	if (token == NULL) return;

	cstring_strip(token);
	self->specifier = cspecifier_scan(token);

	if (self->specifier != CS_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->firstOperand = coperand_scan_syntax(token);

	if (self->firstOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->secondOperand = coperand_scan_syntax(token);

	if (self->secondOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->thirdOperand = coperand_scan_syntax(token);

	if (self->thirdOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	if (token != 0) {
		fprintf(stderr, "casm: Invalid operand encoding '%s'\n", syntax);
		fflush(stderr);
		assert(0);
		return;
	}

	free(string);
}

CInstruction *cinst_get_next(CInstruction *self)
{
	assert(self);
	return self->next;
}

void cinst_attach_new(CInstruction *self, const CInstSyntax *instruction)
{
	if (!self->next) {
		self->next = cinst_create(instruction);
	}	else {
		cinst_attach_new(self->next, instruction);
	}
}
		
void cinst_reset_match(CInstruction *self)
{
	self->syntaxMnemonic = 0;
	self->syntaxSpecifier = 0;
	self->syntaxFirstOperand = 0;
	self->syntaxSecondOperand = 0;
	self->syntaxThirdOperand = 0;

	if (self->next) {
		cinst_reset_match(self->next);
	}
}

int cinst_match_syntax(CInstruction *self)
{
	return  self->syntaxMnemonic != 0 &&
			self->syntaxSpecifier != 0 &&
			self->syntaxFirstOperand != 0 &&
			self->syntaxSecondOperand != 0 &&
			self->syntaxThirdOperand != 0;	
}

void cinst_match_mnemonic(CInstruction *self, const char *mnemonic)
{
	if (stricmp(self->syntax->mnemonic, mnemonic) == 0) {
		self->syntaxMnemonic = 1;
	}
	if (self->next) {
		cinst_match_mnemonic(self->next, mnemonic);
	}
}

void cinst_match_specifier(CInstruction *self, enum CSpecifierType specifier)
{
	if (self->specifier == CS_UNKNOWN) 
	{
		if (self->specifier != CS_UNKNOWN) {
			if (self->firstOperand == O_R_M8 || 
				self->secondOperand == O_R_M8) {
				self->syntaxSpecifier = self->specifier == CS_BYTE;
			}
			else if (self->firstOperand == O_R_M16 || 
					self->secondOperand == O_R_M16) {
				self->syntaxSpecifier = self->specifier == CS_WORD;
			}
			else if (self->firstOperand == O_R_M32 || 
					self->secondOperand == O_R_M32) {
				self->syntaxSpecifier = self->specifier == CS_DWORD;
			}
			else if (self->firstOperand == O_R_M64 || 
					self->secondOperand == O_R_M64) {
				self->syntaxSpecifier = 
					(self->specifier == CS_QWORD || 
					self->specifier == CS_MMWORD);
			}
			else if (self->firstOperand == O_R_M128 || 
					self->secondOperand == O_R_M128) {
				self->syntaxSpecifier = self->specifier == CS_XMMWORD;
			}	
			else {
				self->syntaxSpecifier = 1;
			}
		}	else {
			self->syntaxSpecifier = 1;
		}
	}
	else if (self->specifier != CS_UNKNOWN)   // Explicit specifier
	{
		if (self->specifier == specifier) {
			self->syntaxSpecifier = 1;
		}
		else if (specifier == CS_UNKNOWN) {  
			self->syntaxSpecifier = 1;	// Specifiers are optional
		}
		else {
			self->syntaxSpecifier = 0;
		}
	}

	if (self->next) {
		cinst_match_specifier(self->next, specifier);
	}
}

void cinst_match_first_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->firstOperand)) {
		self->syntaxFirstOperand = 1;
	}
	else if (operand->type == O_MEM && self->firstOperand & O_MEM) {
		if(self->syntaxSpecifier) {  // Explicit size specfier
			self->syntaxFirstOperand = 1;
		}
		else if(self->secondOperand != O_UNKNOWN) { //Implicit size specifier
			self->syntaxFirstOperand = 1;
		}
	}

	if (self->next) {
		cinst_match_first_operand(self->next, operand);
	}
}

void cinst_match_second_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->secondOperand)) {
		self->syntaxSecondOperand = 1;
	}
	else if (operand->type == O_MEM && self->secondOperand & O_MEM) {
		if (self->syntaxSpecifier) {  // Explicit size specfier
			self->syntaxSecondOperand = 1;
		}
		else if (self->firstOperand != O_UNKNOWN) {
			self->syntaxSecondOperand = 1;
		}
	}
	if (self->next) {
		cinst_match_second_operand(self->next, operand);
	}
}

void cinst_match_third_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->thirdOperand)) {
		self->syntaxThirdOperand = 1;
	}
	if (self->next) {
		cinst_match_third_operand(self->next, operand);
	}
}

enum COperandType cinst_getFirstOperand(CInstruction *self)
{
	return self->firstOperand;
}

enum COperandType cinst_getSecondOperand(CInstruction *self)
{
	return self->secondOperand;
}

enum COperandType cinst_getThirdOperand(CInstruction *self)
{
	return self->thirdOperand;
}

const char *cinst_getMnemonic(CInstruction *self)
{
	return self->syntax->mnemonic;
}

const char *cinst_getOperandSyntax(CInstruction *self)
{
	return self->syntax->operands;
}

const char *cinst_getEncoding(CInstruction *self)
{
	return self->syntax->encoding;
}
		
int cinst_is_32bit(CInstruction *self)
{
	return (self->flags & CT_CPU_386) == CT_CPU_386;
}



struct CInstructionEntry
{
	const char *mnemonic;
	CInstruction *instruction;
};

typedef struct CInstructionEntry CInstructionEntry;

struct CInstructionSet
{
	CInstructionEntry *instructionMap;
};

typedef struct CInstructionSet CInstructionSet;


extern CInstSyntax cinstruction_set[];

int cinstset_num_instructions(void);
int cinstset_num_mnemonics(void);

CInstructionSet *cinstset_create(void);
void cinstset_release(CInstructionSet *self);

CInstruction *cinstset_query(const CInstructionSet *self, const char *name);


#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

CInstSyntax cinstruction_set[] = 
{
	/*
		Encoding syntax:
		----------------
		+r Add register value to opcode
		/# Value for Mod R/M register field encoding
		/r Effective address encoding
		ib Byte immediate
		iw Word immediate
		id Dword immediate
		-b Byte relative address
		-i Word or dword relative address
		p0 LOCK instruction prefix (F0h)
		p2 REPNE/REPNZ instruction prefix (F2h)
		p3 REP/REPE/REPZ instruction prefix (F3h) (also SSE prefix)
		po Offset override prefix (66h)
		pa Address override prefix (67h)
	*/

	{"AAA",			"",							"37",				CT_CPU_8086},
	{"AAS",			"",							"3F",				CT_CPU_8086},
	{"AAD",			"",							"D5 0A",			CT_CPU_8086},
	{"AAD",			"imm",						"D5 ib",			CT_CPU_8086},
	{"AAM",			"",							"D4 0A",			CT_CPU_8086},
	{"AAM",			"imm",						"D4 ib",			CT_CPU_8086},
	{"ADC",			"r/m8,reg8",				"10 /r",			CT_CPU_8086},
	{"ADC",			"r/m16,reg16",				"po 11 /r",			CT_CPU_8086},
	{"ADC",			"r/m32,reg32",				"po 11 /r",			CT_CPU_386},
	{"ADC",			"reg8,r/m8",				"12 /r",			CT_CPU_8086},
	{"ADC",			"reg16,r/m16",				"po 13 /r",			CT_CPU_8086},
	{"ADC",			"reg32,r/m32",				"po 13 /r",			CT_CPU_386},
	{"ADC",			"r/m8,imm8",				"80 /2 ib",			CT_CPU_8086},
	{"ADC",			"r/m16,imm16",				"po 81 /2 iw",		CT_CPU_8086},
	{"ADC",			"r/m32,imm32",				"po 81 /2 id",		CT_CPU_386},
	{"ADC",			"r/m16,imm8",				"po 83 /2 ib",		CT_CPU_8086},
	{"ADC",			"r/m32,imm8",				"po 83 /2 ib",		CT_CPU_386},
	{"ADC",			"AL,imm8",					"14 ib",			CT_CPU_8086},
	{"ADC",			"AX,imm16",					"po 15 iw",			CT_CPU_8086},
	{"ADC",			"EAX,imm32",				"po 15 id",			CT_CPU_386},
	{"ADD",			"r/m8,reg8",				"00 /r",			CT_CPU_8086},
	{"ADD",			"r/m16,reg16",				"po 01 /r",			CT_CPU_8086},
	{"ADD",			"r/m32,reg32",				"po 01 /r",			CT_CPU_386},
	{"ADD",			"reg8,r/m8",				"02 /r",			CT_CPU_8086},
	{"ADD",			"reg16,r/m16",				"po 03 /r",			CT_CPU_8086},
	{"ADD",			"reg32,r/m32",				"po 03 /r",			CT_CPU_386},
	{"ADD",			"r/m8,imm8",				"80 /0 ib",			CT_CPU_8086},
	{"ADD",			"r/m16,imm16",				"po 81 /0 iw",		CT_CPU_8086},
	{"ADD",			"r/m32,imm32",				"po 81 /0 id",		CT_CPU_386},
	{"ADD",			"r/m16,imm8",				"po 83 /0 ib",		CT_CPU_8086},
	{"ADD",			"r/m32,imm8",				"po 83 /0 ib",		CT_CPU_386},
	{"ADD",			"AL,imm8",					"04 ib",			CT_CPU_8086},
	{"ADD",			"AX,imm16",					"po 05 iw",			CT_CPU_8086},
	{"ADD",			"EAX,imm32",				"po 05 id",			CT_CPU_386},
	{"ADDPS",		"xmmreg,r/m128",			"0F 58 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"ADDSS",		"xmmreg,xmmreg/mem32",		"p3 0F 58 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"AND",			"r/m8,reg8",				"20 /r",			CT_CPU_8086},
	{"AND",			"r/m16,reg16",				"po 21 /r",			CT_CPU_8086},
	{"AND",			"r/m32,reg32",				"po 21 /r",			CT_CPU_386},
	{"AND",			"reg8,r/m8",				"22 /r",			CT_CPU_8086},
	{"AND",			"reg16,r/m16",				"po 23 /r",			CT_CPU_8086},
	{"AND",			"reg32,r/m32",				"po 23 /r",			CT_CPU_386},
	{"AND",			"r/m8,imm8",				"80 /4 ib",			CT_CPU_8086},
	{"AND",			"r/m16,imm16",				"po 81 /4 iw",		CT_CPU_8086},
	{"AND",			"r/m32,imm32",				"po 81 /4 id",		CT_CPU_386},
	{"AND",			"r/m16,imm8",				"po 83 /4 ib",		CT_CPU_8086},
	{"AND",			"r/m32,imm8",				"po 83 /4 ib",		CT_CPU_386},
	{"AND",			"AL,imm8",					"24 ib",			CT_CPU_8086},
	{"AND",			"AX,imm16",					"po 25 iw",			CT_CPU_8086},
	{"AND",			"EAX,imm32",				"po 25 id",			CT_CPU_386},
	{"ANDNPS",		"xmmreg,r/m128",			"0F 55 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"ANDPS",		"xmmreg,r/m128",			"0F 54 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
//	{"ARPL",		"r/m16,reg16",				"63 /r",			CT_CPU_286 | CT_CPU_PRIV},
	{"BOUND",		"reg16,mem",				"po 62 /r",			CT_CPU_186},
	{"BOUND",		"reg32,mem",				"po 62 /r",			CT_CPU_386},
	{"BSF",			"reg16,r/m16",				"po 0F BC /r",		CT_CPU_386},
	{"BSF",			"reg32,r/m32",				"po 0F BC /r",		CT_CPU_386},
	{"BSR",			"reg16,r/m16",				"po 0F BD /r",		CT_CPU_386},
	{"BSR",			"reg32,r/m32",				"po 0F BD /r",		CT_CPU_386},
	{"BSWAP",		"reg32",					"po 0F C8 +r",		CT_CPU_486},
	{"BT",			"r/m16,reg16",				"po 0F A3 /r",		CT_CPU_386},
	{"BT",			"r/m32,reg32",				"po 0F A3 /r",		CT_CPU_386},
	{"BT",			"r/m16,imm8",				"po 0F BA /4 ib",	CT_CPU_386},
	{"BT",			"r/m32,imm8",				"po 0F BA /4 ib",	CT_CPU_386},
	{"BTC",			"r/m16,reg16",				"po 0F BB /r",		CT_CPU_386},
	{"BTC",			"r/m32,reg32",				"po 0F BB /r",		CT_CPU_386},
	{"BTC",			"r/m16,imm8",				"po 0F BA /7 ib",	CT_CPU_386},
	{"BTC",			"r/m32,imm8",				"po 0F BA /7 ib",	CT_CPU_386},
	{"BTR",			"r/m16,reg16",				"po 0F B3 /r",		CT_CPU_386},
	{"BTR",			"r/m32,reg32",				"po 0F B3 /r",		CT_CPU_386},
	{"BTR",			"r/m16,imm8",				"po 0F BA /6 ib",	CT_CPU_386},
	{"BTR",			"r/m32,imm8",				"po 0F BA /6 ib",	CT_CPU_386},
	{"BTS",			"r/m16,reg16",				"po 0F AB /r",		CT_CPU_386},
	{"BTS",			"r/m32,reg32",				"po 0F AB /r",		CT_CPU_386},
	{"BTS",			"r/m16,imm",				"po 0F BA /5 ib",	CT_CPU_386},
	{"BTS",			"r/m32,imm",				"po 0F BA /5 ib",	CT_CPU_386},
	{"CALL",		"imm",						"E8 -i",			CT_CPU_8086},
//	{"CALL",		"imm:imm16",				"po 9A iw iw",		CT_CPU_8086},
//	{"CALL",		"imm:imm32",				"po 9A id iw",		CT_CPU_386},
//	{"CALL",		"FAR mem16",				"po FF /3",			CT_CPU_8086},
//	{"CALL",		"FAR mem32",				"po FF /3",			CT_CPU_386},
	{"CALL",		"WORD r/m16",				"po FF /2",			CT_CPU_8086},
	{"CALL",		"DWORD r/m32",				"po FF /2",			CT_CPU_386},
	{"CBW",			"",							"po 98",			CT_CPU_8086},
	{"CWD",			"",							"po 99",			CT_CPU_8086},
	{"CDQ",			"",							"po 99",			CT_CPU_386},
	{"CWDE",		"",							"po 98",			CT_CPU_386},
	{"CLC",			"",							"F8",				CT_CPU_8086},
	{"CLD",			"",							"FC",				CT_CPU_8086},
	{"CLI",			"",							"FA",				CT_CPU_8086},
//	{"CLTS",		"",							"0F 06",			CT_CPU_286 | CT_CPU_PRIV},
	{"CMC",			"",							"F5",				CT_CPU_8086},
	{"CMOVO",		"reg16,r/m16",				"po 0F 40 /r",		CT_CPU_P6},
	{"CMOVNO",		"reg16,r/m16",				"po 0F 41 /r",		CT_CPU_P6},
	{"CMOVB",		"reg16,r/m16",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVC",		"reg16,r/m16",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVNEA",		"reg16,r/m16",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVAE",		"reg16,r/m16",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVNB",		"reg16,r/m16",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVNC",		"reg16,r/m16",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVE",		"reg16,r/m16",				"po 0F 44 /r",		CT_CPU_P6},
	{"CMOVZ",		"reg16,r/m16",				"po 0F 44 /r",		CT_CPU_P6},
	{"CMOVNE",		"reg16,r/m16",				"po 0F 45 /r",		CT_CPU_P6},
	{"CMOVNZ",		"reg16,r/m16",				"po 0F 45 /r",		CT_CPU_P6},
	{"CMOVBE",		"reg16,r/m16",				"po 0F 46 /r",		CT_CPU_P6},
	{"CMOVNA",		"reg16,r/m16",				"po 0F 46 /r",		CT_CPU_P6},
	{"CMOVA",		"reg16,r/m16",				"po 0F 47 /r",		CT_CPU_P6},
	{"CMOVNBE",		"reg16,r/m16",				"po 0F 47 /r",		CT_CPU_P6},
	{"CMOVS",		"reg16,r/m16",				"po 0F 48 /r",		CT_CPU_P6},
	{"CMOVNS",		"reg16,r/m16",				"po 0F 49 /r",		CT_CPU_P6},
	{"CMOVP",		"reg16,r/m16",				"po 0F 4A /r",		CT_CPU_P6},
	{"CMOVPE",		"reg16,r/m16",				"po 0F 4A /r",		CT_CPU_P6},
	{"CMOVNP",		"reg16,r/m16",				"po 0F 4B /r",		CT_CPU_P6},
	{"CMOVPO",		"reg16,r/m16",				"po 0F 4B /r",		CT_CPU_P6},
	{"CMOVL",		"reg16,r/m16",				"po 0F 4C /r",		CT_CPU_P6},
	{"CMOVNGE",		"reg16,r/m16",				"po 0F 4C /r",		CT_CPU_P6},
	{"CMOVGE",		"reg16,r/m16",				"po 0F 4D /r",		CT_CPU_P6},
	{"CMOVNL",		"reg16,r/m16",				"po 0F 4D /r",		CT_CPU_P6},
	{"CMOVLE",		"reg16,r/m16",				"po 0F 4E /r",		CT_CPU_P6},
	{"CMOVNG",		"reg16,r/m16",				"po 0F 4E /r",		CT_CPU_P6},
	{"CMOVG",		"reg16,r/m16",				"po 0F 4F /r",		CT_CPU_P6},
	{"CMOVNLE",		"reg16,r/m16",				"po 0F 4F /r",		CT_CPU_P6},
	{"CMOVO",		"reg32,r/m32",				"po 0F 40 /r",		CT_CPU_P6},
	{"CMOVNO",		"reg32,r/m32",				"po 0F 41 /r",		CT_CPU_P6},
	{"CMOVB",		"reg32,r/m32",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVC",		"reg32,r/m32",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVNEA",		"reg32,r/m32",				"po 0F 42 /r",		CT_CPU_P6},
	{"CMOVAE",		"reg32,r/m32",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVNB",		"reg32,r/m32",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVNC",		"reg32,r/m32",				"po 0F 43 /r",		CT_CPU_P6},
	{"CMOVE",		"reg32,r/m32",				"po 0F 44 /r",		CT_CPU_P6},
	{"CMOVZ",		"reg32,r/m32",				"po 0F 44 /r",		CT_CPU_P6},
	{"CMOVNE",		"reg32,r/m32",				"po 0F 45 /r",		CT_CPU_P6},
	{"CMOVNZ",		"reg32,r/m32",				"po 0F 45 /r",		CT_CPU_P6},
	{"CMOVBE",		"reg32,r/m32",				"po 0F 46 /r",		CT_CPU_P6},
	{"CMOVNA",		"reg32,r/m32",				"po 0F 46 /r",		CT_CPU_P6},
	{"CMOVA",		"reg32,r/m32",				"po 0F 47 /r",		CT_CPU_P6},
	{"CMOVNBE",		"reg32,r/m32",				"po 0F 47 /r",		CT_CPU_P6},
	{"CMOVS",		"reg32,r/m32",				"po 0F 48 /r",		CT_CPU_P6},
	{"CMOVNS",		"reg32,r/m32",				"po 0F 49 /r",		CT_CPU_P6},
	{"CMOVP",		"reg32,r/m32",				"po 0F 4A /r",		CT_CPU_P6},
	{"CMOVPE",		"reg32,r/m32",				"po 0F 4A /r",		CT_CPU_P6},
	{"CMOVNP",		"reg32,r/m32",				"po 0F 4B /r",		CT_CPU_P6},
	{"CMOVPO",		"reg32,r/m32",				"po 0F 4B /r",		CT_CPU_P6},
	{"CMOVL",		"reg32,r/m32",				"po 0F 4C /r",		CT_CPU_P6},
	{"CMOVNGE",		"reg32,r/m32",				"po 0F 4C /r",		CT_CPU_P6},
	{"CMOVGE",		"reg32,r/m32",				"po 0F 4D /r",		CT_CPU_P6},
	{"CMOVNL",		"reg32,r/m32",				"po 0F 4D /r",		CT_CPU_P6},
	{"CMOVLE",		"reg32,r/m32",				"po 0F 4E /r",		CT_CPU_P6},
	{"CMOVNG",		"reg32,r/m32",				"po 0F 4E /r",		CT_CPU_P6},
	{"CMOVG",		"reg32,r/m32",				"po 0F 4F /r",		CT_CPU_P6},
	{"CMOVNLE",		"reg32,r/m32",				"po 0F 4F /r",		CT_CPU_P6},
	{"CMP",			"r/m8,reg8",				"38 /r",			CT_CPU_8086},
	{"CMP",			"r/m16,reg16",				"po 39 /r",			CT_CPU_8086},
	{"CMP",			"r/m32,reg32",				"po 39 /r",			CT_CPU_386},
	{"CMP",			"reg8,r/m8",				"3A /r",			CT_CPU_8086},
	{"CMP",			"reg16,r/m16",				"po 3B /r",			CT_CPU_8086},
	{"CMP",			"reg32,r/m32",				"po 3B /r",			CT_CPU_386},
	{"CMP",			"r/m8,imm8",				"80 /7 ib",			CT_CPU_8086},
	{"CMP",			"r/m16,imm16",				"po 81 /7 iw",		CT_CPU_8086},
	{"CMP",			"r/m32,imm32",				"po 81 /7 id",		CT_CPU_386},
	{"CMP",			"r/m16,imm8",				"po 83 /7 ib",		CT_CPU_8086},
	{"CMP",			"r/m32,imm8",				"po 83 /7 ib",		CT_CPU_386},
	{"CMP",			"AL,imm8",					"3C ib",			CT_CPU_8086},
	{"CMP",			"AX,imm16",					"po 3D iw",			CT_CPU_8086},
	{"CMP",			"EAX,imm32",				"po 3D id",			CT_CPU_386},
	{"CMPPS",		"xmmreg,r/m128,imm8",		"0F C2 /r ib",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPEQPS",		"xmmreg,r/m128",			"0F C2 /r 00",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPLEPS",		"xmmreg,r/m128",			"0F C2 /r 02",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPLTPS",		"xmmreg,r/m128",			"0F C2 /r 01",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNEQPS",	"xmmreg,r/m128",			"0F C2 /r 04",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNLEPS",	"xmmreg,r/m128",			"0F C2 /r 06",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNLTPS",	"xmmreg,r/m128",			"0F C2 /r 05",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPORDPS",	"xmmreg,r/m128",			"0F C2 /r 07",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPUNORDPS",	"xmmreg,r/m128",			"0F C2 /r 03",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPSB",		"",							"A6",				CT_CPU_8086},
	{"CMPSW",		"",							"po A7",			CT_CPU_8086},
	{"CMPSD",		"",							"po A7",			CT_CPU_386},
	{"CMPSS",		"xmmreg,xmmreg/mem32,imm8",	"p3 0F C2 /r ib",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPEQSS",		"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 00",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPLESS",		"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 02",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPLTSS",		"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 01",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNEQSS",	"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 04",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNLESS",	"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 06",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPNLTSS",	"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 05",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPORDSS",	"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 07",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPUNORDSS",	"xmmreg,xmmreg/mem32",		"p3 0F C2 /r 03",	CT_CPU_KATMAI | CT_CPU_SSE},
	{"CMPXCHG",		"r/m8,reg8",				"0F B0 /r",			CT_CPU_PENT},
	{"CMPXCHG",		"r/m16,reg16",				"po 0F B1 /r",		CT_CPU_PENT},
	{"CMPXCHG",		"r/m32,reg32",				"po 0F B1 /r",		CT_CPU_PENT},
//	{"CMPXCHG486",	"r/m8,reg8",				"0F A6 /r",			CT_CPU_486 | CT_CPU_UNDOC},
//	{"CMPXCHG486",	"r/m16,reg16",				"po 0F A7 /r",		CT_CPU_486 | CT_CPU_UNDOC},
//	{"CMPXCHG486",	"r/m32,reg32",				"po 0F A7 /r",		CT_CPU_486 | CT_CPU_UNDOC},
	{"CMPXCHG8B",	"mem",						"0F C7 /1",			CT_CPU_PENT},
	{"COMISS",		"xmmreg,xmmreg/mem32",		"0F 2F /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"CPUID",		"",							"0F A2",			CT_CPU_PENT},
	{"CVTPI2PS",	"xmmreg,r/m64",				"0F 2A /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"CVTPS2PI",	"mmreg,xmmreg/mem64",		"0F 2D /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"CVTTPS2PI",	"mmreg,xmmreg/mem64",		"0F 2C /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"CVTSI2SS",	"xmmreg,r/m32",				"p3 0F 2A /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CVTSS2SI",	"reg32,xmmreg/mem32",		"p3 0F 2D /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"CVTTSS2SI",	"reg32,xmmreg/mem32",		"p3 0F 2C /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"DAA",			"",							"27",				CT_CPU_8086},
	{"DAS",			"",							"2F",				CT_CPU_8086},
	{"DEC",			"reg16",					"po 48 +r",			CT_CPU_8086},
	{"DEC",			"reg32",					"po 48 +r",			CT_CPU_386},
	{"DEC",			"BYTE r/m8",				"FE /1",			CT_CPU_8086},
	{"DEC",			"WORD r/m16",				"po FF /1",			CT_CPU_8086},
	{"DEC",			"DWORD r/m32",				"po FF /1",			CT_CPU_386},
	{"DIV",			"BYTE r/m8",				"F6 /6",			CT_CPU_8086},
	{"DIV",			"WORD r/m16",				"po F7 /6",			CT_CPU_8086},
	{"DIV",			"DWORD r/m32",				"po F7 /6",			CT_CPU_386},
	{"DIVPS",		"xmmreg,r/m128",			"0F 5E /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"DIVSS",		"xmmreg,xmmreg/mem32",		"p3 0F 5E /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"EMMS",		"",							"0F 77",			CT_CPU_PENT | CT_CPU_MMX},
//	{"ENTER",		"imm,imm",					"C8 iw ib",			CT_CPU_186},
	{"F2XM1",		"",							"D9 F0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FABS",		"",							"D9 E1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADD",		"DWORD mem32",				"D8 /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADD",		"QWORD mem64",				"DC /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADD",		"fpureg",					"D8 C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADD",		"ST0,fpureg",				"D8 C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FADD",		"TO fpureg",				"DC C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADD",		"fpureg,ST0",				"DC C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADDP",		"fpureg",					"DE C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FADDP",		"fpureg,ST0",				"DE C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FBLD",		"mem80",					"DF /4",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FBSTP",		"mem80",					"DF /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCHS",		"",							"D9 E0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCLEX",		"",							"9B DB E2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNCLEX",		"",							"DB E2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCMOVB",		"fpureg",					"DA C0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVB",		"ST0,fpureg",				"DA C0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVBE",		"fpureg",					"DA D0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVBE",		"ST0,fpureg",				"DA D0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVE",		"fpureg",					"DA C8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVE",		"ST0,fpureg",				"DA C8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNB",		"fpureg",					"DB C0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNB",		"ST0,fpureg",				"DB C0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNBE",	"fpureg",					"DB D0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNBE",	"ST0,fpureg",				"DB D0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNE",		"fpureg",					"DB C8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNE",		"ST0,fpureg",				"DB C8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNU",		"fpureg",					"DB D8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVNU",		"ST0,fpureg",				"DB D8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVU",		"fpureg",					"DA D8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCMOVU",		"ST0,fpureg",				"DA D8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCOM",		"DWORD mem32",				"D8 /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOM",		"QWORD mem64",				"DC /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOM",		"fpureg",					"D8 D0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOM",		"ST0,fpureg",				"D8 D0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMP",		"DWORD mem32",				"D8 /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMP",		"QWORD mem64",				"DC /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMP",		"fpureg",					"D8 D8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMP",		"ST0,fpureg",				"D8 D8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMPP",		"",							"DE D9",			CT_CPU_8086 | CT_CPU_FPU},
	{"FCOMI",		"fpureg",					"DB F0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCOMI",		"ST0,fpureg",				"DB F0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCOMIP",		"fpureg",					"DF F0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCOMIP",		"ST0,fpureg",				"DF F0 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FCOS",		"",							"D9 FF",			CT_CPU_386 | CT_CPU_FPU},
	{"FDECSTP",		"",							"D9 F6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDISI",		"",							"9B DB E1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNDISI",		"",							"DB E1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FENI",		"",							"9B DB E0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNENI",		"",							"DB E0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIV",		"DWORD mem32",				"D8 /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIV",		"QWORD mem64",				"DC /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIV",		"fpureg",					"D8 F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIV",		"ST0,fpureg",				"D8 F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FDIV",		"TO fpureg",				"DC F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIV",		"fpureg,ST0",				"DC F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVR",		"DWORD mem32",				"D8 /7",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVR",		"QWORD mem64",				"DC /7",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVR",		"fpureg",					"D8 F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVR",		"ST0,fpureg",				"D8 F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FDIVR",		"TO fpureg",				"DC F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVR",		"fpureg,ST0",				"DC F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVP",		"fpureg",					"DE F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVP",		"fpureg,ST0",				"DE F8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVRP",		"fpureg",					"DE F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FDIVRP",		"fpureg,ST0",				"DE F0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FEMMS",		"",							"0F 0E",			CT_CPU_3DNOW},
	{"FFREE",		"fpureg",					"DD C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FFREEP",		"fpureg",					"DF C0 +r",			CT_CPU_P6 | CT_CPU_FPU | CT_CPU_UNDOC},
	{"FIADD",		"WORD mem16",				"DE /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIADD",		"DWORD mem32",				"DA /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FICOM",		"WORD mem16",				"DE /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FICOM",		"DWORD mem32",				"DA /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FICOMP",		"WORD mem16",				"DE /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FICOMP",		"DWORD mem32",				"DA /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIDIV",		"WORD mem16",				"DE /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIDIV",		"DWORD mem32",				"DA /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIDIVR",		"WORD mem16",				"DE /7",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIDIVR",		"DWORD mem32",				"DA /7",			CT_CPU_8086 | CT_CPU_FPU},
	{"FILD",		"WORD mem16",				"DF /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FILD",		"DWORD mem32",				"DB /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FILD",		"QWORD mem64",				"DF /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIST",		"WORD mem16",				"DF /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIST",		"DWORD mem32",				"DB /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISTP",		"WORD mem16",				"DF /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISTP",		"DWORD mem32",				"DB /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISTP",		"QWORD mem64",				"DF /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIMUL",		"WORD mem16",				"DE /1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FIMUL",		"DWORD mem32",				"DA /1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FINCSTP",		"",							"D9 F7",			CT_CPU_8086 | CT_CPU_FPU},
	{"FINIT",		"",							"9B DB E3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNINIT",		"",							"DB E3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISUB",		"WORD mem16",				"DE /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISUB",		"DWORD mem32",				"DA /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISUBR",		"WORD mem16",				"DE /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FISUBR",		"DWORD mem32",				"DA /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLD",			"DWORD mem32",				"D9 /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLD",			"QWORD mem64",				"DD /0",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FLD",			"mem80",					"DB /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLD",			"fpureg",					"D9 C0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLD1",		"",							"D9 E8",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDL2E",		"",							"D9 EA",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDL2T",		"",							"D9 E9",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDLG2",		"",							"D9 EC",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDLN2",		"",							"D9 ED",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDPI",		"",							"D9 EB",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDZ",		"",							"D9 EE",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDCW",		"mem16",					"D9 /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FLDENV",		"mem",						"D9 /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMUL",		"DWORD mem32",				"D8 /1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMUL",		"QWORD mem64",				"DC /1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMUL",		"fpureg",					"D8 C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMUL",		"ST0,fpureg",				"D8 C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FMUL",		"TO fpureg",				"DC C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMUL",		"fpureg,ST0",				"DC C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMULP",		"fpureg",					"DE C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FMULP",		"fpureg,ST0",				"DE C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNOP",		"",							"D9 D0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FPATAN",		"",							"D9 F3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FPTAN",		"",							"D9 F2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FPREM",		"",							"D9 F8",			CT_CPU_8086 | CT_CPU_FPU},
	{"FPREM1",		"",							"D9 F5",			CT_CPU_386 | CT_CPU_FPU},
	{"FRNDINT",		"",							"D9 FC",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSAVE",		"mem",						"9B DD /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNSAVE",		"mem",						"DD /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FRSTOR",		"mem",						"DD /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSCALE",		"",							"D9 FD",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSETPM",		"",							"DB E4",			CT_CPU_286 | CT_CPU_FPU},
	{"FSIN",		"",							"D9 FE",			CT_CPU_386 | CT_CPU_FPU},
	{"FSINCOS",		"",							"D9 FB",			CT_CPU_386 | CT_CPU_FPU},
	{"FSQRT",		"",							"D9 FA",			CT_CPU_8086 | CT_CPU_FPU},
	{"FST",			"DWORD mem32",				"D9 /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FST",			"QWORD mem64",				"DD /2",			CT_CPU_8086 | CT_CPU_FPU},
	{"FST",			"fpureg",					"DD D0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTP",		"DWORD mem32",				"D9 /3",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTP",		"QWORD mem64",				"DD /3",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FSTP",		"mem80",					"DB /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTP",		"fpureg",					"DD D8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTCW",		"mem16",					"9B D9 /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNSTCW",		"mem16",					"D9 /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTENV",		"mem",						"9B D9 /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNSTENV",		"mem",						"D9 /6",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTSW",		"mem16",					"9B DD /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSTSW",		"AX",						"9B DF E0",			CT_CPU_286 | CT_CPU_FPU},
	{"FNSTSW",		"mem16",					"DD /0",			CT_CPU_8086 | CT_CPU_FPU},
	{"FNSTSW",		"AX",						"DF E0",			CT_CPU_286 | CT_CPU_FPU},
	{"FSUB",		"DWORD mem32",				"D8 /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUB",		"QWORD mem64",				"DC /4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUB",		"fpureg",					"D8 E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUB",		"ST0,fpureg",				"D8 E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FSUB",		"TO fpureg",				"DC E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUB",		"fpureg,ST0",				"DC E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBR",		"DWORD mem32",				"D8 /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBR",		"QWORD mem64",				"DC /5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBR",		"fpureg",					"D8 E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBR",		"ST0,fpureg",				"D8 E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FSUBR",		"TO fpureg",				"DC E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBR",		"fpureg,ST0",				"DC E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBP",		"fpureg",					"DE E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBP",		"fpureg,ST0",				"DE E8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBRP",		"fpureg",					"DE E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FSUBRP",		"fpureg,ST0",				"DE E0 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FTST",		"",							"D9 E4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FUCOM",		"fpureg",					"DD E0 +r",			CT_CPU_386 | CT_CPU_FPU},
	{"FUCOM",		"ST0,fpureg",				"DD E0 +r",			CT_CPU_386 | CT_CPU_FPU},
	{"FUCOMP",		"fpureg",					"DD E8 +r",			CT_CPU_386 | CT_CPU_FPU},
	{"FUCOMP",		"ST0,fpureg",				"DD E8 +r",			CT_CPU_386 | CT_CPU_FPU},
	{"FUCOMPP",		"",							"DA E9",			CT_CPU_386 | CT_CPU_FPU},
	{"FUCOMI",		"fpureg",					"DB E8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FUCOMI",		"ST0,fpureg",				"DB E8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FUCOMIP",		"fpureg",					"DF E8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FUCOMIP",		"ST0,fpureg",				"DF E8 +r",			CT_CPU_P6 | CT_CPU_FPU},
	{"FWAIT",		"",							"9B",				CT_CPU_8086},
	{"FXAM",		"",							"D9 E5",			CT_CPU_8086 | CT_CPU_FPU},
	{"FXCH",		"",							"D9 C9",			CT_CPU_8086 | CT_CPU_FPU},
	{"FXCH",		"fpureg",					"D9 C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FXCH",		"fpureg,ST0",				"D9 C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
	{"FXCH",		"ST0,fpureg",				"D9 C8 +r",			CT_CPU_8086 | CT_CPU_FPU},
//	{"FXRSTOR",		"m512byte",					"0F AE /1",			CT_CPU_P6 | CT_CPU_SSE | CT_CPU_FPU},
//	{"FXSAVE",		"m512byte",					"0F AE /0",			CT_CPU_P6 | CT_CPU_SSE | CT_CPU_FPU},
	{"FXTRACT",		"",							"D9 F4",			CT_CPU_8086 | CT_CPU_FPU},
	{"FYL2X",		"",							"D9 F1",			CT_CPU_8086 | CT_CPU_FPU},
	{"FYL2XP1",		"",							"D9 F9",			CT_CPU_8086 | CT_CPU_FPU},
	{"HLT",			"",							"F4",				CT_CPU_8086},
//	{"IBTS",		"r/m16,reg16",				"po 0F A7 /r",		CT_CPU_386 | CT_CPU_UNDOC},
//	{"IBTS",		"r/m32,reg32",				"po 0F A7 /r",		CT_CPU_386 | CT_CPU_UNDOC},
	{"IDIV",		"BYTE r/m8",				"F6 /7",			CT_CPU_8086},
	{"IDIV",		"WORD r/m16",				"po F7 /7",			CT_CPU_8086},
	{"IDIV",		"DWORD r/m32",				"po F7 /7",			CT_CPU_386},
	{"IMUL",		"BYTE r/m8",				"F6 /5",			CT_CPU_8086},
	{"IMUL",		"WORD r/m16",				"po F7 /5",			CT_CPU_8086},
	{"IMUL",		"DWORD r/m32",				"po F7 /5",			CT_CPU_386},
	{"IMUL",		"reg16,r/m16",				"po 0F AF /r",		CT_CPU_386},
	{"IMUL",		"reg32,r/m32",				"po 0F AF /r",		CT_CPU_386},
	{"IMUL",		"reg16,imm8",				"po 6B /r ib",		CT_CPU_286},
	{"IMUL",		"reg16,imm16",				"po 69 /r iw",		CT_CPU_286},
	{"IMUL",		"reg32,imm8",				"po 6B /r ib",		CT_CPU_386},
	{"IMUL",		"reg32,imm32",				"po 69 /r id",		CT_CPU_386},
	{"IMUL",		"reg16,r/m16,imm8",			"po 6B /r ib",		CT_CPU_286},
	{"IMUL",		"reg16,r/m16,imm16",		"po 69 /r iw",		CT_CPU_286},
	{"IMUL",		"reg32,r/m32,imm8",			"po 6B /r ib",		CT_CPU_386},
	{"IMUL",		"reg32,r/m32,imm32",		"po 69 /r id",		CT_CPU_386},
	{"IN",			"AL,imm8",					"E4 ib",			CT_CPU_8086},
	{"IN",			"AX,imm8",					"po E5 ib",			CT_CPU_8086},
	{"IN",			"EAX,imm8",					"po E5 ib",			CT_CPU_386},
	{"IN",			"AL,DX",					"EC",				CT_CPU_8086},
	{"IN",			"AX,DX",					"po ED",			CT_CPU_8086},
	{"IN",			"EAX,DX",					"po ED",			CT_CPU_386},
	{"INC",			"reg16",					"po 40 +r",			CT_CPU_8086},
	{"INC",			"reg32",					"po 40 +r",			CT_CPU_386},
	{"INC",			"BYTE r/m8",				"FE /0",			CT_CPU_8086},
	{"INC",			"WORD r/m16",				"po FF /0",			CT_CPU_8086},
	{"INC",			"DWORD r/m32",				"po FF /0",			CT_CPU_386},
	{"INSB",		"",							"6C",				CT_CPU_186},
	{"INSW",		"",							"po 6D",			CT_CPU_186},
	{"INSD",		"",							"po 6D",			CT_CPU_386},
	{"INT",			"imm8",						"CD ib",			CT_CPU_8086},
	{"INT1",		"",							"F1",				CT_CPU_P6},
	{"ICEBP",		"",							"F1",				CT_CPU_P6},
	{"INT01",		"",							"F1",				CT_CPU_P6},
	{"INT3",		"",							"CC",				CT_CPU_8086},
	{"INT03",		"",							"CC",				CT_CPU_8086},
	{"INTO",		"",							"CE",				CT_CPU_8086},
	{"INVD",		"",							"0F 08",			CT_CPU_486},
	{"INVLPG",		"mem",						"0F 01 /0",			CT_CPU_486},
	{"IRET",		"",							"CF",				CT_CPU_8086},
	{"IRETW",		"",							"po CF",			CT_CPU_8086},
	{"IRETD",		"",							"po CF",			CT_CPU_386},
	{"JCXZ",		"imm",						"po E3 -b",			CT_CPU_8086},
	{"JECXZ",		"imm",						"po E3 -b",			CT_CPU_386},
	{"JMP",			"imm",						"E9 -i",			CT_CPU_8086},
	{"JMP",			"SHORT imm",				"EB -b",			CT_CPU_8086},
//	{"JMP",			"imm:imm16",				"po EA iw iw",		CT_CPU_8086},
//	{"JMP",			"imm:imm32",				"po EA id iw",		CT_CPU_386},
	{"JMP",			"mem",						"po FF /5",			CT_CPU_8086},
//	{"JMP",			"FAR mem",					"po FF /5",			CT_CPU_386},
	{"JMP",			"WORD r/m16",				"po FF /4",			CT_CPU_8086},
	{"JMP",			"DWORD r/m32",				"po FF /4",			CT_CPU_386},
	{"JO",			"imm",						"70 -b",			CT_CPU_8086},
	{"JNO",			"imm",						"71 -b",			CT_CPU_8086},
	{"JB",			"imm",						"72 -b",			CT_CPU_8086},
	{"JC",			"imm",						"72 -b",			CT_CPU_8086},
	{"JNEA",		"imm",						"72 -b",			CT_CPU_8086},
	{"JAE",			"imm",						"73 -b",			CT_CPU_8086},
	{"JNB",			"imm",						"73 -b",			CT_CPU_8086},
	{"JNC",			"imm",						"73 -b",			CT_CPU_8086},
	{"JE",			"imm",						"74 -b",			CT_CPU_8086},
	{"JZ",			"imm",						"74 -b",			CT_CPU_8086},
	{"JNE",			"imm",						"75 -b",			CT_CPU_8086},
	{"JNZ",			"imm",						"75 -b",			CT_CPU_8086},
	{"JBE",			"imm",						"76 -b",			CT_CPU_8086},
	{"JNA",			"imm",						"76 -b",			CT_CPU_8086},
	{"JA",			"imm",						"77 -b",			CT_CPU_8086},
	{"JNBE",		"imm",						"77 -b",			CT_CPU_8086},
	{"JS",			"imm",						"78 -b",			CT_CPU_8086},
//	{"JNS",			"imm",						"79 -b",			CT_CPU_8086},
	{"JP",			"imm",						"7A -b",			CT_CPU_8086},
	{"JPE",			"imm",						"7A -b",			CT_CPU_8086},
	{"JNP",			"imm",						"7B -b",			CT_CPU_8086},
	{"JPO",			"imm",						"7B -b",			CT_CPU_8086},
	{"JL",			"imm",						"7C -b",			CT_CPU_8086},
	{"JNGE",		"imm",						"7C -b",			CT_CPU_8086},
	{"JGE",			"imm",						"7D -b",			CT_CPU_8086},
	{"JNL",			"imm",						"7D -b",			CT_CPU_8086},
	{"JLE",			"imm",						"7E -b",			CT_CPU_8086},
	{"JNG",			"imm",						"7E -b",			CT_CPU_8086},
	{"JG",			"imm",						"7F -b",			CT_CPU_8086},
	{"JNLE",		"imm",						"7F -b",			CT_CPU_8086},
	{"JO",			"NEAR imm",					"0F 80 -i",			CT_CPU_386},
	{"JNO",			"NEAR imm",					"0F 81 -i",			CT_CPU_386},
	{"JB",			"NEAR imm",					"0F 82 -i",			CT_CPU_386},
	{"JC",			"NEAR imm",					"0F 82 -i",			CT_CPU_386},
	{"JNEA",		"NEAR imm",					"0F 82 -i",			CT_CPU_386},
	{"JAE",			"NEAR imm",					"0F 83 -i",			CT_CPU_386},
	{"JNB",			"NEAR imm",					"0F 83 -i",			CT_CPU_386},
	{"JNC",			"NEAR imm",					"0F 83 -i",			CT_CPU_386},
	{"JE",			"NEAR imm",					"0F 84 -i",			CT_CPU_386},
	{"JZ",			"NEAR imm",					"0F 84 -i",			CT_CPU_386},
	{"JNE",			"NEAR imm",					"0F 85 -i",			CT_CPU_386},
	{"JNZ",			"NEAR imm",					"0F 85 -i",			CT_CPU_386},
	{"JBE",			"NEAR imm",					"0F 86 -i",			CT_CPU_386},
	{"JNA",			"NEAR imm",					"0F 86 -i",			CT_CPU_386},
	{"JA",			"NEAR imm",					"0F 87 -i",			CT_CPU_386},
	{"JNBE",		"NEAR imm",					"0F 87 -i",			CT_CPU_386},
	{"JS",			"NEAR imm",					"0F 88 -i",			CT_CPU_386},
	{"JNS",			"NEAR imm",					"0F 89 -i",			CT_CPU_386},
	{"JP",			"NEAR imm",					"0F 8A -i",			CT_CPU_386},
	{"JPE",			"NEAR imm",					"0F 8A -i",			CT_CPU_386},
	{"JNP",			"NEAR imm",					"0F 8B -i",			CT_CPU_386},
	{"JPO",			"NEAR imm",					"0F 8B -i",			CT_CPU_386},
	{"JL",			"NEAR imm",					"0F 8C -i",			CT_CPU_386},
	{"JNGE",		"NEAR imm",					"0F 8C -i",			CT_CPU_386},
	{"JGE",			"NEAR imm",					"0F 8D -i",			CT_CPU_386},
	{"JNL",			"NEAR imm",					"0F 8D -i",			CT_CPU_386},
	{"JLE",			"NEAR imm",					"0F 8E -i",			CT_CPU_386},
	{"JNG",			"NEAR imm",					"0F 8E -i",			CT_CPU_386},
	{"JG",			"NEAR imm",					"0F 8F -i",			CT_CPU_386},
	{"JNLE",		"NEAR imm",					"0F 8F -i",			CT_CPU_386},
	{"LAHF",		"",							"9F",				CT_CPU_8086},
//	{"LAR",			"reg16,r/m16",				"po 0F 02 /r",		CT_CPU_286 | CT_CPU_PRIV},
//	{"LAR",			"reg32,r/m32",				"po 0F 02 /r",		CT_CPU_286 | CT_CPU_PRIV},
	{"LDS",			"reg16,mem",				"po C5 /r",			CT_CPU_8086},
	{"LDS",			"reg32,mem",				"po C5 /r",			CT_CPU_8086},
	{"LES",			"reg16,mem",				"po C4 /r",			CT_CPU_8086},
	{"LES",			"reg32,mem",				"po C4 /r",			CT_CPU_8086},
	{"LFS",			"reg16,mem",				"po 0F B4 /r",		CT_CPU_386},
	{"LFS",			"reg32,mem",				"po 0F B4 /r",		CT_CPU_386},
	{"LGS",			"reg16,mem",				"po 0F B5 /r",		CT_CPU_386},
	{"LGS",			"reg32,mem",				"po 0F B5 /r",		CT_CPU_386},
	{"LSS",			"reg16,mem",				"po 0F B2 /r",		CT_CPU_386},
	{"LSS",			"reg32,mem",				"po 0F B2 /r",		CT_CPU_386},
	{"LDMXCSR",		"mem32",					"0F AE /2",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"LEA",			"reg16,mem",				"po 8D /r",			CT_CPU_8086},
	{"LEA",			"reg32,mem",				"po 8D /r",			CT_CPU_386},
	{"LEAVE",		"",							"C9",				CT_CPU_186},
//	{"LGDT",		"mem",						"0F 01 /2",			CT_CPU_286 | CT_CPU_PRIV},
//	{"LIDT",		"mem",						"0F 01 /3",			CT_CPU_286 | CT_CPU_PRIV},
//	{"LLDT",		"r/m16",					"0F 00 /2",			CT_CPU_286 | CT_CPU_PRIV},
//	{"LMSW",		"r/m16",					"0F 01 /6",			CT_CPU_286 | CT_CPU_PRIV},
//	{"LOADALL",		"",							"0F 07",			CT_CPU_386 | CT_CPU_UNDOC},
//	{"LOADALL286",	"",							"0F 05",			CT_CPU_286 | CT_CPU_UNDOC},
	{"LODSB",		"",							"AC",				CT_CPU_8086},
	{"LODSW",		"",							"po AD",			CT_CPU_8086},
	{"LODSD",		"",							"po AD",			CT_CPU_386},
	{"LOOP",		"imm",						"E2 -b",			CT_CPU_8086},
	{"LOOP",		"imm,CX",					"pa E2 -b",			CT_CPU_8086},
	{"LOOP",		"imm,ECX",					"pa E2 -b",			CT_CPU_386},
	{"LOOPE",		"imm",						"E1 -b",			CT_CPU_8086},
	{"LOOPE",		"imm,CX",					"pa E1 -b",			CT_CPU_8086},
	{"LOOPE",		"imm,ECX",					"pa E1 -b",			CT_CPU_386},
	{"LOOPZ",		"imm",						"E1 -b",			CT_CPU_8086},
	{"LOOPZ",		"imm,CX",					"pa E1 -b",			CT_CPU_8086},
	{"LOOPZ",		"imm,ECX",					"pa E1 -b",			CT_CPU_386},
	{"LOOPNE",		"imm",						"E0 -b",			CT_CPU_8086},
	{"LOOPNE",		"imm,CX",					"pa E0 -b",			CT_CPU_8086},
	{"LOOPNE",		"imm,ECX",					"pa E0 -b",			CT_CPU_386},
	{"LOOPNZ",		"imm",						"E0 -b",			CT_CPU_8086},
	{"LOOPNZ",		"imm,CX",					"pa E0 -b",			CT_CPU_8086},
	{"LOOPNZ",		"imm,ECX",					"pa E0 -b",			CT_CPU_386},
//	{"LSL",			"reg16,r/m16",				"po 0F 03 /r",		CT_CPU_286 | CT_CPU_PRIV},
//	{"LSL",			"reg32,r/m32",				"po 0F 03 /r",		CT_CPU_286 | CT_CPU_PRIV},
//	{"LTR",			"r/m16",					"0F 00 /3",			CT_CPU_286 | CT_CPU_PRIV},
	{"MASKMOVQ",	"mmreg,mmreg",				"0F F7 /r",			CT_CPU_KATMAI},
//	{"MAXPS",		"xmmreg,r/m128",			"0F 5F /r",			CT_CPU_KATMAI | CT_CPU_SSE},
//	{"MAXSS",		"xmmreg,r/m128",			"p3 0F 5F /r",		CT_CPU_KATMAI | CT_CPU_SSE},
//	{"MINPS",		"xmmreg,r/m128",			"0F 5D /r",			CT_CPU_KATMAI | CT_CPU_SSE},
//	{"MINSS",		"xmmreg,r/m128",			"p3 0F 5D /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOV",			"r/m8,reg8",				"88 /r",			CT_CPU_8086},
	{"MOV",			"r/m16,reg16",				"po 89 /r",			CT_CPU_8086},
	{"MOV",			"r/m32,reg32",				"po 89 /r",			CT_CPU_386},
	{"MOV",			"reg8,r/m8",				"8A /r",			CT_CPU_8086},
	{"MOV",			"reg16,r/m16",				"po 8B /r",			CT_CPU_8086},
	{"MOV",			"reg32,r/m32",				"po 8B /r",			CT_CPU_386},
	{"MOV",			"reg8,imm8",				"B0 +r ib",			CT_CPU_8086},
	{"MOV",			"reg16,imm16",				"po B8 +r iw",		CT_CPU_8086},
	{"MOV",			"reg32,imm32",				"po B8 +r id",		CT_CPU_386},
	{"MOV",			"r/m8,imm8",				"C6 /0 ib",			CT_CPU_8086},
	{"MOV",			"r/m16,imm16",				"po C7 /0 iw",		CT_CPU_8086},
	{"MOV",			"r/m32,imm32",				"po C7 /0 id",		CT_CPU_386},
//	{"MOV",			"AL,memoffs8",				"A0 id",			CT_CPU_8086},
//	{"MOV",			"AX,memoffs16",				"po A1 id",			CT_CPU_8086},
//	{"MOV",			"EAX,memoffs32",			"po A1 id",			CT_CPU_386},
//	{"MOV",			"memoffs8,AL",				"A2 id",			CT_CPU_8086},
//	{"MOV",			"memoffs16,AX",				"po A3 id",			CT_CPU_8086},
//	{"MOV",			"memoffs32,EAX",			"po A3 id",			CT_CPU_386},
//	{"MOV",			"r/m16,segreg",				"po 8C /r",			CT_CPU_8086},
//	{"MOV",			"r/m32,segreg",				"po 8C /r",			CT_CPU_386},
//	{"MOV",			"segreg,r/m16",				"po 8E /r",			CT_CPU_8086},
//	{"MOV",			"segreg,r/m32",				"po 8E /r",			CT_CPU_386},
//	{"MOV",			"reg32,CR0/2/3/4",			"0F 20 /r",			CT_CPU_386},
//	{"MOV",			"reg32,DR0/1/2/3/6/7",		"0F 21 /r",			CT_CPU_386},
//	{"MOV",			"reg32,TR3/4/5/6/7",		"0F 24 /r",			CT_CPU_386},
//	{"MOV",			"CR0/2/3/4,reg32",			"0F 22 /r",			CT_CPU_386},
//	{"MOV",			"DR0/1/2/3/6/7,reg32",		"0F 23 /r",			CT_CPU_386},
//	{"MOV",			"TR3/4/5/6/7,reg32",		"0F 26 /r",			CT_CPU_386},
	{"MOVAPS",		"xmmreg,r/m128",			"0F 28 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVAPS",		"r/m128,xmmreg",			"0F 29 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVD",		"mmreg,r/m32",				"0F 6E /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"MOVD",		"r/m32,mmreg",				"0F 7E /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"MOVHLPS",		"xmmreg,xmmreg",			"0F 12 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVHPS",		"xmmreg,mem64",				"0F 16 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVHPS",		"mem64,xmmreg",				"0F 17 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVHPS",		"xmmreg,xmmreg",			"0F 16 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVLHPS",		"xmmreg,xmmreg",			"0F 16 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVLPS",		"xmmreg,mem64",				"0F 12 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVLPS",		"mem64,xmmreg",				"0F 13 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVLPS",		"xmmreg,xmmreg",			"0F 12 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVMSKPS",	"reg32,xmmreg",				"0F 50 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVNTPS",		"mem128,xmmreg",			"0F 2B /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVNTQ",		"mem64,mmreg",				"0F E7 /r",			CT_CPU_KATMAI},
	{"MOVQ",		"mmreg,r/m64",				"0F 6F /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"MOVQ",		"r/m64,mmreg",				"0F 7F /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"MOVSB",		"",							"A4",				CT_CPU_8086},
	{"MOVSW",		"",							"po A5",			CT_CPU_8086},
	{"MOVSD",		"",							"po A5",			CT_CPU_386},
	{"MOVSS",		"xmmreg,xmmreg/mem32",		"p3 0F 10 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVSS",		"xmmreg/mem32,xmmreg",		"p3 0F 11 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVSX",		"reg16,r/m8",				"po 0F BE /r",		CT_CPU_386},
	{"MOVSX",		"reg32,r/m8",				"po 0F BE /r",		CT_CPU_386},
	{"MOVSX",		"reg32,r/m16",				"po 0F BF /r",		CT_CPU_386},
	{"MOVZX",		"reg16,r/m8",				"po 0F B6 /r",		CT_CPU_386},
	{"MOVZX",		"reg32,r/m8",				"po 0F B6 /r",		CT_CPU_386},
	{"MOVZX",		"reg32,r/m16",				"po 0F B7 /r",		CT_CPU_386},
	{"MOVUPS",		"xmmreg,r/m128",			"0F 10 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MOVUPS",		"r/m128,xmmreg",			"0F 11 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MUL",			"BYTE r/m8",				"F6 /4",			CT_CPU_8086},
	{"MUL",			"WORD r/m16",				"po F7 /4",			CT_CPU_8086},
	{"MUL",			"DWORD r/m32",				"po F7 /4",			CT_CPU_386},
	{"MULPS",		"xmmreg,r/m128",			"0F 59 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"MULSS",		"xmmreg,xmmreg/mem32",		"p3 0F 59 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"NEG",			"BYTE r/m8",				"F6 /3",			CT_CPU_8086},
	{"NEG",			"WORD r/m16",				"po F7 /3",			CT_CPU_8086},
	{"NEG",			"DWORD r/m32",				"po F7 /3",			CT_CPU_386},
	{"NOT",			"BYTE r/m8",				"F6 /2",			CT_CPU_8086},
	{"NOT",			"WORD r/m16",				"po F7 /2",			CT_CPU_8086},
	{"NOT",			"DWORD r/m32",				"po F7 /2",			CT_CPU_386},
	{"NOP",			"",							"90",				CT_CPU_8086},
	{"OR",			"r/m8,reg8",				"08 /r",			CT_CPU_8086},
	{"OR",			"r/m16,reg16",				"po 09 /r",			CT_CPU_8086},
	{"OR",			"r/m32,reg32",				"po 09 /r",			CT_CPU_386},
	{"OR",			"reg8,r/m8",				"0A /r",			CT_CPU_8086},
	{"OR",			"reg16,r/m16",				"po 0B /r",			CT_CPU_8086},
	{"OR",			"reg32,r/m32",				"po 0B /r",			CT_CPU_386},
	{"OR",			"r/m8,imm8",				"80 /1 ib",			CT_CPU_8086},
	{"OR",			"r/m16,imm16",				"po 81 /1 iw",		CT_CPU_8086},
	{"OR",			"r/m32,imm32",				"po 81 /1 id",		CT_CPU_386},
	{"OR",			"r/m16,imm8",				"po 83 /1 ib",		CT_CPU_8086},
	{"OR",			"r/m32,imm8",				"po 83 /1 ib",		CT_CPU_386},
	{"OR",			"AL,imm8",					"0C ib",			CT_CPU_8086},
	{"OR",			"AX,imm16",					"po 0D iw",			CT_CPU_8086},
	{"OR",			"EAX,imm32",				"po 0D id",			CT_CPU_386},
	{"ORPS",		"xmmreg,r/m128",			"0F 56 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"OUT",			"imm8,AL",					"E6 ib",			CT_CPU_8086},
	{"OUT",			"imm8,AX",					"po E7 ib",			CT_CPU_8086},
	{"OUT",			"imm8,EAX",					"po E7 ib",			CT_CPU_386},
	{"OUT",			"DX,AL",					"EE",				CT_CPU_8086},
	{"OUT",			"DX,AX",					"po EF",			CT_CPU_8086},
	{"OUT",			"DX,EAX",					"po EF",			CT_CPU_386},
	{"OUTSB",		"",							"6E",				CT_CPU_186},
	{"OUTSW",		"",							"po 6F",			CT_CPU_186},
	{"OUTSD",		"",							"po 6F",			CT_CPU_386},
	{"PACKSSDW",	"mmreg,r/m64",				"0F 6B /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PACKSSWB",	"mmreg,r/m64",				"0F 63 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PACKUSWB",	"mmreg,r/m64",				"0F 67 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDB",		"mmreg,r/m64",				"0F FC /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDW",		"mmreg,r/m64",				"0F FD /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDD",		"mmreg,r/m64",				"0F FE /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDSB",		"mmreg,r/m64",				"0F EC /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDSW",		"mmreg,r/m64",				"0F ED /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDUSB",		"mmreg,r/m64",				"0F DC /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDUSW",		"mmreg,r/m64",				"0F DD /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PADDSIW",		"mmreg,r/m64",				"0F 51 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PAND",		"mmreg,r/m64",				"0F DB /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PANDN",		"mmreg,r/m64",				"0F DF /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PAVEB",		"mmreg,r/m64",				"0F 50 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PAVGB",		"mmreg,r/m64",				"0F E0 /r",			CT_CPU_KATMAI},
	{"PAVGW",		"mmreg,r/m64",				"0F E3 /r",			CT_CPU_KATMAI},
	{"PAVGUSB",		"mmreg,r/m64",				"0F 0F /r BF",		CT_CPU_3DNOW},
	{"PCMPEQB",		"mmreg,r/m64",				"0F 74 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PCMPEQW",		"mmreg,r/m64",				"0F 75 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PCMPEQD",		"mmreg,r/m64",				"0F 76 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PCMPGTB",		"mmreg,r/m64",				"0F 64 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PCMPGTW",		"mmreg,r/m64",				"0F 65 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PCMPGTD",		"mmreg,r/m64",				"0F 66 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PDISTIB",		"mmreg,mem64",				"0F 54 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PEXTRW",		"reg32,mmreg,imm8",			"0F C5 /r ib",		CT_CPU_KATMAI},
	{"PF2ID",		"mmreg,r/m64",				"0F 0F /r 1D",		CT_CPU_3DNOW},
	{"PF2IW",		"mmreg,r/m64",				"0F 0F /r 1C",		CT_CPU_ATHLON},
	{"PFACC",		"mmreg,r/m64",				"0F 0F /r AE",		CT_CPU_3DNOW},
	{"PFADD",		"mmreg,r/m64",				"0F 0F /r 9E",		CT_CPU_3DNOW},
	{"PFCMPEQ",		"mmreg,r/m64",				"0F 0F /r B0",		CT_CPU_3DNOW},
	{"PFCMPGE",		"mmreg,r/m64",				"0F 0F /r 90",		CT_CPU_3DNOW},
	{"PFCMPGT",		"mmreg,r/m64",				"0F 0F /r A0",		CT_CPU_3DNOW},
	{"PFMAX",		"mmreg,r/m64",				"0F 0F /r A4",		CT_CPU_3DNOW},
	{"PFMIN",		"mmreg,r/m64",				"0F 0F /r 94",		CT_CPU_3DNOW},
	{"PFMUL",		"mmreg,r/m64",				"0F 0F /r B4",		CT_CPU_3DNOW},
	{"PFNACC",		"mmreg,r/m64",				"0F 0F /r 8A",		CT_CPU_ATHLON},
	{"PFPNACC",		"mmreg,r/m64",				"0F 0F /r 8E",		CT_CPU_ATHLON},
	{"PFRCP",		"mmreg,r/m64",				"0F 0F /r 96",		CT_CPU_3DNOW},
	{"PFRCPIT1",	"mmreg,r/m64",				"0F 0F /r A6",		CT_CPU_3DNOW},
	{"PFRCPIT2",	"mmreg,r/m64",				"0F 0F /r B6",		CT_CPU_3DNOW},
	{"PFRSQIT1",	"mmreg,r/m64",				"0F 0F /r A7",		CT_CPU_3DNOW},
	{"PFRSQRT",		"mmreg,r/m64",				"0F 0F /r 97",		CT_CPU_3DNOW},
	{"PFSUB",		"mmreg,r/m64",				"0F 0F /r 9A",		CT_CPU_3DNOW},
	{"PFSUBR",		"mmreg,r/m64",				"0F 0F /r AA",		CT_CPU_3DNOW},
	{"PI2FD",		"mmreg,r/m64",				"0F 0F /r 0D",		CT_CPU_3DNOW},
	{"PI2FW",		"mmreg,r/m64",				"0F 0F /r 0C",		CT_CPU_ATHLON},
	{"PINSRW",		"mmreg,r/m16,imm8",			"0F C4 /r ib",		CT_CPU_KATMAI},
	{"PMACHRIW",	"mmreg,mem64",				"0F 5E /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMADDWD",		"mmreg,r/m64",				"0F F5 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PMAGW",		"mmreg,r/m64",				"0F 52 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMAXSW",		"mmreg,r/m64",				"0F EE /r",			CT_CPU_KATMAI},
	{"PMAXUB",		"mmreg,r/m64",				"0F DE /r",			CT_CPU_KATMAI},
	{"PMINSW",		"mmreg,r/m64",				"0F EA /r",			CT_CPU_KATMAI},
	{"PMINUB",		"mmreg,r/m64",				"0F DA /r",			CT_CPU_KATMAI},
	{"PMOVMSKB",	"reg32,mmreg",				"0F D7 /r",			CT_CPU_KATMAI},
	{"PMULHRWA",	"mmreg,r/m64",				"0F 0F /r B7",		CT_CPU_3DNOW},
	{"PMULHRWC",	"mmreg,r/m64",				"0F 59 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMULHRIW",	"mmreg,r/m64",				"0F 5D /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMULHUW",		"mmreg,r/m64",				"0F E4 /r",			CT_CPU_KATMAI},
	{"PMULHW",		"mmreg,r/m64",				"0F E5 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PMULLW",		"mmreg,r/m64",				"0F D5 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PMVZB",		"mmreg,mem64",				"0F 58 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMVNZB",		"mmreg,mem64",				"0F 5A /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMVLZB",		"mmreg,mem64",				"0F 5B /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PMVGEZB",		"mmreg,mem64",				"0F 5C /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"POP",			"reg16",					"po 58 +r",			CT_CPU_8086},
	{"POP",			"reg32",					"po 58 +r",			CT_CPU_386},
	{"POP",			"WORD r/m16",				"po 8F /0",			CT_CPU_8086},
	{"POP",			"DWORD r/m32",				"po 8F /0",			CT_CPU_386},
//	{"POP",			"CS",						"0F",				CT_CPU_8086 | CT_CPU_UNDOC},
//	{"POP",			"DS",						"1F",				CT_CPU_8086},
//	{"POP",			"ES",						"07",				CT_CPU_8086},
//	{"POP",			"SS",						"17",				CT_CPU_8086},
//	{"POP",			"FS",						"0F A1",			CT_CPU_386},
//	{"POP",			"GS",						"0F A9",			CT_CPU_386},
	{"POPA",		"",							"61",				CT_CPU_186},
	{"POPAW",		"",							"po 61",			CT_CPU_186},
	{"POPAD",		"",							"po 61",			CT_CPU_386},
	{"POPF",		"",							"9D",				CT_CPU_186},
	{"POPFW",		"",							"po 9D",			CT_CPU_186},
	{"POPFD",		"",							"po 9D",			CT_CPU_386},
	{"POR",			"mmreg,r/m64",				"0F EB /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PREFETCH",	"mem",						"0F 0D /0",			CT_CPU_3DNOW},
	{"PREFETCHW",	"mem",						"0F 0D /1",			CT_CPU_3DNOW},
	{"PREFETCHNTA",	"mem",						"0F 18 /0",			CT_CPU_KATMAI},
	{"PREFETCHT0",	"mem",						"0F 18 /1",			CT_CPU_KATMAI},
	{"PREFETCHT1",	"mem",						"0F 18 /2",			CT_CPU_KATMAI},
	{"PREFETCHT2",	"mem",						"0F 18 /3",			CT_CPU_KATMAI},
	{"PSADBW",		"mmreg,r/m64",				"0F F6 /r",			CT_CPU_KATMAI},
	{"PSHUFW",		"mmreg,r/m64,imm8",			"0F 70 /r ib",		CT_CPU_KATMAI},
	{"PSLLW",		"mmreg,r/m64",				"0F F1 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSLLW",		"mmreg,imm8",				"0F 71 /6 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSLLD",		"mmreg,r/m64",				"0F F2 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSLLD",		"mmreg,imm8",				"0F 72 /6 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSLLQ",		"mmreg,r/m64",				"0F F3 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSLLQ",		"mmreg,imm8",				"0F 73 /6 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSRAW",		"mmreg,r/m64",				"0F E1 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSRAW",		"mmreg,imm8",				"0F 71 /4 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSRAD",		"mmreg,r/m64",				"0F E2 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSRAD",		"mmreg,imm8",				"0F 72 /4 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLW",		"mmreg,r/m64",				"0F D1 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLW",		"mmreg,imm8",				"0F 71 /2 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLD",		"mmreg,r/m64",				"0F D2 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLD",		"mmreg,imm8",				"0F 72 /2 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLQ",		"mmreg,r/m64",				"0F D3 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSRLQ",		"mmreg,imm8",				"0F 73 /2 ib",		CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBB",		"mmreg,r/m64",				"0F F8 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBW",		"mmreg,r/m64",				"0F F9 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBD",		"mmreg,r/m64",				"0F FA /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBSB",		"mmreg,r/m64",				"0F E8 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBSW",		"mmreg,r/m64",				"0F E9 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBUSB",		"mmreg,r/m64",				"0F D8 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBUSW",		"mmreg,r/m64",				"0F D9 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PSUBSIW",		"mmreg,r/m64",				"0F 55 /r",			CT_CPU_CYRIX | CT_CPU_MMX},
	{"PSWAPD",		"mmreg,r/m64",				"0F 0F /r BB",		CT_CPU_ATHLON},
	{"PUNPCKHBW",	"mmreg,r/m64",				"0F 68 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUNPCKHWD",	"mmreg,r/m64",				"0F 69 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUNPCKHDQ",	"mmreg,r/m64",				"0F 6A /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUNPCKLBW",	"mmreg,r/m64",				"0F 60 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUNPCKLWD",	"mmreg,r/m64",				"0F 61 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUNPCKLDQ",	"mmreg,r/m64",				"0F 62 /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"PUSH",		"reg16",					"po 50 +r",			CT_CPU_8086},
	{"PUSH",		"reg32",					"po 50 +r",			CT_CPU_386},
	{"PUSH",		"WORD r/m16",				"po FF /6",			CT_CPU_8086},
	{"PUSH",		"DWORD r/m32",				"po FF /6",			CT_CPU_386},
//	{"PUSH",		"CS",						"0E",				CT_CPU_8086},
//	{"PUSH",		"DS",						"1E",				CT_CPU_8086},
//	{"PUSH",		"ES",						"06",				CT_CPU_8086},
//	{"PUSH",		"SS",						"16",				CT_CPU_8086},
//	{"PUSH",		"FS",						"0F A0",			CT_CPU_386},
//	{"PUSH",		"GS",						"0F A8",			CT_CPU_386},
	{"PUSH",		"imm8",						"6A ib",			CT_CPU_286},
	{"PUSH",		"imm16",					"po 68 iw",			CT_CPU_286},
	{"PUSH",		"imm32",					"po 68 id",			CT_CPU_386},
	{"PUSHA",		"",							"60",				CT_CPU_186},
	{"PUSHAD",		"",							"po 60",			CT_CPU_386},
	{"PUSHAW",		"",							"po 60",			CT_CPU_186},
	{"PUSHF",		"",							"9C",				CT_CPU_186},
	{"PUSHFD",		"",							"po 9C",			CT_CPU_386},
	{"PUSHFW",		"",							"po 9C",			CT_CPU_186},
	{"PXOR",		"mmreg,r/m64",				"0F EF /r",			CT_CPU_PENT | CT_CPU_MMX},
	{"RCL",			"r/m8,1",					"D0 /2",			CT_CPU_8086},
	{"RCL",			"r/m8,CL",					"D2 /2",			CT_CPU_8086},
	{"RCL",			"r/m8,imm8",				"C0 /2 ib",			CT_CPU_286},
	{"RCL",			"r/m16,1",					"po D1 /2",			CT_CPU_8086},
	{"RCL",			"r/m16,CL",					"po D3 /2",			CT_CPU_8086},
	{"RCL",			"r/m16,imm8",				"po C1 /2 ib",		CT_CPU_286},
	{"RCL",			"r/m32,1",					"po D1 /2",			CT_CPU_386},
	{"RCL",			"r/m32,CL",					"po D3 /2",			CT_CPU_386},
	{"RCL",			"r/m32,imm8",				"po C1 /2 ib",		CT_CPU_386},
	{"RCR",			"r/m8,1",					"D0 /3",			CT_CPU_8086},
	{"RCR",			"r/m8,CL",					"D2 /3",			CT_CPU_8086},
	{"RCR",			"r/m8,imm8",				"C0 /3 ib",			CT_CPU_286},
	{"RCR",			"r/m16,1",					"po D1 /3",			CT_CPU_8086},
	{"RCR",			"r/m16,CL",					"po D3 /3",			CT_CPU_8086},
	{"RCR",			"r/m16,imm8",				"po C1 /3 ib",		CT_CPU_286},
	{"RCR",			"r/m32,1",					"po D1 /3",			CT_CPU_386},
	{"RCR",			"r/m32,CL",					"po D3 /3",			CT_CPU_386},
	{"RCR",			"r/m32,imm8",				"po C1 /3 ib",		CT_CPU_386},
	{"RCPPS",		"xmmreg,r/m128",			"0F 53 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"RCPSS",		"xmmreg,xmmreg/mem32",		"p3 0F 53 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"RDMSR",		"",							"0F 32",			CT_CPU_PENT},
	{"RDPMC",		"",							"0F 33",			CT_CPU_P6},
//	{"RDSHR",		"",							"0F 36",			CT_CPU_P6 | CT_CPU_CYRIX | CT_CPU_SMM},
	{"RDTSC",		"",							"0F 31",			CT_CPU_PENT},
	{"RET",			"",							"C3",				CT_CPU_8086},
	{"RET",			"imm16",					"C2 iw",			CT_CPU_8086},
	{"RETF",		"",							"CB",				CT_CPU_8086},
	{"RETF",		"imm16",					"CA iw",			CT_CPU_8086},
	{"RETN",		"",							"C3",				CT_CPU_8086},
	{"RETN",		"imm16",					"C2 iw",			CT_CPU_8086},
	{"ROL",			"r/m8,1",					"D0 /0",			CT_CPU_8086},
	{"ROL",			"r/m8,CL",					"D2 /0",			CT_CPU_8086},
	{"ROL",			"r/m8,imm8",				"C0 /0 ib",			CT_CPU_286},
	{"ROL",			"r/m16,1",					"po D1 /0",			CT_CPU_8086},
	{"ROL",			"r/m16,CL",					"po D3 /0",			CT_CPU_8086},
	{"ROL",			"r/m16,imm8",				"po C1 /0 ib",		CT_CPU_286},
	{"ROL",			"r/m32,1",					"po D1 /0",			CT_CPU_386},
	{"ROL",			"r/m32,CL",					"po D3 /0",			CT_CPU_386},
	{"ROL",			"r/m32,imm8",				"po C1 /0 ib",		CT_CPU_386},
	{"ROR",			"r/m8,1",					"D0 /1",			CT_CPU_8086},
	{"ROR",			"r/m8,CL",					"D2 /1",			CT_CPU_8086},
	{"ROR",			"r/m8,imm8",				"C0 /1 ib",			CT_CPU_286},
	{"ROR",			"r/m16,1",					"po D1 /1",			CT_CPU_8086},
	{"ROR",			"r/m16,CL",					"po D3 /1",			CT_CPU_8086},
	{"ROR",			"r/m16,imm8",				"po C1 /1 ib",		CT_CPU_286},
	{"ROR",			"r/m32,1",					"po D1 /1",			CT_CPU_386},
	{"ROR",			"r/m32,CL",					"po D3 /1",			CT_CPU_386},
	{"ROR",			"r/m32,imm8",				"po C1 /1 ib",		CT_CPU_386},
//	{"RSDC",		"segreg,mem80",				"0F 79 /r",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
//	{"RSLDT",		"mem80",					"0F 7B /0",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
	{"RSM",			"",							"0F AA",			CT_CPU_PENT},
	{"RSQRTPS",		"xmmreg,r/m128",			"0F 52 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"RSQRTSS",		"xmmreg,r/m128",			"p3 0F 52 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
//	{"RSTS",		"mem80",					"0F 7D /0",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
	{"SAHF",		"",							"9E",				CT_CPU_8086},
	{"SAL",			"r/m8,1",					"D0 /4",			CT_CPU_8086},
	{"SAL",			"r/m8,CL",					"D2 /4",			CT_CPU_8086},
	{"SAL",			"r/m8,imm8",				"C0 /4 ib",			CT_CPU_286},
	{"SAL",			"r/m16,1",					"po D1 /4",			CT_CPU_8086},
	{"SAL",			"r/m16,CL",					"po D3 /4",			CT_CPU_8086},
	{"SAL",			"r/m16,imm8",				"po C1 /4 ib",		CT_CPU_286},
	{"SAL",			"r/m32,1",					"po D1 /4",			CT_CPU_386},
	{"SAL",			"r/m32,CL",					"po D3 /4",			CT_CPU_386},
	{"SAL",			"r/m32,imm8",				"po C1 /4 ib",		CT_CPU_386},
	{"SAR",			"r/m8,1",					"D0 /7",			CT_CPU_8086},
	{"SAR",			"r/m8,CL",					"D2 /7",			CT_CPU_8086},
	{"SAR",			"r/m8,imm8",				"C0 /7 ib",			CT_CPU_286},
	{"SAR",			"r/m16,1",					"po D1 /7",			CT_CPU_8086},
	{"SAR",			"r/m16,CL",					"po D3 /7",			CT_CPU_8086},
	{"SAR",			"r/m16,imm8",				"po C1 /7 ib",		CT_CPU_286},
	{"SAR",			"r/m32,1",					"po D1 /7",			CT_CPU_386},
	{"SAR",			"r/m32,CL",					"po D3 /7",			CT_CPU_386},
	{"SAR",			"r/m32,imm8",				"po C1 /7 ib",		CT_CPU_386},
//	{"SALC",		"",							"D6",				CT_CPU_8086 | CT_CPU_UNDOC},
	{"SBB",			"r/m8,reg8",				"18 /r",			CT_CPU_8086},
	{"SBB",			"r/m16,reg16",				"po 19 /r",			CT_CPU_8086},
	{"SBB",			"r/m32,reg32",				"po 19 /r",			CT_CPU_386},
	{"SBB",			"reg8,r/m8",				"1A /r",			CT_CPU_8086},
	{"SBB",			"reg16,r/m16",				"po 1B /r",			CT_CPU_8086},
	{"SBB",			"reg32,r/m32",				"po 1B /r",			CT_CPU_386},
	{"SBB",			"r/m8,imm8",				"80 /3 ib",			CT_CPU_8086},
	{"SBB",			"r/m16,imm16",				"po 81 /3 iw",		CT_CPU_8086},
	{"SBB",			"r/m32,imm32",				"po 81 /3 id",		CT_CPU_386},
	{"SBB",			"r/m16,imm8",				"po 83 /3 ib",		CT_CPU_8086},
	{"SBB",			"r/m32,imm8",				"po 83 /3 ib",		CT_CPU_8086},
	{"SBB",			"AL,imm8",					"1C ib",			CT_CPU_8086},
	{"SBB",			"AX,imm16",					"po 1D iw",			CT_CPU_8086},
	{"SBB",			"EAX,imm32",				"po 1D id",			CT_CPU_386},
	{"SCASB",		"",							"AE",				CT_CPU_8086},
	{"SCASW",		"",							"po AF",			CT_CPU_8086},
	{"SCASD",		"",							"po AF",			CT_CPU_386},
	{"SETO",		"BYTE r/m8",				"0F 90 /2",			CT_CPU_386},
	{"SETNO",		"BYTE r/m8",				"0F 91 /2",			CT_CPU_386},
	{"SETB",		"BYTE r/m8",				"0F 92 /2",			CT_CPU_386},
	{"SETC",		"BYTE r/m8",				"0F 92 /2",			CT_CPU_386},
	{"SETNEA",		"BYTE r/m8",				"0F 92 /2",			CT_CPU_386},
	{"SETAE",		"BYTE r/m8",				"0F 93 /2",			CT_CPU_386},
	{"SETNB",		"BYTE r/m8",				"0F 93 /2",			CT_CPU_386},
	{"SETNC",		"BYTE r/m8",				"0F 93 /2",			CT_CPU_386},
	{"SETE",		"BYTE r/m8",				"0F 94 /2",			CT_CPU_386},
	{"SETZ",		"BYTE r/m8",				"0F 94 /2",			CT_CPU_386},
	{"SETNE",		"BYTE r/m8",				"0F 95 /2",			CT_CPU_386},
	{"SETNZ",		"BYTE r/m8",				"0F 95 /2",			CT_CPU_386},
	{"SETBE",		"BYTE r/m8",				"0F 96 /2",			CT_CPU_386},
	{"SETNA",		"BYTE r/m8",				"0F 96 /2",			CT_CPU_386},
	{"SETA",		"BYTE r/m8",				"0F 97 /2",			CT_CPU_386},
	{"SETNBE",		"BYTE r/m8",				"0F 97 /2",			CT_CPU_386},
	{"SETS",		"BYTE r/m8",				"0F 98 /2",			CT_CPU_386},
	{"SETNS",		"BYTE r/m8",				"0F 99 /2",			CT_CPU_386},
	{"SETP",		"BYTE r/m8",				"0F 9A /2",			CT_CPU_386},
	{"SETPE",		"BYTE r/m8",				"0F 9A /2",			CT_CPU_386},
	{"SETNP",		"BYTE r/m8",				"0F 9B /2",			CT_CPU_386},
	{"SETPO",		"BYTE r/m8",				"0F 9B /2",			CT_CPU_386},
	{"SETL",		"BYTE r/m8",				"0F 9C /2",			CT_CPU_386},
	{"SETNGE",		"BYTE r/m8",				"0F 9C /2",			CT_CPU_386},
	{"SETGE",		"BYTE r/m8",				"0F 9D /2",			CT_CPU_386},
	{"SETNL",		"BYTE r/m8",				"0F 9D /2",			CT_CPU_386},
	{"SETLE",		"BYTE r/m8",				"0F 9E /2",			CT_CPU_386},
	{"SETNG",		"BYTE r/m8",				"0F 9E /2",			CT_CPU_386},
	{"SETG",		"BYTE r/m8",				"0F 9F /2",			CT_CPU_386},
	{"SETNLE",		"BYTE r/m8",				"0F 9F /2",			CT_CPU_386},
	{"SFENCE",		"",							"0F AE /7",			CT_CPU_KATMAI},
//	{"SGDT",		"mem",						"0F 01 /0",			CT_CPU_286 | CT_CPU_PRIV},
//	{"SIDT",		"mem",						"0F 01 /1",			CT_CPU_286 | CT_CPU_PRIV},
//	{"SLDT",		"r/m16",					"0F 00 /0",			CT_CPU_286 | CT_CPU_PRIV},
	{"SHL",			"BYTE r/m8,1",				"D0 /4",			CT_CPU_8086},
	{"SHL",			"BYTE r/m8,CL",				"D2 /4",			CT_CPU_8086},
	{"SHL",			"BYTE r/m8,imm8",			"C0 /4 ib",			CT_CPU_286},
	{"SHL",			"WORD r/m16,1",				"po D1 /4",			CT_CPU_8086},
	{"SHL",			"WORD r/m16,CL",			"po D3 /4",			CT_CPU_8086},
	{"SHL",			"WORD r/m16,imm8",			"po C1 /4 ib",		CT_CPU_286},
	{"SHL",			"DWORD r/m32,1",			"po D1 /4",			CT_CPU_386},
	{"SHL",			"DWORD r/m32,CL",			"po D3 /4",			CT_CPU_386},
	{"SHL",			"DWORD r/m32,imm8",			"po C1 /4 ib",		CT_CPU_386},
	{"SHR",			"BYTE r/m8,1",				"D0 /5",			CT_CPU_8086},
	{"SHR",			"BYTE r/m8,CL",				"D2 /5",			CT_CPU_8086},
	{"SHR",			"BYTE r/m8,imm8",			"C0 /5 ib",			CT_CPU_286},
	{"SHR",			"WORD r/m16,1",				"po D1 /5",			CT_CPU_8086},
	{"SHR",			"WORD r/m16,CL",			"po D3 /5",			CT_CPU_8086},
	{"SHR",			"WORD r/m16,imm8",			"po C1 /5 ib",		CT_CPU_286},
	{"SHR",			"DWORD r/m32,1",			"po D1 /5",			CT_CPU_386},
	{"SHR",			"DWORD r/m32,CL",			"po D3 /5",			CT_CPU_386},
	{"SHR",			"DWORD r/m32,imm8",			"po C1 /5 ib",		CT_CPU_386},
	{"SHLD",		"WORD r/m16,reg16,imm8",	"po 0F A4 /r ib",	CT_CPU_386},
	{"SHLD",		"DWORD r/m32,reg32,imm8",	"po 0F A4 /r ib",	CT_CPU_386},
	{"SHLD",		"WORD r/m16,reg16,CL",		"po 0F A5 /r",		CT_CPU_386},
	{"SHLD",		"DWORD r/m32,reg32,CL",		"po 0F A5 /r",		CT_CPU_386},
	{"SHRD",		"WORD r/m16,reg16,imm8",	"po 0F AC /r ib",	CT_CPU_386},
	{"SHRD",		"DWORD r/m32,reg32,imm8",	"po 0F AC /r ib",	CT_CPU_386},
	{"SHRD",		"WORD r/m16,reg16,CL",		"po 0F AD /r",		CT_CPU_386},
	{"SHRD",		"DWORD r/m32,reg32,CL",		"po 0F AD /r",		CT_CPU_386},
	{"SHUFPS",		"xmmreg,r/m128,imm8",		"0F C6 /r ib",		CT_CPU_KATMAI | CT_CPU_SSE},
//	{"SMI",			"",							"F1",				CT_CPU_386 | CT_CPU_UNDOC},
	{"SMINT",		"",							"0F 38",			CT_CPU_P6 | CT_CPU_CYRIX},
	{"SMINTOLD",	"",							"0F 7E",			CT_CPU_486 | CT_CPU_CYRIX},
//	{"SMSW",		"r/m16",					"0F 01 /4",			CT_CPU_286 | CT_CPU_PRIV},
	{"SQRTPS",		"xmmreg,r/m128",			"0F 51 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"SQRTSS",		"xmmreg,xmmreg/mem32",		"p3 0F 51 /r",		CT_CPU_KATMAI | CT_CPU_SSE},
	{"STC",			"",							"F9",				CT_CPU_8086},
	{"STD",			"",							"FD",				CT_CPU_8086},
	{"STI",			"",							"FB",				CT_CPU_8086},
	{"STMXCSR",		"mem32",					"0F AE /3",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"STOSB",		"",							"AA",				CT_CPU_8086},
	{"STOSW",		"",							"po AB",			CT_CPU_8086},
	{"STOSD",		"",							"po AB",			CT_CPU_386},
//	{"STR",			"r/m16",					"0F 00 /1",			CT_CPU_286 | CT_CPU_PRIV},
	{"SUB",			"r/m8,reg8",				"28 /r",			CT_CPU_8086},
	{"SUB",			"r/m16,reg16",				"po 29 /r",			CT_CPU_8086},
	{"SUB",			"r/m32,reg32",				"po 29 /r",			CT_CPU_386},
	{"SUB",			"reg8,r/m8",				"2A /r",			CT_CPU_8086},
	{"SUB",			"reg16,r/m16",				"po 2B /r",			CT_CPU_8086},
	{"SUB",			"reg32,r/m32",				"po 2B /r",			CT_CPU_386},
	{"SUB",			"r/m8,imm8",				"80 /5 ib",			CT_CPU_8086},
	{"SUB",			"r/m16,imm16",				"po 81 /5 iw",		CT_CPU_8086},
	{"SUB",			"r/m32,imm32",				"po 81 /5 id",		CT_CPU_386},
	{"SUB",			"r/m16,imm8",				"po 83 /5 ib",		CT_CPU_8086},
	{"SUB",			"r/m32,imm8",				"po 83 /5 ib",		CT_CPU_386},
	{"SUB",			"AL,imm8",					"2C ib",			CT_CPU_8086},
	{"SUB",			"AX,imm16",					"po 2D iw",			CT_CPU_8086},
	{"SUB",			"EAX,imm32",				"po 2D id",			CT_CPU_386},
	{"SUBPS",		"xmmreg,r/m128",			"0F 5C /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"SUBSS",		"xmmreg,xmmreg/mem32",		"p3 0F 5C /r",		CT_CPU_KATMAI | CT_CPU_SSE},
//	{"SVDC",		"mem80,segreg",				"0F 78 /r",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
//	{"SVLDT",		"mem80",					"0F 7A /0",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
//	{"SVTS",		"mem80",					"0F 7C /0",			CT_CPU_486 | CT_CPU_CYRIX | CT_CPU_SMM},
//	{"SYSCALL",		"",							"0F 05",			CT_CPU_P6 | CT_CPU_AMD},
	{"SYSENTER",	"",							"0F 34",			CT_CPU_P6},
//	{"SYSEXIT",		"",							"0F 36",			CT_CPU_P6 | CT_CPU_PRIV},
//	{"SYSRET",		"",							"0F 07",			CT_CPU_P6 | CT_CPU_AMD | CT_CPU_PRIV},
	{"TEST",		"r/m8,reg8",				"84 /r",			CT_CPU_8086},
	{"TEST",		"r/m16,reg16",				"po 85 /r",			CT_CPU_8086},
	{"TEST",		"r/m32,reg32",				"po 85 /r",			CT_CPU_386},
	{"TEST",		"r/m8,imm8",				"F6 /7 ib",			CT_CPU_8086},
	{"TEST",		"r/m16,imm16",				"po F7 /7 iw",		CT_CPU_8086},
	{"TEST",		"r/m32,imm32",				"po F7 /7 id",		CT_CPU_386},
	{"TEST",		"AL,imm8",					"A8 ib",			CT_CPU_8086},
	{"TEST",		"AX,imm16",					"po A9 iw",			CT_CPU_8086},
	{"TEST",		"EAX,imm32",				"po A9 id",			CT_CPU_386},
	{"UCOMISS",		"xmmreg,xmmreg/mem32",		"0F 2E /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"UD2",			"",							"0F 0B",			CT_CPU_286},
//	{"UMOV",		"r/m8,reg8",				"0F 10 /r",			CT_CPU_386 | CT_CPU_UNDOC},
//	{"UMOV",		"r/m16,reg16",				"po 0F 11 /r",		CT_CPU_386 | CT_CPU_UNDOC},
//	{"UMOV",		"r/m32,reg32",				"po 0F 11 /r",		CT_CPU_386 | CT_CPU_UNDOC},
//	{"UMOV",		"reg8,r/m8",				"0F 12 /r",			CT_CPU_386 | CT_CPU_UNDOC},
//	{"UMOV",		"reg16,r/m16",				"po 0F 13 /r",		CT_CPU_386 | CT_CPU_UNDOC},
//	{"UMOV",		"reg32,r/m32",				"po 0F 13 /r",		CT_CPU_386 | CT_CPU_UNDOC},
	{"UNPCKHPS",	"xmmreg,r/m128",			"0F 15 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
	{"UNPCKLPS",	"xmmreg,r/m128",			"0F 14 /r",			CT_CPU_KATMAI | CT_CPU_SSE},
//	{"VERR",		"r/m16",					"0F 00 /4",			CT_CPU_286 | CT_CPU_PRIV},
//	{"VERW",		"r/m16",					"0F 00 /5",			CT_CPU_286 | CT_CPU_PRIV},
	{"WAIT",		"",							"9B",				CT_CPU_8086},
	{"WBINVD",		"",							"0F 09",			CT_CPU_486},
	{"WRMSR",		"",							"0F 30",			CT_CPU_PENT},
//	{"WRSHR",		"",							"0F 37",			CT_CPU_P6 | CT_CPU_CYRIX | CT_CPU_SMM},
	{"XADD",		"r/m8,reg8",				"0F C0 /r",			CT_CPU_486},
	{"XADD",		"r/m16,reg16",				"po 0F C1 /r",		CT_CPU_486},
	{"XADD",		"r/m32,reg32",				"po 0F C1 /r",		CT_CPU_486},
//	{"XBTS",		"reg16,r/m16",				"po 0F A6 /r",		CT_CPU_386 | CT_CPU_UNDOC},
//	{"XBTS",		"reg32,r/m32",				"po 0F A6 /r",		CT_CPU_386 | CT_CPU_UNDOC},
	{"XCHG",		"reg8,r/m8",				"86 /r",			CT_CPU_8086},
	{"XCHG",		"reg16,r/m8",				"po 87 /r",			CT_CPU_8086},
	{"XCHG",		"reg32,r/m32",				"po 87 /r",			CT_CPU_386},
	{"XCHG",		"r/m8,reg8",				"86 /r",			CT_CPU_8086},
	{"XCHG",		"r/m16,reg16",				"po 87 /r",			CT_CPU_8086},
	{"XCHG",		"r/m32,reg32",				"po 87 /r",			CT_CPU_386},
	{"XCHG",		"AX,reg16",					"po 90 +r",			CT_CPU_8086},
	{"XCHG",		"EAX,reg32",				"po 90 +r",			CT_CPU_386},
	{"XCHG",		"reg16,AX",					"po 90 +r",			CT_CPU_8086},
	{"XCHG",		"reg32,EAX",				"po 90 +r",			CT_CPU_386},
	{"XLATB",		"",							"D7",				CT_CPU_8086},
	{"XOR",			"r/m8,reg8",				"30 /r",			CT_CPU_8086},
	{"XOR",			"r/m16,reg16",				"po 31 /r",			CT_CPU_8086},
	{"XOR",			"r/m32,reg32",				"po 31 /r",			CT_CPU_386},
	{"XOR",			"reg8,r/m8",				"32 /r",			CT_CPU_8086},
	{"XOR",			"reg16,r/m16",				"po 33 /r",			CT_CPU_8086},
	{"XOR",			"reg32,r/m32",				"po 33 /r",			CT_CPU_386},
	{"XOR",			"r/m8,imm8",				"80 /6 ib",			CT_CPU_8086},
	{"XOR",			"r/m16,imm16",				"po 81 /6 iw",		CT_CPU_8086},
	{"XOR",			"r/m32,imm32",				"po 81 /6 id",		CT_CPU_386},
	{"XOR",			"r/m16,imm8",				"po 83 /6 ib",		CT_CPU_8086},
	{"XOR",			"r/m32,imm8",				"po 83 /6 ib",		CT_CPU_386},
	{"XOR",			"AL,imm8",					"34 ib",			CT_CPU_8086},
	{"XOR",			"AX,imm16",					"po 35 iw",			CT_CPU_8086},
	{"XOR",			"EAX,imm32",				"po 35 id",			CT_CPU_386},
	{"XORPS",		"xmmreg,r/m128",			"0F 57 /r",			CT_CPU_KATMAI | CT_CPU_SSE}
};


static int cinstset_compare_syntax(const void *element1, const void *element2)
{
	return stricmp(((CInstSyntax*)element1)->mnemonic, 
		((CInstSyntax*)element2)->mnemonic);
}

static int cinstset_compare_entry(const void *mnemonic, const void *entry)
{
	return stricmp((char*)mnemonic, ((CInstructionEntry*)entry)->mnemonic);
}

int cinstset_num_instructions(void)
{
	return sizeof(cinstruction_set) / sizeof(CInstSyntax);
}

int cinstset_num_mnemonics(void)
{
	static int num = 0;
	int i;
	if (!num) {
		if (cinstset_num_instructions() != 0) {
			num = 1;
		}
		for(i = 0; i + 1 < cinstset_num_instructions(); i++) {
			if (stricmp(cinstruction_set[i].mnemonic, 
				cinstruction_set[i + 1].mnemonic) != 0) {
				num++;
			}
		}
	}

	return num;
}

static void cinstset_matrix_sort(void)
{
	static int inited = 0;
	if (inited == 0) {
		inited = 1;
		qsort(cinstruction_set, cinstset_num_instructions(), 
			sizeof(CInstSyntax), cinstset_compare_syntax);
	}
}

CInstructionSet *cinstset_create(void)
{
	CInstructionSet *self;
	int i, j;

	cinstset_matrix_sort();

	self = (CInstructionSet*)malloc(sizeof(CInstructionSet));
	assert(self);

	self->instructionMap = (CInstructionEntry*)malloc(
		sizeof(CInstructionEntry) * cinstset_num_mnemonics());

	for (i = 0, j = 0; i < cinstset_num_instructions(); ) {
		self->instructionMap[j].mnemonic = cinstruction_set[i].mnemonic;
		self->instructionMap[j].instruction = 
			cinst_create(&cinstruction_set[i++]);

		while (i < cinstset_num_instructions() && 
			stricmp(cinstruction_set[i - 1].mnemonic, 
			cinstruction_set[i].mnemonic) == 0) {
			cinst_attach_new(self->instructionMap[j].instruction, 
				&cinstruction_set[i++]);
		}

		j++;
	}
	assert(j == cinstset_num_mnemonics());
	return self;
}

void cinstset_release(CInstructionSet *self)
{
	int i;
	assert(self);
	for (i = 0; i < cinstset_num_mnemonics(); i++) {
		cinst_release(self->instructionMap[i].instruction);
	}
	free(self->instructionMap);
	self->instructionMap = NULL;
	free(self);
}


CInstruction *cinstset_query(const CInstructionSet *self, const char *name)
{
	CInstructionEntry *entry;
	assert(self && name);
	assert(self->instructionMap);

	entry = (CInstructionEntry*)bsearch(name, self->instructionMap, 
		cinstset_num_mnemonics(), sizeof(CInstructionEntry), 
		cinstset_compare_entry);

	if (entry == NULL) {
		fprintf(stderr, "Unrecognised mnemonic '%s'\n", name);
		fflush(stderr);
		return NULL;
	}

	cinst_reset_match(entry->instruction);

	return entry->instruction;
}




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

//---------------------------------------------------------------------
// TOKEN Type
//---------------------------------------------------------------------
enum CTokenType
{
	CTokenENDL		= 0,
	CTokenENDF		= 1,
	CTokenIDENT		= 2,
	CTokenKEYWORD	= 3,
	CTokenSTR		= 4,
	CTokenOPERATOR	= 5,
	CTokenINT		= 6,
	CTokenFLOAT		= 7,
	CTokenERROR		= 8,
};


//---------------------------------------------------------------------
// CTOKEN DEFINITION
//---------------------------------------------------------------------
struct CTOKEN
{
	enum CTokenType type;
	union {
		long intval; 
		double fltval;
		int keyword;
		int ch;
		int errcode;
		char *str;
	};
	long size;
	int lineno;
	int fileno;
	struct CTOKEN *next;
	struct CTOKEN *prev;
};

typedef struct CTOKEN CTOKEN;

#define ctoken_type(token) ((token)->type)
#define ctoken_int(token) ((token)->intval)
#define ctoken_str(token) ((token)->str)
#define ctoken_chr(token) ((token)->ch)
#define ctoken_len(token) ((token)->size)
#define ctoken_key(token) ((token)->keyword)


//---------------------------------------------------------------------
// BASIC INTERFACE
//---------------------------------------------------------------------
// create a new token
CTOKEN *ctoken_new(enum CTokenType type, const void *data, int size);

// delete and free memory
void ctoken_delete(CTOKEN *token);


CTOKEN *ctoken_new_endl(void);                  // create a new endl
CTOKEN *ctoken_new_endf(void);                  // create a new endf
CTOKEN *ctoken_new_ident(const char *ident);    // create a new identity
CTOKEN *ctoken_new_keyword(int keyid);          // create a new keyword
CTOKEN *ctoken_new_string(const char *string);  // create a new string
CTOKEN *ctoken_new_int(long intval);            // create a new integer
CTOKEN *ctoken_new_float(double fltval);        // create a new float
CTOKEN *ctoken_new_operator(int op);            // create a new operator
CTOKEN *ctoken_new_error(int errcode);          // create a new errcode
CTOKEN *ctoken_new_copy(const CTOKEN *token);   // create a new copy


//---------------------------------------------------------------------
// type & value operation
//---------------------------------------------------------------------
const char *ctoken_get_string(const CTOKEN *token);  // get string
long ctoken_get_int(const CTOKEN *token);            // get integer value
int ctoken_get_char(const CTOKEN *token);            // get operator char
double ctoken_get_float(const CTOKEN *token);        // get float value
int ctoken_get_keyword(const CTOKEN *token);         // get keyword

int ctoken_is_endl(const CTOKEN *token);
int ctoken_is_endf(const CTOKEN *token);
int ctoken_is_ident(const CTOKEN *token);
int ctoken_is_keyword(const CTOKEN *token);
int ctoken_is_string(const CTOKEN *token);
int ctoken_is_int(const CTOKEN *token);
int ctoken_is_float(const CTOKEN *token);
int ctoken_is_operator(const CTOKEN *token);
int ctoken_is_error(const CTOKEN *token);


//---------------------------------------------------------------------
// list operation
//---------------------------------------------------------------------
void ctoken_list_add(CTOKEN *node, CTOKEN *head);
void ctoken_list_add_tail(CTOKEN *node, CTOKEN *head);
void ctoken_list_del_between(CTOKEN *p, CTOKEN *n);
void ctoken_list_del(CTOKEN *p);
int ctoken_list_is_empty(const CTOKEN *p);


//---------------------------------------------------------------------
// misc
//---------------------------------------------------------------------
void ctoken_print(FILE *fp, const CTOKEN *token);



//---------------------------------------------------------------------
// create a new token
//---------------------------------------------------------------------
CTOKEN *ctoken_new(enum CTokenType type, const void *str, int size)
{
	CTOKEN *token;

	token = (CTOKEN*)malloc(sizeof(CTOKEN));
	assert(token);

	token->type = type;
	token->str = NULL;
	token->size = 0;
	token->lineno = -1;
	token->fileno = -1;
	token->keyword = -1;

	switch (type)
	{
	case CTokenENDL:
		break;
	case CTokenENDF:
		break;
	case CTokenSTR:
	case CTokenIDENT:
		token->str = (char*)malloc(size + 1);
		assert(token->str);
		memcpy(token->str, str, size);
		token->str[size] = 0;
		token->size = size;
		break;
	case CTokenKEYWORD:
		token->keyword = *(int*)str;
		break;
	case CTokenOPERATOR:
		token->ch = *(int*)str;
		break;
	case CTokenINT:
		token->intval = *(long*)str;
		break;
	case CTokenFLOAT:
		token->fltval = *(double*)str;
		break;
	default:
		token->type = CTokenERROR;
		token->errcode = *(int*)str;
		break;
	}

	token->next = token;
	token->prev = token;

	return token;
}

//---------------------------------------------------------------------
// release token
//---------------------------------------------------------------------
void ctoken_delete(CTOKEN *token)
{
	if (token->type == CTokenSTR || token->type == CTokenIDENT) {
		if (token->str) free(token->str);
		token->str = NULL;
	}
	token->type = CTokenERROR;
	free(token);
}

//---------------------------------------------------------------------
// create a new endl
//---------------------------------------------------------------------
CTOKEN *ctoken_new_endl(void) {
	return ctoken_new(CTokenENDL, NULL, 0);
}

//---------------------------------------------------------------------
// create a new endf
//---------------------------------------------------------------------
CTOKEN *ctoken_new_endf(void) {
	return ctoken_new(CTokenENDF, NULL, 0);
}

//---------------------------------------------------------------------
// create a new identity
//---------------------------------------------------------------------
CTOKEN *ctoken_new_ident(const char *ident) {
	return ctoken_new(CTokenIDENT, ident, (int)strlen(ident));
}

//---------------------------------------------------------------------
// create a new keyword
//---------------------------------------------------------------------
CTOKEN *ctoken_new_keyword(int keyid) {
	return ctoken_new(CTokenKEYWORD, &keyid, sizeof(int));
}

//---------------------------------------------------------------------
// create a new string
//---------------------------------------------------------------------
CTOKEN *ctoken_new_string(const char *string) {
	return ctoken_new(CTokenSTR, string, (int)strlen(string));
}

//---------------------------------------------------------------------
// create a new integer
//---------------------------------------------------------------------
CTOKEN *ctoken_new_int(long intval) {
	return ctoken_new(CTokenINT, &intval, sizeof(long));
}

//---------------------------------------------------------------------
// create a new float
//---------------------------------------------------------------------
CTOKEN *ctoken_new_float(double fltval) {
	return ctoken_new(CTokenFLOAT, &fltval, sizeof(double));
}

//---------------------------------------------------------------------
// create a new operator
//---------------------------------------------------------------------
CTOKEN *ctoken_new_operator(int op) {
	return ctoken_new(CTokenOPERATOR, &op, sizeof(int));
}

//---------------------------------------------------------------------
// create a new error
//---------------------------------------------------------------------
CTOKEN *ctoken_new_error(int code) {
	return ctoken_new(CTokenERROR, &code, sizeof(int));
}

//---------------------------------------------------------------------
// token copy
//---------------------------------------------------------------------
CTOKEN *ctoken_new_copy(const CTOKEN *token) {
	CTOKEN *newtoken;

	newtoken = (CTOKEN*)malloc(sizeof(CTOKEN));
	assert(newtoken);

	newtoken->type = token->type;
	newtoken->str = NULL;
	newtoken->size = 0;
	newtoken->lineno = token->lineno;
	newtoken->fileno = token->fileno;

	switch (newtoken->type)
	{
	case CTokenSTR:
	case CTokenIDENT:
		newtoken->str = (char*)malloc(token->size + 1);
		assert(newtoken->str);
		memcpy(newtoken->str, token->str, token->size);
		newtoken->str[token->size] = 0;
		newtoken->size = token->size;
		break;
	case CTokenKEYWORD:
		newtoken->keyword = token->keyword;
		break;
	case CTokenOPERATOR:
		newtoken->ch = token->ch;
		break;
	case CTokenINT:
		newtoken->intval = token->intval;
		break;
	case CTokenFLOAT:
		newtoken->fltval = token->fltval;
		break;
	default:
		break;
	}

	newtoken->next = newtoken;
	newtoken->prev = newtoken;

	return newtoken;
}


//---------------------------------------------------------------------
// get string
//---------------------------------------------------------------------
const char *ctoken_get_string(const CTOKEN *token)
{
	if (token->type != CTokenIDENT && token->type != CTokenSTR) {
		return "";
	}
	return token->str;
}

//---------------------------------------------------------------------
// get integer
//---------------------------------------------------------------------
long ctoken_get_int(const CTOKEN *token)
{
	if (token->type != CTokenINT) {
		return 0;
	}
	return token->intval;
}

//---------------------------------------------------------------------
// get char
//---------------------------------------------------------------------
int ctoken_get_char(const CTOKEN *token)
{
	if (token->type != CTokenOPERATOR) {
		return '\0';
	}
	return token->ch;
}

//---------------------------------------------------------------------
// get float
//---------------------------------------------------------------------
double ctoken_get_float(const CTOKEN *token)
{
	if (token->type != CTokenFLOAT) {
		return 0.0;
	}
	return token->fltval;
}

//---------------------------------------------------------------------
// get keyword
//---------------------------------------------------------------------
int ctoken_get_keyword(const CTOKEN *token)
{
	if (token->type != CTokenKEYWORD) {
		return -1;
	}
	return token->keyword;
}


int ctoken_is_endl(const CTOKEN *token) {
	return token->type == CTokenENDL;
}

int ctoken_is_endf(const CTOKEN *token) {
	return token->type == CTokenENDF;
}

int ctoken_is_ident(const CTOKEN *token) {
	return token->type == CTokenIDENT;
}

int ctoken_is_keyword(const CTOKEN *token) {
	return token->type == CTokenKEYWORD;
}

int ctoken_is_string(const CTOKEN *token) {
	return token->type == CTokenSTR;
}

int ctoken_is_int(const CTOKEN *token) {
	return token->type == CTokenINT;
}

int ctoken_is_float(const CTOKEN *token) {
	return token->type == CTokenFLOAT;
}

int ctoken_is_operator(const CTOKEN *token) {
	return token->type == CTokenOPERATOR;
}

int ctoken_is_error(const CTOKEN *token) {
	return token->type == CTokenERROR;
}


//---------------------------------------------------------------------
// add node to head
//---------------------------------------------------------------------
void ctoken_list_add(CTOKEN *node, CTOKEN *head)
{
	(node)->prev = (head), (node)->next = (head)->next;
	(head)->next->prev = (node), (head)->next = (node);
}

//---------------------------------------------------------------------
// add node to head's tail
//---------------------------------------------------------------------
void ctoken_list_add_tail(CTOKEN *node, CTOKEN *head)
{
	(node)->prev = (head)->prev, (node)->next = (head);
	(head)->prev->next = (node), (head)->prev = (node);
}

//---------------------------------------------------------------------
// delete between
//---------------------------------------------------------------------
void ctoken_list_del_between(CTOKEN *p, CTOKEN *n)
{
	(n)->prev = (p), (p)->next = (n);
}

//---------------------------------------------------------------------
// remove self
//---------------------------------------------------------------------
void ctoken_list_del(CTOKEN *entry)
{
	(entry)->next->prev = (entry)->prev;
	(entry)->prev->next = (entry)->next;
	(entry)->next = 0, (entry)->prev = 0;
	(entry)->next = entry;
	(entry)->prev = entry;
}

//---------------------------------------------------------------------
// check if empty
//---------------------------------------------------------------------
int ctoken_list_is_empty(const CTOKEN *entry)
{
	return (entry) == (entry)->next;
}

//---------------------------------------------------------------------
// print to file
//---------------------------------------------------------------------
void ctoken_print(FILE *fp, const CTOKEN *token)
{
	if (fp == NULL) fp = stdout;
	if (token->type == CTokenIDENT) {
		fprintf(fp, "(%s)", token->str);
	}
	else if (token->type == CTokenSTR) {
		fprintf(fp, "(\"%s\")", token->str);
	}
	else if (token->type == CTokenENDL) {
		fprintf(fp, "ENDL");
	}
	else if (token->type == CTokenENDF) {
		fprintf(fp, "ENDF");
	}
	else if (token->type == CTokenKEYWORD) {
		fprintf(fp, "<%d>", token->keyword);
	}
	else if (token->type == CTokenINT) {
		fprintf(fp, "[%ld]", token->intval);
	}
	else if (token->type == CTokenFLOAT) {
		fprintf(fp, "[%f]", token->fltval);
	}
	else if (token->type == CTokenOPERATOR) {
		fprintf(fp, "[%c]", (char)token->ch);
	}
	else if (token->type == CTokenERROR) {
		fprintf(fp, "ERROR");
	}
	fflush(fp);
}





#define CMAX_IDENT	8192

//---------------------------------------------------------------------
// CTokenReader (assembly)
//---------------------------------------------------------------------
struct CTokenReader
{
	int (*readch)(void *fp);
	void *fp;
	int ch;
	int unch;
	int saved;
	char *buffer;
	char *error;
	char **keywords;
	int state;
	int pos;
	int lineno;
	int eof;
	int colno;
	int errcode;
};

typedef struct CTokenReader CTokenReader;

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// Assembly Token Reader
//---------------------------------------------------------------------
CTokenReader *ctoken_reader_create(int (*getch)(void*), void *fp);

void ctoken_reader_release(CTokenReader *reader);

CTOKEN *ctoken_reader_read(CTokenReader *reader);

#ifdef __cplusplus
}
#endif


//---------------------------------------------------------------------
// CMacro
//---------------------------------------------------------------------
struct CMacro
{
	char *ident;
	char *value;
	struct CMacro *next;
};

typedef struct CMacro CMacro;

//---------------------------------------------------------------------
// CScanner
//---------------------------------------------------------------------
struct CScanner
{
	const char *source;
	long length;
	long position;
	int errcode;
	char *error;
	int jmplabel;
	int lineno;
	CTOKEN endf;
	CTOKEN *root;
	CMacro *macros;
	const CTOKEN *link;
	CTokenReader *reader;
};

typedef struct CScanner CScanner;


#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// Scanner
//---------------------------------------------------------------------
CScanner *cscanner_create(void);

void cscanner_release(CScanner *scan);

void cscanner_macro_reset(CScanner *scan);

int cscanner_macro_set(CScanner *scan, const char *name, const char *value);
int cscanner_macro_del(CScanner *scan, const char *name);

int cscanner_set_source(CScanner *scan, const char *source);

const CTOKEN *cscanner_token_current(const CScanner *scan);
const CTOKEN *cscanner_token_lookahead(const CScanner *scan);
const CTOKEN *cscanner_token_advance(CScanner *scan, int n);

int cscanner_get_type(const CScanner *scan);
const char *cscanner_get_string(const CScanner *scan);
int cscanner_get_char(const CScanner *scan);
int cscanner_get_value(const CScanner *scan);
int cscanner_get_lineno(const CScanner *scan);

int cscanner_is_endl(const CScanner *scan);
int cscanner_is_endf(const CScanner *scan);
int cscanner_is_ident(const CScanner *scan);
int cscanner_is_operator(const CScanner *scan);
int cscanner_is_int(const CScanner *scan);
int cscanner_is_string(const CScanner *scan);


#ifdef __cplusplus
}
#endif




#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif


//---------------------------------------------------------------------
// compatible
//---------------------------------------------------------------------
int cstricmp(const char *dst, const char *src) {
	int ch1, ch2;
	do {
		if ( ((ch1 = (unsigned char)(*(dst++))) >= 'A') && (ch1 <= 'Z') )
			ch1 += 0x20;
		if ( ((ch2 = (unsigned char)(*(src++))) >= 'A') && (ch2 <= 'Z') )
			ch2 += 0x20;
	}	while ( ch1 && (ch1 == ch2) );
	return(ch1 - ch2);
}


//---------------------------------------------------------------------
// Token Reader
//---------------------------------------------------------------------
CTokenReader *ctoken_reader_create(int (*readch)(void*), void *fp)
{
	CTokenReader *reader;
	reader = (CTokenReader*)malloc(sizeof(CTokenReader));
	assert(reader);
	reader->readch = readch;
	reader->fp = fp;
	reader->ch = ' ';
	reader->unch = -1;
	reader->saved = -1;
	reader->lineno = 1;
	reader->colno = 0;
	reader->state = 0;
	reader->buffer = (char*)malloc(CMAX_IDENT * 2);
	assert(reader->buffer);
	reader->error = (char*)malloc(8192);
	assert(reader->error);
	reader->pos = 0;
	reader->keywords = NULL;
	reader->eof = 0;
	reader->error[0] = 0;
	reader->errcode = 0;
	return reader;
}

void ctoken_reader_release(CTokenReader *reader)
{
	assert(reader);
	if (reader->buffer) {
		free(reader->buffer);
		reader->buffer = NULL;
	}
	if (reader->error) {
		free(reader->error);
		reader->error = NULL;
	}
	free(reader);
}

int ctoken_reader_getch(CTokenReader *reader)
{
	assert(reader);
	if (reader->unch >= 0) {
		reader->ch = reader->unch;
		reader->unch = -1;
	}	else {
		reader->saved = reader->ch;
		reader->ch = reader->readch(reader->fp);
		if (reader->ch == '\n') reader->lineno++, reader->colno = 1;
		else if (reader->ch >= 0) reader->colno++;
	}
	return reader->ch;
}

static int ctoken_reader_ungetch(CTokenReader *reader, int ch)
{
	assert(reader->unch < 0);
	reader->unch = ch;
	return 0;
}

static int ctoken_reader_skip_space(CTokenReader *reader)
{
	while (1) {
		int skip = 0;
		for (; isspace(reader->ch) && reader->ch != '\n'; skip++)
			ctoken_reader_getch(reader);
		if (reader->ch == ';' || reader->ch == '#') {
			skip++;
			while (reader->ch != '\n' && reader->ch >= 0) {
				ctoken_reader_getch(reader);
				skip++;
			}
		}	
		else if (reader->ch == '/') {
			ctoken_reader_getch(reader);
			if (reader->ch == '/') {
				skip++;
				while (reader->ch != '\n' && reader->ch >= 0) {
					ctoken_reader_getch(reader);
					skip++;
				}
			}	else {
				ctoken_reader_ungetch(reader, reader->ch);
				reader->ch = '/';
			}
		}
		if (skip == 0) break;
	}
	return 0;
}

static CTOKEN *ctoken_reader_read_string(CTokenReader *reader, int *state)
{
	CTOKEN *token = NULL;

	if (reader->ch == '\'' || reader->ch == '\"') {
		int mode = (reader->ch == '\"')? 0 : 1;
		reader->pos = 0;
		while (1) {
			int ch = ctoken_reader_getch(reader);
			if (ch == '\\') {
				ctoken_reader_getch(reader);
				reader->buffer[reader->pos++] = '\\';
				reader->buffer[reader->pos++] = (char)reader->ch;
			}	
			else if (mode == 0 && ch == '\'') {
				reader->buffer[reader->pos++] = '\'';
			}
			else if (mode == 1 && ch == '\"') {
				reader->buffer[reader->pos++] = '\"';
			}
			else if (mode == 0 && ch == '\"') {
				ch = ctoken_reader_getch(reader);
				if (ch == '\"') {
					reader->buffer[reader->pos++] = '\"';
					reader->buffer[reader->pos++] = '\"';
				}	else {
					*state = 1;
					reader->buffer[reader->pos] = 0;
					token = ctoken_new_string(reader->buffer);
					break;
				}
			}
			else if (mode == 1 && ch == '\'') {
				ch = ctoken_reader_getch(reader);
				if (ch == '\'') {
					reader->buffer[reader->pos++] = '\'';
					reader->buffer[reader->pos++] = '\'';
				}	else {
					*state = 1;
					reader->buffer[reader->pos] = 0;
					token = ctoken_new_string(reader->buffer);
					break;
				}
			}
			else if (ch == '\n') {
				*state = -1;
				break;
			}
			else if (ch >= 0) {
				reader->buffer[reader->pos++] = (char)ch;
			}
			else {			// ch < 0
				*state = -2;
				break;
			}

			if (reader->pos >= CMAX_IDENT) {
				*state = -3;
				strncpy(reader->error, "string too long", 64);
				break;
			}
		}
	}

	return token;
}

static CTOKEN *ctoken_reader_read_number(CTokenReader *reader, int *state)
{
	int lineno = reader->lineno;
	char *text = reader->buffer;
	CTOKEN *token;
	int ec1, ec2, pos;
	long value;

	if (reader->ch < '0' || reader->ch > '9') {
		*state = 0;
		return NULL;
	}

	for (reader->pos = 0; isalnum(reader->ch) || reader->ch == '.'; ) {
		reader->buffer[reader->pos++] = (char)reader->ch;
		ctoken_reader_getch(reader);
		if (reader->pos >= CMAX_IDENT) {
			strncpy(reader->error, "number too long", 64);
			*state = -1;
			reader->errcode = 1;
			return NULL;
		}
	}

	reader->buffer[reader->pos] = 0;
	for (pos = reader->pos; pos > 0; pos--) {
		if (isdigit(text[pos - 1]) || text[pos - 1] == '.') {
			break;
		}
		else if (text[pos - 1] >= 'a' && text[pos - 1] <= 'f') {
			break;
		}
		else if (text[pos - 1] >= 'A' && text[pos - 1] <= 'F') {
			break;
		}
	}

	if (reader->pos - pos > 2) {
		strncpy(reader->error, "number format error", 64);
		*state = -2;
		reader->errcode = 2;
		return NULL;
	}

	if (reader->pos - pos == 2) ec1 = text[pos], ec2 = text[pos + 1];
	else if (reader->pos - pos == 1) ec1 = text[pos], ec2 = 0;
	else ec1 = ec2 = 0;
	text[pos] = 0;
	
	// hex
	if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		value = (long)strtoul(text + 2, NULL, 16);
		token = ctoken_new_int(value);
	}	// hex
	else if (ec1 == 'h' && ec2 == 0) {
		value = (long)strtoul(text, NULL, 16);
		token = ctoken_new_int(value);
	}	// binary
	else if (ec1 == 'b' && ec2 == 0) {
		value = (long)strtoul(text, NULL, 2);
		token = ctoken_new_int(value);
	}	// octal
	else if (ec1 == 'q' && ec2 == 0) {
		value = (long)strtol(text, NULL, 8);
		token = ctoken_new_int(value);
	}	// decimal or float
	else {
		int decimal = 1;
		int i;
		for (i = 0; text[i]; i++) 
			if (text[i] == '.') decimal = 0;
		if (decimal) {
			value = (long)strtoul(text, NULL, 10);
			token = ctoken_new_int(value);
		}	else {
			float ff;
			sscanf(text, "%f", &ff);
			token = ctoken_new_float(ff);
		}
	}
	token->lineno = lineno;
	*state = 0;
	return token;
}

CTOKEN *ctoken_reader_read(CTokenReader *reader)
{
	CTOKEN *token = NULL;

	assert(reader);

	// skip memo and space
	ctoken_reader_skip_space(reader);

	// this is a endl
	if (reader->ch == '\n') {
		int lineno = reader->lineno - 1;
		token = ctoken_new_endl();
		token->lineno = lineno;
		ctoken_reader_getch(reader);
		return token;
	}

	// this is a endf
	if (reader->ch < 0) {
		if (reader->eof++) return NULL;
		token = ctoken_new_endf();
		token->lineno = reader->lineno;
		return token;
	}

	// this is a string 
	if (reader->ch == '\'' || reader->ch == '\"') {
		int lineno = reader->lineno;
		int state;
		token = ctoken_reader_read_string(reader, &state);
		if (state < 0) {
			strncpy(reader->error, "expected closing quotation mark", 100);
			reader->errcode = 3;
			return NULL;
		}
		token->lineno = lineno;
		return token;
	}

	#define issym2f(c) ((c) == '_' || isalpha(c) || (c) == '$' || (c) == '@')
	#define issym2x(c) ((c) == '_' || isalnum(c) || (c) == '$' || (c) == '@')

	// this is a identity or a keyword
	if (issym2f(reader->ch)) {
		int lineno = reader->lineno;
		for (reader->pos = 0; issym2x(reader->ch); ) {
			reader->buffer[reader->pos++] = (char)reader->ch;
			ctoken_reader_getch(reader);
			if (reader->pos >= CMAX_IDENT) {
				strncpy(reader->error, "ident too long", 100);
				reader->errcode = 4;
				return NULL;
			}
		}
		reader->buffer[reader->pos] = 0;
		if (reader->keywords) {
			int i;
			for (i = 0; reader->keywords[i]; i++) {
				if (stricmp(reader->buffer, reader->keywords[i]) == 0) {
					token = ctoken_new_keyword(i);
					token->lineno = lineno;
					return token;
				}
			}
		}
		token = ctoken_new_ident(reader->buffer);
		token->lineno = lineno;
		//printf("{%s:%d:%d}\n", token->str, token->lineno, reader->lineno);
		return token;
	}

	#undef issym2f
	#undef issym2x

	// this is a number
	if (reader->ch >= '0' && reader->ch <= '9') {
		int lineno = reader->lineno;
		int state;
		//printf("number\n");
		token = ctoken_reader_read_number(reader, &state);
		if (state < 0) {
			strncpy(reader->error, "number format error", 100);
			reader->errcode = 5;
			return NULL;
		}
		token->lineno = lineno;
		return token;
	}

	// operators
	token = ctoken_new_operator(reader->ch);
	assert(token);
	token->lineno = reader->lineno;
	ctoken_reader_getch(reader);

	return token;
}


//---------------------------------------------------------------------
// token stream
//---------------------------------------------------------------------
static int ctoken_stream_text_getch(void *fp) {
	char **ptr = (char**)fp;
	if (**ptr == 0) return -1;
	return *((*ptr)++);
}

static void ctoken_stream_free(CTOKEN *root) {
	assert(root);
	while (!ctoken_list_is_empty(root)) {
		CTOKEN *token = root->next;
		ctoken_list_del(token);
		ctoken_delete(token);
	}
	ctoken_delete(root);
}

static CTOKEN *ctoken_stream_load(const char *text, char *error)
{
	CTokenReader *reader;
	CTOKEN *root;
	char *string;
	int retval;

	string = (char*)text;
	reader = ctoken_reader_create(ctoken_stream_text_getch, &string);
	assert(reader);

	root = ctoken_new_endf();
	assert(root);

	for (retval = 0, *error = 0; ; ) {
		CTOKEN *token;
		token = ctoken_reader_read(reader);
		if (token == NULL) {
			if (error) strncpy(error, reader->error, 100);
			retval = -1;
			break;
		}
		if (token->type == CTokenENDF) {
			break;
		}
		ctoken_list_add_tail(token, root);
	}

	ctoken_reader_release(reader);

	if (retval != 0) {
		ctoken_stream_free(root);
		return NULL;
	}

	return root;
}


//---------------------------------------------------------------------
// Scanner
//---------------------------------------------------------------------
CScanner *cscanner_create(void)
{
	CScanner *scan;
	scan = (CScanner*)malloc(sizeof(CScanner));
	assert(scan);
	scan->root = NULL;
	scan->link = NULL;
	scan->source = NULL;
	scan->length = 0;
	scan->position = 0;
	scan->reader = NULL;
	scan->error = (char*)malloc(1024);
	assert(scan->error);
	scan->errcode = 0;
	scan->macros = NULL;
	scan->endf.type = CTokenENDF;
	scan->endf.lineno = 0;
	scan->endf.fileno = 0;
	scan->jmplabel = 0;
	return scan;
}

static void cscanner_token_reset(CScanner *scan)
{
	assert(scan);
	if (scan->root) {
		while (!ctoken_list_is_empty(scan->root)) {
			CTOKEN *token = scan->root->next;
			ctoken_list_del(token);
			ctoken_delete(token);
		}
		ctoken_delete(scan->root);
		scan->root = NULL;
		scan->link = NULL;
	}
	if (scan->reader) {
		ctoken_reader_release(scan->reader);
		scan->reader = NULL;
	}
	scan->source = NULL;
	scan->length = 0;
	scan->position = 0;
	scan->errcode = 0;
}

void cscanner_macro_reset(CScanner *scan)
{
	while (scan->macros) {
		CMacro *macro = scan->macros;
		scan->macros = scan->macros->next;
		free(macro->ident);
		free(macro->value);
		free(macro);
	}
	scan->jmplabel = 0;
}

void cscanner_release(CScanner *scan)
{
	cscanner_token_reset(scan);
	cscanner_macro_reset(scan);
	if (scan->error) {
		free(scan->error);
		scan->error = NULL;
	}
	free(scan);
}

int cscanner_macro_set(CScanner *scan, const char *name, const char *value)
{
	CMacro *macro;

	for (macro = scan->macros; macro; macro = macro->next) {
		if (strcmp(macro->ident, name) == 0) {
			return -1;
		}
	}

	macro = (CMacro*)malloc(sizeof(CMacro));
	assert(macro);

	macro->ident = strdup(name);
	macro->value = strdup(value);

	assert(macro->ident);
	assert(macro->value);

	macro->next = scan->macros;
	scan->macros = macro;

	return 0;
}

int cscanner_macro_del(CScanner *scan, const char *name)
{
	CMacro *macro;
	CMacro *prev;

	for (macro = scan->macros, prev = NULL; macro; ) {
		if (strcmp(macro->ident, name) == 0) {
			break;
		}
		prev = macro;
		macro = macro->next;
	}

	if (macro == NULL) {		// not find macro
		return -1;
	}

	if (prev) prev->next = macro->next;
	else scan->macros = macro->next;

	free(macro->ident);
	free(macro->value);
	free(macro);

	return 0;
}

const char *cscanner_macro_search(CScanner *scan, const char *name)
{
	CMacro *macro;
	for (macro = scan->macros; macro; macro = macro->next) {
		if (strcmp(macro->ident, name) == 0) {
			return macro->value;
		}
	}

	return NULL;
}

static int cscanner_reader_getch(void *fp) 
{
	CScanner *scan = (CScanner*)fp;
	if (scan->source == NULL) return -1;
	if (scan->position >= scan->length) return -1;
	return scan->source[scan->position++];
}

int cscanner_set_source(CScanner *scan, const char *source)
{
	int retval = 0;

	cscanner_token_reset(scan);
	scan->source = source;
	scan->length = (int)strlen(source);
	scan->position = 0;
	scan->reader = ctoken_reader_create(cscanner_reader_getch, scan);
	
	scan->root = ctoken_new_endf();
	scan->link = scan->root;
	scan->error[0] = 0;
	scan->errcode = 0;

	for (; ; ) {
		CTOKEN *token;

		token = ctoken_reader_read(scan->reader);

		if (token == NULL) {
			scan->lineno = scan->reader->lineno;
			scan->errcode = scan->reader->errcode;
			strncpy(scan->error, scan->reader->error, 80);
			retval = -1;
			break;
		}

		if (token->type == CTokenIDENT) {
			const char *macro = cscanner_macro_search(scan, token->str);
			if (macro != NULL) {
				CTOKEN *ts = ctoken_stream_load(macro, scan->error);
				if (ts == NULL) {
					scan->lineno = scan->reader->lineno;
					scan->errcode = 88;
					retval = -2;
					break;
				}
				while (!ctoken_list_is_empty(ts)) {
					CTOKEN *next = ts->next;
					if (next->type == CTokenENDF) break;
					ctoken_list_del(next);
					next->lineno = scan->lineno;
					ctoken_list_add_tail(next, scan->root);
				}
				ctoken_stream_free(ts);
				continue;
			}	
			else {
				if (strcmp(token->str, "@@") == 0) {
					scan->jmplabel++;
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel);
					//printf("label %d\n", scan->jmplabel);
				}
				else if (stricmp(token->str, "@b") == 0) {
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel);
				}
				else if (stricmp(token->str, "@f") == 0) {
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel + 1);
				}
			}
		}

		ctoken_list_add_tail(token, scan->root);
		if (token->type == CTokenENDF) {
			break;
		}
	}

	if (retval != 0) {
		cscanner_token_reset(scan);
		return retval;
	}

	scan->link = scan->root->next;
	scan->root->lineno = scan->root->prev->lineno;

	return 0;
}

const CTOKEN *cscanner_token_current(const CScanner *scan)
{
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	return scan->link;
}

const CTOKEN *cscanner_token_lookahead(const CScanner *scan)
{
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	if (scan->link == scan->root) {
		return &(scan->endf);
	}
	return scan->link->next;
}

const CTOKEN *cscanner_token_advance(CScanner *scan, int n)
{
	if (n < 0) n = 0;
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	while (n--) {
		if (scan->link == scan->root) {
			return &(scan->endf);
		}
		scan->link = scan->link->next;
	}
	return scan->link;
}

int cscanner_get_type(const CScanner *scan)
{
	return cscanner_token_current(scan)->type;
}

const char *cscanner_get_string(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenIDENT && token->type != CTokenSTR) {
		return "";
	}
	return token->str;
}

int cscanner_get_char(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenOPERATOR) {
		return '\0';
	}
	return token->ch;
}

int cscanner_get_value(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenINT) {
		return 0;
	}
	return token->intval;
}

int cscanner_get_lineno(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	return token->lineno;
}

int cscanner_is_endl(const CScanner *scan) {
	const CTOKEN *token = cscanner_token_current(scan);
	return (token->type == CTokenENDF || token->type == CTokenENDL);
}

int cscanner_is_endf(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenENDF;
}

int cscanner_is_ident(const CScanner *scan)  {
	return cscanner_get_type(scan) == CTokenIDENT;
}

int cscanner_is_operator(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenOPERATOR;
}

int cscanner_is_int(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenINT;
}

int cscanner_is_string(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenSTR;
}




//---------------------------------------------------------------------
// CSynthesizer
//---------------------------------------------------------------------
struct CSynthesizer
{
	CEncoding encoding;
	enum COperandType firstType;
	enum COperandType secondType;
	enum CRegID firstReg;
	enum CRegID secondReg;
	enum CRegID baseReg;
	enum CRegID indexReg;
	int scale;
	int prefix;
	char *error;
	int errcode;
};

typedef struct CSynthesizer CSynthesizer;



#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------
void csynth_init(CSynthesizer *synth);
void csynth_destroy(CSynthesizer *synth);
void csynth_reset(CSynthesizer *synth);

int csynth_define_label(CSynthesizer *synth, const char *label);
int csynth_reference_label(CSynthesizer *synth, const char *label);

int csynth_encode_first_operand(CSynthesizer *synth, const COperand *);
int csynth_encode_second_operand(CSynthesizer *synth, const COperand *);
int csynth_encode_third_operand(CSynthesizer *synth, const COperand *);

int csynth_encode_base(CSynthesizer *synth, const COperand *base);
int csynth_encode_index(CSynthesizer *synth, const COperand *index);

int csynth_encode_scale(CSynthesizer *synth, int scale);
int csynth_encode_immediate(CSynthesizer *synth, long immediate);
int csynth_encode_displacement(CSynthesizer *synth, long displacement);

int csynth_encode_prefix(CSynthesizer *synth, int code);

const CEncoding *csynth_encode_instruction(CSynthesizer *, CInstruction*);


#ifdef __cplusplus
}
#endif



#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

void csynth_init(CSynthesizer *synth)
{
	cencoding_init(&synth->encoding);
	synth->error = (char*)malloc(1024);
	assert(synth->error);
	csynth_reset(synth);
}

void csynth_destroy(CSynthesizer *synth)
{
	cencoding_destroy(&synth->encoding);
	if (synth->error) free(synth->error);
	synth->error = NULL;
	synth->errcode = 0;
}

void csynth_reset(CSynthesizer *synth)
{
	cencoding_reset(&synth->encoding);
	synth->firstType = O_UNKNOWN;
	synth->secondType = O_UNKNOWN;
	synth->firstReg = REG_UNKNOWN;
	synth->secondReg = REG_UNKNOWN;
	synth->baseReg = REG_UNKNOWN;
	synth->indexReg = REG_UNKNOWN;
	synth->scale = 0;
	synth->prefix = 0;
	synth->error[0] = 0;
	synth->errcode = 0;
}

static void csynth_error(CSynthesizer *synth, const char *error, int code)
{
	strncpy(synth->error, error, 100);
	synth->errcode = code;
}

int csynth_define_label(CSynthesizer *synth, const char *label)
{
	if (synth->encoding.label != NULL) {
		csynth_error(synth,  "Instruction can't have multiple label", 1);
		return -1;
	}
	cencoding_set_label(&synth->encoding, label);
	return 0;
}

int csynth_reference_label(CSynthesizer *synth, const char *label)
{
	if (synth->encoding.reference != NULL) {
		csynth_error(synth,  "Instruction can't have multiple refrence", 2);
		return -1;
	}
	cencoding_set_reference(&synth->encoding, label);
	return 0;
}

int csynth_encode_first_operand(CSynthesizer *synth, 
	const COperand *firstOperand)
{
	if (synth->firstType != O_UNKNOWN) {
		csynth_error(synth,  "Instrucition destination already set", 3);
		return -1;
	}

	synth->firstType = firstOperand->type;

	if (coperand_is_reg(firstOperand) || coperand_is_mem(firstOperand)) {
		synth->firstReg = firstOperand->reg;
	}
	else if (coperand_is_imm(firstOperand)) {
		csynth_encode_immediate(synth, firstOperand->value);
	}
	else if (!coperand_is_void(firstOperand)) {
		csynth_error(synth, "csynth_encode_first_operand: error", 4);
		return -2;
	}

	return 0;
}

int csynth_encode_second_operand(CSynthesizer *synth, 
	const COperand *secondOperand)
{
	if (synth->secondType != O_UNKNOWN) {
		csynth_error(synth,  "Instrucition source already set", 4);
		return -1;
	}

	synth->secondType = secondOperand->type;

	if (coperand_is_reg(secondOperand) || coperand_is_mem(secondOperand)) {
		synth->secondReg = secondOperand->reg;
	}
	else if (coperand_is_imm(secondOperand)) {
		csynth_encode_immediate(synth, secondOperand->value);
	}
	else if (!coperand_is_void(secondOperand)) {
		csynth_error(synth, "csynth_encode_second_operand: error", 5);
		return -2;
	}

	return 0;
}

int csynth_encode_third_operand(CSynthesizer *synth, 
	const COperand *thirdOperand)
{
	if (coperand_is_imm(thirdOperand)) {
		csynth_encode_immediate(synth, thirdOperand->value);
	}
	else if (!coperand_is_void(thirdOperand)) {
		csynth_error(synth, "csynth_encode_third_operand: error", 6);
		return -3;
	}
	return 0;
}

int csynth_encode_base(CSynthesizer *synth, const COperand *base)
{
	if (synth->baseReg != REG_UNKNOWN) {
		int retval;
		// base already set, use as index with scale = 1
		retval = csynth_encode_index(synth, base);
		if (retval != 0) return -1;
		retval = csynth_encode_scale(synth, 1);
		if (retval != 0) return -2;
		return 0;
	}

	synth->baseReg = base->reg;
	return 0;
}

int csynth_encode_index(CSynthesizer *synth, const COperand *index)
{
	if (synth->indexReg != REG_UNKNOWN) {
		csynth_error(synth, 
			"Memory reference can't have multiple index registers", 7);
		return -1;
	}
	synth->indexReg = index->reg;
	return 0;
}

int csynth_encode_scale(CSynthesizer *synth, int scale)
{
	if (synth->scale != 0) {
		csynth_error(synth, 
			"Memory reference can't have multiple scale factors", 8);
		return -1;
	}
	if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
		csynth_error(synth, "Invalid scale value", 9);
		return -2;
	}
	synth->scale = scale;
	return 0;
}

int csynth_encode_immediate(CSynthesizer *synth, long immediate)
{
	if (synth->encoding.immediate != 0) {
		csynth_error(synth, 
			"Instruction can't have multiple immediate operands", 10);
		return -1;
	}
	synth->encoding.immediate = (cint32)immediate;
	return 0;
}

int csynth_encode_displacement(CSynthesizer *synth, long displacement)
{
	synth->encoding.displacement += (cint32)displacement;
	return 0;
}

static int csynth_encode_mod_field(CSynthesizer *synth)
{
	synth->encoding.format.modRM = 1;
	if (coperand_type_is_reg(synth->firstType) && (
		coperand_type_is_reg(synth->secondType) ||
		coperand_type_is_imm(synth->secondType) ||
		coperand_type_is_void(synth->secondType))) {
		synth->encoding.modRM.mod = MOD_REG;
	}
	else if ((coperand_type_is_mem(synth->firstType) ||
			  coperand_type_is_mem(synth->secondType)) &&
			 (coperand_type_is_reg(synth->firstType) ||
			  coperand_type_is_reg(synth->secondType))) {
		if (!synth->encoding.displacement) {
			synth->encoding.modRM.mod = MOD_NO_DISP;
		}
		else if ((char)synth->encoding.displacement == 
				synth->encoding.displacement) {
			synth->encoding.modRM.mod = MOD_BYTE_DISP;
			synth->encoding.format.D1 = 1;
		}
		else {
			synth->encoding.modRM.mod = MOD_DWORD_DISP;
			synth->encoding.format.D1 = 1;
			synth->encoding.format.D2 = 1;
			synth->encoding.format.D3 = 1;
			synth->encoding.format.D4 = 1;
		}
	}
	else {
		csynth_error(synth, "mod field error", 30);
		return -1;
	}
	return 0;
}

static int csynth_encode_sib_byte(CSynthesizer *synth)
{
	if (synth->scale == 0 && synth->indexReg == REG_UNKNOWN) {
		if (synth->baseReg == REG_UNKNOWN || (
			synth->encoding.modRM.r_m != E_ESP && 
			synth->encoding.modRM.r_m != E_EBP)) {
			if (synth->encoding.format.SIB) {
				csynth_error(synth, "SIB byte error", 31);
				return -1;
			}
			// No SIB byte needed
			return 0;
		}
	}

	// Indicates use of SIB in mod R/M
	synth->encoding.format.SIB = 1;
	synth->encoding.modRM.r_m = E_ESP;

	if (synth->baseReg == E_EBP && synth->encoding.modRM.mod == MOD_NO_DISP)
	{
		synth->encoding.modRM.mod = MOD_BYTE_DISP;
		synth->encoding.format.D1 = 1;
	}

	if (synth->indexReg == E_ESP) {
		if (synth->scale != 1) {
			csynth_error(synth, 
				"ESP can't be scaled index in memory reference", 32);
			return -2;
		}
		else {
			enum CRegID tempReg;
			tempReg = synth->indexReg;
			synth->indexReg = synth->baseReg;
			synth->baseReg = tempReg;
		}
	}

	if (synth->baseReg == REG_UNKNOWN) {
		synth->encoding.SIB.base = E_EBP;
		synth->encoding.modRM.mod = MOD_NO_DISP;
		synth->encoding.format.D1 = 1;
		synth->encoding.format.D2 = 1;
		synth->encoding.format.D3 = 1;
		synth->encoding.format.D4 = 1;
	}	else {
		synth->encoding.SIB.base = synth->baseReg;
	}
	
	if (synth->indexReg != REG_UNKNOWN) {
		synth->encoding.SIB.index = synth->indexReg;
	}	else {
		synth->encoding.SIB.index = E_ESP;
	}

	switch (synth->scale)
	{
	case 0:
	case 1:
		synth->encoding.SIB.scale = SCALE_1;
		break;
	case 2:
		synth->encoding.SIB.scale = SCALE_2;
		break;
	case 4:
		synth->encoding.SIB.scale = SCALE_4;
		break;
	case 8:
		synth->encoding.SIB.scale = SCALE_8;
		break;
	default:
		csynth_error(synth, "scale number error", 33);
		return -3;
		break;
	}

	return 0;
}

int csynth_encode_prefix(CSynthesizer *synth, int code)
{
	if (cencoding_add_prefix(&synth->encoding, code)) {
		csynth_error(synth, "cannot add prefix", 90);
		return -1;
	}
	return 0;
}

const CEncoding *csynth_encode_instruction(CSynthesizer *synth, 
	CInstruction *instruction)
{
	enum COperandType p1, p2;
	const char *format;
	unsigned long O;

	if (!instruction) {
		return &synth->encoding;
	}

	format = cinst_getEncoding(instruction);

	if (!format) {
		csynth_error(synth, "csynth_encode_instruction: internal error", 11);
		return NULL;
	}

	#define IFORMAT_WORD(x, y) ( (((short)(x)) << 8) | ((short)(y)) )

	while (*format) {
		int head = ((short)format[1] | format[0] << 8);
		switch (head)
		{
		case IFORMAT_WORD('p', '0'):
			if (cencoding_add_prefix(&synth->encoding, 0xf0)) {
				csynth_error(synth, "prefix error 0xf0", 40);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', '2'):
			if (cencoding_add_prefix(&synth->encoding, 0xf2)) {
				csynth_error(synth, "prefix error 0xf2", 41);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', '3'):
			if (cencoding_add_prefix(&synth->encoding, 0xf3)) {
				csynth_error(synth, "prefix error 0xf3", 42);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', 'o'):
			if (!cinst_is_32bit(instruction)) {
				if (cencoding_add_prefix(&synth->encoding, 0x66)) {
					csynth_error(synth, "prefix error 0x66", 43);
					return NULL;
				}
			}
			break;
		case IFORMAT_WORD('p', 'a'):
			if (!cinst_is_32bit(instruction)) {
				if (cencoding_add_prefix(&synth->encoding, 0x67)) {
					csynth_error(synth, "prefix error 0x67", 44);
					return NULL;
				}
			}
			break;
		case IFORMAT_WORD('+', 'r'):
			if (synth->encoding.format.O1) {
				if (coperand_type_is_reg(synth->firstType)) {
					synth->encoding.O1 += synth->firstReg;
				}
				else if(coperand_type_is_reg(synth->secondType)) {
					synth->encoding.O1 += synth->secondReg;
				}
				else {
					csynth_error(synth, 
						"'+r' not compatible with operands", 12);
					return NULL;
				}
			}
			else {
				csynth_error(synth, "'+r' needs first opcode byte", 13);
				return NULL;
			}
			break;
		case IFORMAT_WORD('/', 'r'):
			if (csynth_encode_mod_field(synth) != 0) {
				return NULL;
			}
			p1 = cinst_getFirstOperand(instruction);
			p2 = cinst_getSecondOperand(instruction);
			if (coperand_type_is_reg(p1) && coperand_type_is_R_M(p2)) {
				if (coperand_type_is_mem(synth->secondType)) {
					synth->encoding.modRM.r_m = synth->baseReg;
				}
				else if (coperand_type_is_reg(synth->secondType)) {
					synth->encoding.modRM.r_m = synth->secondReg;
				}
				else {
					csynth_error(synth, "syntax error", 14);
					return NULL;
				}
				synth->encoding.modRM.reg = synth->firstReg;
			}
			else if (coperand_type_is_R_M(p1) && coperand_type_is_reg(p2)) {
				if (coperand_type_is_mem(synth->firstType)) {
					synth->encoding.modRM.r_m = synth->baseReg;
				}
				else if (coperand_type_is_reg(synth->firstType)) {
					synth->encoding.modRM.r_m = synth->firstReg;
				}
				else {
					csynth_error(synth, "syntax error", 15);
					return NULL;
				}
				synth->encoding.modRM.reg = synth->secondReg;
			}
			else {
				csynth_error(synth, "format error", 16);
				return NULL;
			}
			if (csynth_encode_sib_byte(synth) != 0) {
				return NULL;
			}
			break;
		case IFORMAT_WORD('/', '0'):
		case IFORMAT_WORD('/', '1'):
		case IFORMAT_WORD('/', '2'):
		case IFORMAT_WORD('/', '3'):
		case IFORMAT_WORD('/', '4'):
		case IFORMAT_WORD('/', '5'):
		case IFORMAT_WORD('/', '6'):
		case IFORMAT_WORD('/', '7'):
			if (csynth_encode_mod_field(synth) != 0) {
				return NULL;
			}
			synth->encoding.modRM.reg = format[1] - '0';
			if (coperand_type_is_mem(synth->firstType)) {
				synth->encoding.modRM.r_m = synth->baseReg;
			}
			else if (coperand_type_is_reg(synth->firstType)) {
				synth->encoding.modRM.r_m = synth->firstReg;
			}
			else {
				csynth_error(synth, "syntax error", 17);
				return NULL;
			}
			if (csynth_encode_sib_byte(synth) != 0) {
				return NULL;
			}
			break;
		case IFORMAT_WORD('i', 'd'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.format.I3 = 1;
			synth->encoding.format.I4 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('i', 'w'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('i', 'b'):
			synth->encoding.format.I1 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('-', 'b'):
			synth->encoding.format.I1 = 1;
			synth->encoding.relative = 1;
			break;
		case IFORMAT_WORD('-', 'i'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.format.I3 = 1;
			synth->encoding.format.I4 = 1;
			synth->encoding.relative = 1;
			break;
		default:
			O = strtoul(format, 0, 16);
			if (O > 0xFF) {
				csynth_error(synth, "format error", 18);
				return NULL;
			}
			if (!synth->encoding.format.O1) {
				synth->encoding.O1 = (cbyte)O;
				synth->encoding.format.O1 = 1;
			}
			else if (synth->encoding.format.O2 == 0 &&
					(synth->encoding.O1 == 0x0f ||
					 synth->encoding.O1 == 0xd8 ||
					 synth->encoding.O1 == 0xd9 ||
					 synth->encoding.O1 == 0xda ||
					 synth->encoding.O1 == 0xdb ||
					 synth->encoding.O1 == 0xdc ||
					 synth->encoding.O1 == 0xde ||
					 synth->encoding.O1 == 0xdf)) {
				synth->encoding.O2 = synth->encoding.O1;
				synth->encoding.O1 = (cbyte)O;
				synth->encoding.format.O2 = 1;
			}
			else {
				csynth_error(synth, "synth error", 19);
				return NULL;
			}
			break;
		}

		format += 2;
		if (*format == ' ') {
			format++;
		}
		else if (*format == '\0') {
			break;
		}
		else {
			csynth_error(synth, "instruction error", 20);
			return NULL;
		}
		#undef IFORMAT_WORD
	}

	return &synth->encoding;
}



//---------------------------------------------------------------------
// CVariable
//---------------------------------------------------------------------
struct CVariable
{
	char *name;
	int pos;
	struct CVariable *next;
};

typedef struct CVariable CVariable;


//---------------------------------------------------------------------
// CParser
//---------------------------------------------------------------------
struct CParser
{
	char *data;
	char *error;
	int errcode;
	int inproc;
	int stack;
	CScanner *token;
	CVariable *vars;
	CInstruction *instruction;
	CInstructionSet *instructionset;
	CSynthesizer synthesizer;
};

typedef struct CParser CParser;

//---------------------------------------------------------------------
// interfaces
//---------------------------------------------------------------------
CParser *cparser_create(void);
void cparser_release(CParser *parser);

void cparser_reset(CParser *parser);

const CEncoding *cparser_parse_line(CParser *parser, const char *source);


#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#define IMAX_DATA 65536

CParser *cparser_create(void)
{
	CParser *parser;
	parser = (CParser*)malloc(sizeof(CParser));
	assert(parser);
	parser->token = cscanner_create();
	assert(parser->token);
	parser->instruction = NULL;
	parser->instructionset = cinstset_create();
	csynth_init(&parser->synthesizer);
	parser->data = (char*)malloc(IMAX_DATA);
	assert(parser->data);
	parser->error = (char*)malloc(1024);
	assert(parser->error);
	parser->error[0] = 0;
	parser->errcode = 0;
	parser->vars = NULL;
	parser->inproc = 0;
	parser->stack = 0;
	return parser;
}

void cparser_release(CParser *parser)
{
	assert(parser);
	if (parser->token) {
		cscanner_release(parser->token);
		parser->token = NULL;
	}
	if (parser->instructionset) {
		cinstset_release(parser->instructionset);
		parser->instructionset = NULL;
	}
	if (parser->error) {
		free(parser->error);
		parser->error = NULL;
	}
	if (parser->data) {
		free(parser->data);
		parser->data = NULL;
	}
	while (parser->vars) {
		CVariable *var = parser->vars;
		parser->vars = parser->vars->next;
		free(var->name);
		free(var);
	}
	csynth_destroy(&parser->synthesizer);
	free(parser);
}

void cparser_reset(CParser *parser)
{
	cscanner_macro_reset(parser->token);
	while (parser->vars) {
		CVariable *var = parser->vars;
		parser->vars = parser->vars->next;
		free(var->name);
		free(var);
	}
	parser->inproc = 0;
	parser->stack = 0;
}

static int cparser_parse_label(CParser *parser);
static int cparser_parse_mnemonic(CParser *parser);
static int cparser_parse_specifier(CParser *parser);
static int cparser_parse_first_operand(CParser *parser);
static int cparser_parse_second_operand(CParser *parser);
static int cparser_parse_third_operand(CParser *parser);
static COperand cparser_parse_immediate(CParser *parser);
static COperand cparser_parse_register(CParser *parser);
static COperand cparser_parse_memory_reference(CParser *parser);

static int cparser_parse_data(CParser *parser);
static int cparser_parse_align(CParser *parser);
static int cparser_parse_prefix(CParser *parser);

static int cparser_parse_proc(CParser *parser);

static void cparser_error(CParser *parser, const char *error, int code)
{
	strncpy(parser->error, error, 100);
	parser->errcode = code;
}

const CEncoding *cparser_parse_line(CParser *parser, const char *source)
{
	int retval;

	if (source == NULL) {
		cparser_error(parser, "empty source line", 1);
		return NULL;
	}

	retval = cscanner_set_source(parser->token, source);

	if (retval != 0) {
		cparser_error(parser, parser->token->error, 2);
		return NULL;
	}

	parser->instruction = NULL;
	csynth_reset(&parser->synthesizer);

	parser->error[0] = 0;
	parser->errcode = 0;

	// parse label
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_label(parser)) {
			cparser_error(parser, "label error", 3);
			return NULL;
		}
	}

	// parse inline data
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_data(parser)) {
			return NULL;
		}
		if (parser->synthesizer.encoding.data != NULL) {
			return &parser->synthesizer.encoding;
		}
	}

	// parse align
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_align(parser)) {
			return NULL;
		}
		if (parser->synthesizer.encoding.align != 0) {
			return &parser->synthesizer.encoding;
		}
	}

	// parse proc
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_proc(parser)) {
			return NULL;
		}
	}

	// parse repnz, repz
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_prefix(parser)) {
			return NULL;
		}
	}

	// parse mnemonic
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_mnemonic(parser)) {
			cparser_error(parser, "mnemonic syntax error", 4);
			return NULL;
		}

		if (!parser->instruction) {
			cparser_error(parser, "mnemonic error", 5);
			return NULL;
		}

		if (cparser_parse_first_operand(parser)) {
			if (parser->errcode == 0) 
				cparser_error(parser, "first operand error", 6);
			return NULL;
		}

		if (cparser_parse_second_operand(parser)) {
			if (parser->errcode == 0)
				cparser_error(parser, "second operand error", 7);
			return NULL;
		}

		if (cparser_parse_third_operand(parser)) {
			if (parser->errcode == 0)
				cparser_error(parser, "third operand error", 8);
			return NULL;
		}
	}

	if (parser->instruction) {
		do {
			if (cinst_match_syntax(parser->instruction)) {
				break;
			}
			parser->instruction = parser->instruction->next;
		}	while (parser->instruction);

		if (parser->instruction == NULL) {
			cparser_error(parser, "operands mismatch", 9);
			return NULL;
		}
#if 0
		printf("%s (%s) (%s) specifier=%d\n",
			parser->instruction->syntax->mnemonic,
			parser->instruction->syntax->operands,
			parser->instruction->syntax->encoding,
			parser->instruction->specifier);
#endif
	}

	return csynth_encode_instruction(&parser->synthesizer, 
		parser->instruction);
}


static int cparser_parse_label(CParser *parser)
{
	const CTOKEN *current = cscanner_token_current(parser->token);
	const CTOKEN *next = cscanner_token_lookahead(parser->token);
	if (ctoken_is_ident(current) && ctoken_get_char(next) == ':') {
		csynth_define_label(&parser->synthesizer, current->str);
		cscanner_token_advance(parser->token, 2);
	}
	else if (ctoken_get_char(current) == '.' && ctoken_is_ident(next)) {
		csynth_define_label(&parser->synthesizer, next->str);
		cscanner_token_advance(parser->token, 2);
	}
	return 0;
}

static int cparser_parse_mnemonic(CParser *parser)
{
	const char *name = cscanner_get_string(parser->token);
	parser->instruction = cinstset_query(parser->instructionset, name);
	if (parser->instruction) {
		cinst_match_mnemonic(parser->instruction, name);
		cscanner_token_advance(parser->token, 1);
	}	else {
		cparser_error(parser, "Mnemonic not recognised", 10);
		return -1;
	}
	return 0;
}

static int cparser_parse_specifier(CParser *parser)
{
	enum CSpecifierType type = CS_UNKNOWN;

	if (cscanner_is_ident(parser->token)) {
		type = cspecifier_scan(cscanner_get_string(parser->token));
	}

	cinst_match_specifier(parser->instruction, type);

	if (type != CS_UNKNOWN) {
		cscanner_token_advance(parser->token, 1);
	}

	return 0;
}

static int cparser_parse_first_operand(CParser *parser)
{
	COperand firstOperand = CINIT;

	assert(parser->instruction);

	cparser_parse_specifier(parser);

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '[':
			firstOperand = cparser_parse_memory_reference(parser);
			if (parser->errcode) return -5;
			break;
		case '+':
		case '-':
		case '~':
			firstOperand = cparser_parse_immediate(parser);
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		firstOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else if (cscanner_is_ident(parser->token)) {
		firstOperand = cparser_parse_register(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Invalid destination operand", 11);
		return -2;
	}

	cinst_match_first_operand(parser->instruction, &firstOperand);
	csynth_encode_first_operand(&parser->synthesizer, &firstOperand);

	return 0;
}

static int cparser_parse_second_operand(CParser *parser)
{
	COperand secondOperand = CINIT;
	assert(parser->instruction);

	if (cscanner_get_char(parser->token) == ',') {
		cscanner_token_advance(parser->token, 1);
	}	
	else if (!cscanner_is_endl(parser->token)) {
		cparser_error(parser, "Operands must be separated by comma", 12);
		return -3;
	}
	else {
		cinst_match_second_operand(parser->instruction, &secondOperand);
		return 0;
	}

	cparser_parse_specifier(parser);

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '[':
			secondOperand = cparser_parse_memory_reference(parser);
			if (parser->errcode) return -5;
			break;
		case '+':
		case '-':
		case '~':
			secondOperand = cparser_parse_immediate(parser);
			if (parser->errcode) return -5;
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		secondOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else if (cscanner_is_ident(parser->token)) {
		secondOperand = cparser_parse_register(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Invalid source operand", 13);
		return -2;
	}

	cinst_match_second_operand(parser->instruction, &secondOperand);
	csynth_encode_second_operand(&parser->synthesizer, &secondOperand);

	return 0;
}

static int cparser_parse_third_operand(CParser *parser)
{
	COperand thirdOperand = CINIT;

	assert(parser->instruction);

	if (cscanner_get_char(parser->token) == ',') {
		cscanner_token_advance(parser->token, 1);
	}
	else if (!cscanner_is_endl(parser->token)) {
		cparser_error(parser, "Operands must be separated by comma", 14);
		return -3;
	}
	else {
		cinst_match_third_operand(parser->instruction, &thirdOperand);
		return 0;
	}

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '+':
		case '-':
		case '~':
			thirdOperand = cparser_parse_immediate(parser);
			if (parser->errcode) return -5;
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		thirdOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Too many operands", 15);
		return -2;
	}

	cinst_match_third_operand(parser->instruction, &thirdOperand);
	csynth_encode_third_operand(&parser->synthesizer, &thirdOperand);

	return 0;
}

static COperand cparser_parse_immediate(CParser *parser)
{
	COperand imm = CINIT;
	if (cscanner_is_operator(parser->token)) {
		int ch = cscanner_get_char(parser->token);
		if (ch == '+') {
			cscanner_token_advance(parser->token, 1);
			imm.value = +cscanner_get_value(parser->token);
		}
		else if (ch == '-') {
			cscanner_token_advance(parser->token, 1);
			imm.value = -cscanner_get_value(parser->token);
		}
		else if (ch == '~') {
			cscanner_token_advance(parser->token, 1);
			imm.value = ~cscanner_get_value(parser->token);
		}
		else {
			cparser_error(parser, "error operator", 16);
			return imm;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		imm.value = cscanner_get_value(parser->token);
	}
	else {
		cparser_error(parser, "immediate error", 17);
		return imm;
	}

	if ((unsigned char)imm.value == imm.value) {
		imm.type = O_IMM8;
	}
	else if ((unsigned short)imm.value == imm.value) {
		imm.type = O_IMM16;
	}
	else {
		imm.type = O_IMM32;
	}

	cscanner_token_advance(parser->token, 1);

	return imm;
}

static COperand cparser_parse_register(CParser *parser)
{
	COperand reg = CINIT;
	const char *name;

	name = cscanner_get_string(parser->token);
	reg = coperand_scan_reg(name);

	// It's not a register, so it must be a reference
	if (reg.type == O_UNKNOWN) {
		csynth_reference_label(&parser->synthesizer, name);
		// first operand should be immediate
		reg.type = O_IMM8;	// also matchs IMM32
		cinst_match_first_operand(parser->instruction, &reg);
	}

	cscanner_token_advance(parser->token, 1);

	return reg;
}

static COperand cparser_parse_memory_reference(CParser *parser)
{
	COperand mem = CINIT;

	for (; ; ) {
		const CTOKEN *next;
		const CTOKEN *prev;
		const CTOKEN *token;
		int type;

		type = cscanner_token_lookahead(parser->token)->type;
		if (type == CTokenENDL || type == CTokenENDF) break;

		prev = cscanner_token_current(parser->token);
		cscanner_token_advance(parser->token, 1);
		next = cscanner_token_lookahead(parser->token);
		token = cscanner_token_current(parser->token);

		if (token->type == CTokenIDENT) {
			COperand reg = coperand_scan_reg(token->str);
			if (reg.type == O_UNKNOWN) {
				cparser_error(parser, "unknow reg reference", 18);
				return reg;
			}
			if (ctoken_get_char(prev) == '*' || ctoken_get_char(next) == '*')
			{
				csynth_encode_index(&parser->synthesizer, &reg);
			}
			else 
			{
				csynth_encode_base(&parser->synthesizer, &reg);
			}
		}
		else if (token->type == CTokenOPERATOR) {
			switch (ctoken_get_char(token)) {
			case ']':
				mem.type = O_MEM;
				cscanner_token_advance(parser->token, 1);
				return mem;
				break;
			case '+':
				if ((prev->type != CTokenINT && prev->type != CTokenIDENT) ||
					(next->type != CTokenINT && next->type != CTokenIDENT)) {
					cparser_error(parser, 
						"Syntax error '+' in memory reference", 19);
					return mem;
				}
				break;
			case '-':
				if ((prev->type != CTokenINT && prev->type != CTokenIDENT &&
					ctoken_get_char(prev) != '[') ||
					next->type != CTokenINT) {
					cparser_error(parser, 
						"Syntax error '-' in memory reference", 20);
					return mem;
				}
				break;
			case '*':
				if ((prev->type != CTokenINT || next->type != CTokenIDENT) &&
					(next->type != CTokenINT || prev->type != CTokenIDENT)) {
					cparser_error(parser,
						"Syntax error '*' in memory reference", 21);
					return mem;
				}
				break;
			default:
				cparser_error(parser, 
					"Unexpected punctuator in memory reference", 22);
				return mem;
				break;
			}
		}
		else if (token->type == CTokenINT) {
			int prevch = ctoken_get_char(prev);
			int nextch = ctoken_get_char(next);
			int value = ctoken_get_int(token);
			if (prevch == '*' || nextch == '*') {
				if (value == 1 || value == 2 || value == 4 || value == 8) {
					csynth_encode_scale(&parser->synthesizer, value);
				}	else {
					cparser_error(parser, 
						"Invalid scale in memory reference", 23);
					return mem;
				}
			}
			else if (prevch == '-') {
				csynth_encode_displacement(&parser->synthesizer, -value);
			}
			else if (prevch == '+' || nextch == '+') {
				csynth_encode_displacement(&parser->synthesizer, value);
			}
			else if (prevch == '[' && nextch == ']') {
				cparser_error(parser,
					"Invalid number in memory reference", 30);
				return mem;
			}
			else {
				cparser_error(parser, 
					"Invalid number in memory reference", 24);
				return mem;
			}
		}
		else {
			cparser_error(parser, 
				"Unexpected token in memory reference", 25);
			return mem;
		}
	}

	cparser_error(parser, "Unexpected end of line in memory reference", 26);

	return mem;
}


static int cparser_parse_data(CParser *parser)
{
	const char *name;
	long pos = 0;
	int size = -1;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "DB") == 0) size = 1;
	else if (stricmp(name, "DW") == 0) size = 2;
	else if (stricmp(name, "DD") == 0) size = 4;

	if (size < 0) return 0;

	cscanner_token_advance(parser->token, 1);

	for (pos = 0; ; ) {
		unsigned char *ptr = (unsigned char*)parser->data;
		const CTOKEN *token;

		if (cscanner_is_endl(parser->token)) break;

		token = cscanner_token_current(parser->token);

		if (token->type == CTokenINT) {
			cuint32 value = (cuint32)token->intval;
			if (pos + size >= IMAX_DATA) {
				cparser_error(parser, "data too long", 41);
				return -1;
			}
			if (size == 1) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
			}
			else if (size == 2) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
				ptr[pos++] = (unsigned char)((value >>  8) & 0xff);
			}
			else if (size == 4) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
				ptr[pos++] = (unsigned char)((value >>  8) & 0xff);
				ptr[pos++] = (unsigned char)((value >> 16) & 0xff);
				ptr[pos++] = (unsigned char)((value >> 24) & 0xff);
			}
		}
		else if (token->type == CTokenSTR) {
			const char *text = token->str;
			long size, i, c;
			char hex[3];
			size = (long)strlen(text);
			for (i = 0; i < size; ) {
				if (i + 1 >= IMAX_DATA) {
					cparser_error(parser, "data too long", 41);
					return -2;
				}
				if (text[i] == '\\') {
					switch (text[i + 1])
					{
					case '\\': ptr[pos++] = '\\'; i += 2; break;
					case 'n' : ptr[pos++] = '\n'; i += 2; break;
					case 'r' : ptr[pos++] = '\r'; i += 2; break;
					case 't' : ptr[pos++] = '\t'; i += 2; break;
					case '0' : ptr[pos++] = '\0'; i += 2; break;
					case '?' : ptr[pos++] = '?'; i += 2; break;
					case '\'': ptr[pos++] = '\''; i += 2; break;
					case '\"': ptr[pos++] = '\"'; i += 2; break;
					case 'a' : ptr[pos++] = '\a'; i += 2; break;
					case 'b' : ptr[pos++] = '\b'; i += 2; break;
					case 'f' : ptr[pos++] = '\f'; i += 2; break;
					case 'v' : ptr[pos++] = '\v'; i += 2; break;
					case 'x' :
						i += 2;
						hex[0] = text[i++];
						hex[1] = text[i++];
						hex[2] = 0;
						c = strtol(hex, NULL, 16);
						ptr[pos++] = (unsigned char)(c & 255);
						break;
					default:
						cparser_error(parser, "string format error", 42);
						return -3;
						break;
					}
				}
				else if (text[i] == '\'') {
					if (text[i + 1] == '\'') {
						ptr[pos++] = '\'';
						i += 2;
					}	else {
						ptr[pos++] = '\'';
						i += 1;
					}
				}
				else if (text[i] == '\"') {
					if (text[i + 1] == '\"') {
						ptr[pos++] = '\"';
						i += 2;
					}	else {
						ptr[pos++] = '\"';
						i += 1;
					}
				}
				else {
					ptr[pos++] = (unsigned char)text[i++];
				}
			}
		}
		else {
			cparser_error(parser, "unrecongnize data", 43);
			return -4;
		}

		cscanner_token_advance(parser->token, 1);

		if (!cscanner_is_endl(parser->token)) {
			if (cscanner_get_char(parser->token) != ',') {
				cparser_error(parser, "expected comma", 40);
				return -3;
			}
			cscanner_token_advance(parser->token, 1);
		}
	}

	if (pos > 0) {
		cencoding_set_data(&parser->synthesizer.encoding, parser->data, pos);
	}

	return 0;
}


static int cparser_parse_prefix(CParser *parser)
{
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "REP") == 0 ||
		stricmp(name, "REPE") == 0 ||
		stricmp(name, "REPZ") == 0) {
		if (csynth_encode_prefix(&parser->synthesizer, 0xf3)) {
			cparser_error(parser, parser->synthesizer.error, 70);
			return -1;
		}
		cscanner_token_advance(parser->token, 1);
	}
	else if (stricmp(name, "REPNE") == 0 || stricmp(name, "REPNZ") == 0) {
		parser->synthesizer.prefix = 0xf2;
		if (csynth_encode_prefix(&parser->synthesizer, 0xf2)) {
			cparser_error(parser, parser->synthesizer.error, 71);
			return -2;
		}
		cscanner_token_advance(parser->token, 1);
	}
	else if (stricmp(name, "LOCK") == 0) {
		if (csynth_encode_prefix(&parser->synthesizer, 0xf0)) {
			cparser_error(parser, parser->synthesizer.error, 72);
			return -3;
		}
		cscanner_token_advance(parser->token, 1);
	}

	return 0;
}

static int cparser_parse_align(CParser *parser)
{
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);
	
	if (stricmp(name, "ALIGN") == 0) {
		int align = 4;
		cscanner_token_advance(parser->token, 1);
		if (cscanner_is_int(parser->token)) {
			align = cscanner_get_value(parser->token);
		}
		while (!cscanner_is_endf(parser->token)) {
			cscanner_token_advance(parser->token, 1);
		}
		if (align < 1) {
			cparser_error(parser, "error align size", 80);
			return -1;
		}
		parser->synthesizer.encoding.align = align;
	}

	return 0;
}

static int cparser_parse_size(CParser *parser)
{
	const CTOKEN *token = cscanner_token_current(parser->token);
	cscanner_token_advance(parser->token, 1);
	if (token->type == CTokenIDENT) {
		if (stricmp(token->str, "BYTE") == 0) return 1;
		if (stricmp(token->str, "CHAR") == 0) return 1;
		if (stricmp(token->str, "INT8") == 0) return 1;
		if (stricmp(token->str, "UINT8") == 0) return 1;
		if (stricmp(token->str, "WORD") == 0) return 2;
		if (stricmp(token->str, "SHORT") == 0) return 2;
		if (stricmp(token->str, "USHORT") == 0) return 2;
		if (stricmp(token->str, "INT16") == 0) return 2;
		if (stricmp(token->str, "UINT16") == 0) return 2;
		if (stricmp(token->str, "DWORD") == 0) return 4;
		if (stricmp(token->str, "INT") == 0) return 4;
		if (stricmp(token->str, "UINT") == 0) return 4;
		if (stricmp(token->str, "LONG") == 0) return 4;
		if (stricmp(token->str, "ULONG") == 0) return 4;
		if (stricmp(token->str, "INT32") == 0) return 4;
		if (stricmp(token->str, "UINT32") == 0) return 4;
	}
	return 0;
}

static int cparser_parse_newvar(CParser *parser, const char *name, int stack)
{
	char *macro = (char*)parser->data;
	CVariable *var;

	if (stricmp(name, "RET") == 0) {
		sprintf(macro, "'%s' conflicted with keyword", name);
		cparser_error(parser, macro, 96);
		return -1;
	}

	for (var = parser->vars; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			sprintf(macro, "'%s' redefined", name);
			cparser_error(parser, macro, 97);
			return -2;
		}
	}

	if (stack >= 0) sprintf(macro, "[EBP + %d]", stack);
	else sprintf(macro, "[EBP - %d]", -stack);

	if (cscanner_macro_set(parser->token, name, macro)) {
		sprintf(macro, "name '%s' redefined", name);
		cparser_error(parser, macro, 95);
		return -3;
	}

	var = (CVariable*)malloc(sizeof(CVariable));
	assert(var);
	var->name = strdup(name);
	assert(var->name);
	var->pos = stack;

	var->next = parser->vars;
	parser->vars = var;

	return 0;
}

static int cparser_parse_proc(CParser *parser)
{
	unsigned char instruction[20];
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "PROC") == 0) {
		const char *replace = "DB 0x8B, 0xE5, 0x5D, 0xC3\n";
		int stack = 8;

		if (parser->inproc) {
			cparser_error(parser, "cannot define proc in a proc block", 90);
			return -1;
		}
		parser->inproc = 1;
		parser->stack = 0;

		// replace ret to "mov esp, ebp; pop ebp; ret"
		cscanner_macro_set(parser->token, "ret", replace);
		cscanner_macro_set(parser->token, "RET", replace);
		cscanner_macro_set(parser->token, "Ret", replace);
		cscanner_macro_set(parser->token, "rEt", replace);
		cscanner_macro_set(parser->token, "reT", replace);
		cscanner_macro_set(parser->token, "rET", replace);
		cscanner_macro_set(parser->token, "ReT", replace);
		cscanner_macro_set(parser->token, "REt", replace);

		cscanner_token_advance(parser->token, 1);

		for (stack = 8; !cscanner_is_endl(parser->token); ) {
			const CTOKEN *token = cscanner_token_current(parser->token);
			const CTOKEN *next = cscanner_token_lookahead(parser->token);
			char *macro = (char*)parser->data;
			if (ctoken_get_char(token) == ',') {
				cscanner_token_advance(parser->token, 1);
			}
			else if (token->type == CTokenIDENT && next->ch == ':') {
				int size;
				cscanner_token_advance(parser->token, 2);
				size = cparser_parse_size(parser);
				if (size == 0) {
					cparser_error(parser, "variable type unknown", 93);
					return -1;
				}
				if (cparser_parse_newvar(parser, token->str, stack)) {
					return -4;
				}
				stack += size;
			}
			else {
				if (token->type == CTokenIDENT) {
					sprintf(macro, "parameter '%s' error", token->str);
				}	else {
					sprintf(macro, "parameter error");
				}
				cparser_error(parser, macro, 93);
				return -3;
			}
		}

		instruction[0] = 0x55;		// push ebp
		instruction[1] = 0x8B;		// mov ebp, esp
		instruction[2] = 0xEC;

		cencoding_set_data(&parser->synthesizer.encoding, instruction, 3);
	}
	else if (stricmp(name, "LOCAL") == 0) {
		int localsize = 0;
		int IS;

		cscanner_token_advance(parser->token, 1);

		if (parser->inproc == 0) {
			cparser_error(parser, "local is forbbiden outside a proc", 90);
			return -5;
		}

		for (; !cscanner_is_endl(parser->token); ) {
			const CTOKEN *token = cscanner_token_current(parser->token);
			const CTOKEN *next = cscanner_token_lookahead(parser->token);
			char *macro = (char*)parser->data;
			if (ctoken_get_char(token) == ',') {
				cscanner_token_advance(parser->token, 1);
			}
			else if (token->type == CTokenIDENT && next->ch == ':') {
				int pos, size;
				cscanner_token_advance(parser->token, 2);
				size = cparser_parse_size(parser);
				if (size == 0) {
					cparser_error(parser, "variable type unknown", 93);
					return -1;
				}
				pos = -(parser->stack + size);
				if (cparser_parse_newvar(parser, token->str, pos)) {
					return -6;
				}
				parser->stack += size;
				localsize += size;
				//printf("LOCAL %s=[EBP+(%d)]\n", token->str, pos);
			}
			else {
				if (token->type == CTokenIDENT) {
					sprintf(macro, "parameter '%s' error", token->str);
				}	else {
					sprintf(macro, "parameter error");
				}
				cparser_error(parser, macro, 92);
				return -3;
			}
		}

		if (localsize <= 127) {
			instruction[0] = 0x83;		// sub esp, imm8
			instruction[1] = 0xEC;
			instruction[2] = (unsigned char)(localsize & 0xff);
			IS = 3;
		}	else {
			instruction[0] = 0x81;		// sub esp, imm32
			instruction[1] = 0xEC;
			instruction[2] = (unsigned char)((localsize >>  0) & 0xff);
			instruction[3] = (unsigned char)((localsize >>  8) & 0xff);
			instruction[4] = (unsigned char)((localsize >> 16) & 0xff);
			instruction[5] = (unsigned char)((localsize >> 24) & 0xff);
			IS = 6;
		}

		cencoding_set_data(&parser->synthesizer.encoding, instruction, IS);
	}
	else if (stricmp(name, "ENDP") == 0) {
		if (parser->inproc == 0) {
			cparser_error(parser, "not find proc definition", 91);
			return -2;
		}
		parser->inproc = 0;
		parser->stack = 0;
		while (parser->vars) {
			CVariable *var = parser->vars;
			parser->vars = parser->vars->next;
			cscanner_macro_del(parser->token, var->name);
			free(var->name);
			free(var);
		}
		cscanner_macro_del(parser->token, "ret");
		cscanner_macro_del(parser->token, "RET");
		cscanner_macro_del(parser->token, "Ret");
		cscanner_macro_del(parser->token, "rEt");
		cscanner_macro_del(parser->token, "reT");
		cscanner_macro_del(parser->token, "rET");
		cscanner_macro_del(parser->token, "ReT");
		cscanner_macro_del(parser->token, "REt");
	}
	else {
		return 0;
	}

	while (!cscanner_is_endf(parser->token)) {
		cscanner_token_advance(parser->token, 1);
	}
	
	return 0;
}



//---------------------------------------------------------------------
// CAssembler Definition
//---------------------------------------------------------------------
struct CAssembler
{
	CParser *parser;
	CLoader *loader;
	int srcblock;
	int srcsize;
	char *line;
	char *source;
	char *error;
	int errcode;
	int lineno;
};


#define IMAX_LINESIZE		4096

//---------------------------------------------------------------------
// CORE INTERFACE
//---------------------------------------------------------------------

// create assembler
CAssembler *casm_create(void)
{
	CAssembler *self;
	self = (CAssembler*)malloc(sizeof(CAssembler));
	assert(self);
	self->parser = cparser_create();
	assert(self->parser);
	self->loader = cloader_create();
	assert(self->loader);
	self->source = (char*)malloc(1024 + 1);
	assert(self->source);
	self->srcblock = 1024;
	self->srcsize = 0;
	self->source[0] = 0;
	self->line = (char*)malloc(IMAX_LINESIZE + 10);
	assert(self->line);
	self->error = (char*)malloc(2048);
	assert(self->error);
	self->error[0] = 0;
	self->errcode = 0;
	return self;
}

// reset assembler
void casm_reset(CAssembler *self)
{
	assert(self);
	if (self->source) free(self->source);
	self->source = (char*)malloc(1024 + 1);
	assert(self->source);
	self->srcblock = 1024;
	self->srcsize = 0;
	self->source[0] = 0;
	cloader_reset(self->loader);
	cparser_reset(self->parser);
}

// release assembler
void casm_release(CAssembler *self)
{
	assert(self);
	if (self->parser) {
		cparser_release(self->parser);
		self->parser = NULL;
	}
	if (self->loader) {
		cloader_release(self->loader);
		self->loader = NULL;
	}
	if (self->source) {
		free(self->source);
		self->source = NULL;
	}
	if (self->line) {
		free(self->line);
		self->line = NULL;
	}
	if (self->error) {
		free(self->error);
		self->error = NULL;
	}
	self->srcblock = 0;
	self->srcsize = 0;
	free(self);
}

// add source to source buffer
int casm_source(CAssembler *self, const char *text)
{
	int datasize = (int)strlen(text);
	int newsize = datasize + self->srcsize;
	int newblock = 1;
	while (newblock < newsize) newblock <<= 1;
	if (newblock != self->srcblock) {
		char *buffer = (char*)malloc(newblock + 1);
		assert(buffer);
		memcpy(buffer, self->source, self->srcsize);
		buffer[self->srcsize] = 0;
		free(self->source);
		self->source = buffer;
		self->srcblock = newblock;
	}
	memcpy(self->source + self->srcsize, text, datasize);
	self->srcsize = newsize;
	self->source[newsize] = 0;
	return 0;
}

// prompt error
static void casm_error(CAssembler *self, const char *msg, int code)
{
	sprintf(self->error, "line(%d): error(%d): %s", self->lineno, code, msg);
	self->errcode = code;
}

// compile single line
static int casm_compile_line(CAssembler *self, const char *line)
{
	const CEncoding *encoding;

	assert(self);

	self->error[0] = 0;
	self->errcode = 0;

	encoding = cparser_parse_line(self->parser, line);

	if (encoding == NULL) {
		casm_error(self, self->parser->error, self->parser->errcode);
		return -1;
	}

	cloader_new_encoding(self->loader, encoding);

	return 0;
}

// compile source buffer
// if (code == NULL) returns compiled code size
// if (code != NULL) and (maxsize >= codesize) compile and returns codesize
// if (code != NULL) and (maxsize < codesize) returns error
int casm_compile(CAssembler *self, unsigned char *code, long maxsize)
{
	int lineno, p1, p2;
	const char *text;
	long codesize;

	assert(self);

	text = self->source;

	cloader_reset(self->loader);
	cparser_reset(self->parser);

	for (lineno = 1, p1 = 0; p1 < self->srcsize; ) {
		for (p2 = p1; text[p2] != 0 && text[p2] != '\n'; p2++);
		self->lineno = lineno++;
		if (p2 - p1 >= IMAX_LINESIZE) {
			casm_error(self, "line size too long", 1);
			return -1;
		}

		memcpy(self->line, self->source + p1, p2 - p1);
		self->line[p2 - p1] = 0;
		p1 = p2 + 1;

		if (casm_compile_line(self, self->line) != 0) {
			return -2;
		}
	}

	codesize = cloader_get_codesize(self->loader) + 10;

	if (code == NULL) 
		return codesize;

	if (maxsize < codesize) {
		casm_error(self, "need a larger memory block to get code", 2);
		return -3;
	}

	memset(code, 0xcc, codesize);

	if (cloader_output(self->loader, code) != 0) {
		self->lineno = self->loader->errcode;
		casm_error(self, self->loader->error, 3);
		return -4;
	}

	return codesize;
}


// get error
const char *casm_geterror(const CAssembler *self, int *errcode)
{
	if (errcode) *errcode = self->errcode;
	return self->error;
}


//---------------------------------------------------------------------
// HIGH LEVEL
//---------------------------------------------------------------------
int casm_pushline(CAssembler *self, const char *fmt, ...)
{
	char *buffer = self->error;
	va_list argptr;

	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	casm_source(self, buffer);
	casm_source(self, "\n");

	self->error[0] = 0;

	return 0;
}


void *casm_callable(CAssembler *self, long *codesize)
{
	unsigned char *code;
	long size;

	if (codesize) *codesize = 0;

	size = casm_compile(self, NULL, 0);

	if (size < 0) {
		return NULL;
	}

	code = (unsigned char*)malloc(size + 1);
	assert(code);

	if (casm_compile(self, code, size) < 0) {
		free(code);
		return NULL;
	}

	if (codesize) *codesize = size;

	return code;
}


// load assembly source file
int casm_loadfile(CAssembler *self, const char *filename)
{
	char line[80];
	FILE *fp;
	casm_reset(self);
	if ((fp = fopen(filename, "r")) == NULL) 
		return -1;
	while (!feof(fp)) {
		int size = (int)fread(line, 1, 60, fp);
		line[size] = 0;
		casm_source(self, line);
	}
	fclose(fp);
	return 0;
}

int casm_savefile(CAssembler *self, const char *filename)
{
	char *codedata, *p;
	long codesize;
	FILE *fp;

	codedata = (char*)casm_callable(self, &codesize);
	if (codedata == NULL) return -1;

	if ((fp = fopen(filename, "wb")) == NULL) {
		free(codedata);
		return -2;
	}

	for (p = codedata; p < codedata + codesize; ) {
		int canwrite = codesize - (int)(p - codedata);
		int hr = (int)fwrite(p, 1, canwrite, fp);
		if (hr > 0) p += hr;
	}

	fclose(fp);

	free(codedata);

	return 0;
}

int casm_dumpinst(CAssembler *self, FILE *fp)
{
	CLoader *loader = self->loader;
	int lineno, p1, p2, maxsize, pos;
	const char *text;
	char *codedata;
	iqueue_head *node;

	text = self->source;

	codedata = (char*)casm_callable(self, NULL);
	if (codedata == NULL) return -1;
	free(codedata);

	node = loader->head.next;

	for (maxsize = 0; node != &loader->head; node = node->next) {
		CLink *link = iqueue_entry(node, CLink, head);
		int length = cencoding_length(&link->encoding);
		if (length > maxsize) maxsize = length;
	}

	node = loader->head.next;
	fp = (fp != NULL)? fp : stdout;

	for (lineno = 1, p1 = 0, pos = 0; p1 < self->srcsize; lineno++) {
		for (p2 = p1; text[p2] != 0 && text[p2] != '\n'; p2++);
		if (p2 - p1 >= IMAX_LINESIZE) {
			casm_error(self, "line size too long", 1);
			return -1;
		}

		memcpy(self->line, self->source + p1, p2 - p1);
		self->line[p2 - p1] = 0;
		p1 = p2 + 1;

		while (node != &loader->head) {
			CLink *link = iqueue_entry(node, CLink, head);
			if (link->lineno >= lineno) break;
			node = node->next;
		}

		if (node != &loader->head) {
			CLink *link = iqueue_entry(node, CLink, head);
			if (link->lineno == lineno) {
				static char output[4096];
				int length, size;
				length = cencoding_length(&link->encoding);
				if (link->encoding.align > 0) {
					int align, i, k;
					align = link->encoding.align;
					length = align - (pos % align);
					for (i = length, k = 0; i > 0; ) {
						if (i >= 2) {
							output[k++] = '6';
							output[k++] = '6';
							output[k++] = ' ';
							i--;
						}
						output[k++] = '9';
						output[k++] = '0';
						output[k++] = ' ';
						i--;
					}
					output[k++] = 0;
				}	else {
					cencoding_to_string(&link->encoding, output);
				}
				for (size = (int)strlen(output); size < (maxsize) * 3; )
					output[size++] = ' ';
				output[size] = 0;
				if (length == 0) fprintf(fp, "         ");
				else fprintf(fp, "%08X:", pos);
				pos += length;
				fprintf(fp, "  %s\t%s\n", output, self->line);
			}
		}
	}

	return 0;
}

