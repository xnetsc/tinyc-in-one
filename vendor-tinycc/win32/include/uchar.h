/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the TinyCC package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_UCHAR
#define _INC_UCHAR

#include <stddef.h>
#include <wchar.h>

/*
 * The character types and the __STDC_UTF_16__ / __STDC_UTF_32__
 * macros are C11 surface.  They are declared for every C
 * compilation (a C++ compilation has the types as keywords);
 * the conversion functions are implemented by
 * win32/lib/uchar.c with its own UTF-8 codec and ship inside
 * libtcc1.a, because the target runtime exports no
 * mbrtoc16/c16rtomb/mbrtoc32/c32rtomb.
 */

#ifndef __cplusplus
/** __STDC_UTF_16__ The integer constant 1, intended to indicate that
 * values of type char16_t are UTF-16 encoded.
 */
#define __STDC_UTF_16__ 1
/**
 * __STDC_UTF_32__ The integer constant 1, intended to indicate that
 * values of type char32_t are UTF-32 encoded.
 */
#define __STDC_UTF_32__ 1

typedef unsigned short char16_t;
typedef unsigned int char32_t;
#endif

size_t mbrtoc16(char16_t *_Dst, const char *_Src, size_t _SizeInBytes, mbstate_t *_State);
size_t c16rtomb(char *_Dst, char16_t _Src, mbstate_t *_State);
size_t mbrtoc32(char32_t *_Dst, const char *_Src, size_t _SizeInBytes, mbstate_t *_State);
size_t c32rtomb(char *_Dst, char32_t _Src, mbstate_t *_State);

#endif /* _INC_UCHAR */
