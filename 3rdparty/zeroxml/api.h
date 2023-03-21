/*
 * This software is available under 2 licenses -- choose whichever you prefer.
 *
 * ALTERNATIVE A - Modified BSD license
 *
 * Copyright (C) 2008-2023 by Erik Hofman.
 * Copyright (C) 2009-2023 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ADALIN B.V. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ADALIN B.V. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUTOF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Adalin B.V.
 *
 * -----------------------------------------------------------------------------
 * ALTERNATIVE B - Public Domain (www.unlicense.org)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of
 * this software dedicate any and all copyright interest in the software to
 * the public domain. We make this dedication for the benefit of the public at
 * large and to the detriment of our heirs and successors. We intend this
 * dedication to be an overt act of relinquishment in perpetuity of all
 * present and future rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __XML_API_H
#define __XML_API_H 1

#include <simgear_zeroxml_config.h>

#include <stdio.h>
#if HAVE_LOCALE_H
# include <locale.h>
#endif

#include <xml.h>

#ifdef NDEBUG
# ifdef DEBUG
#  undef DEBUG
# endif
#endif

#if HAVE_ICONV_H
# include <iconv.h>
#endif
#ifdef USE_RMALLOC
# define USE_LOGGING    1
# include <rmalloc.h>
#else
# define USE_LOGGING    0
# include <stdlib.h>
# include <string.h>
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# define UNICODE
# include <windows.h>

# ifndef __GNUC__
#  define strtoll _strtoi64
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
# endif

typedef _locale_t locale_t;
typedef const char* iconv_t;

# define iconv_close(l)
# define iconv_open(l,e)        (e)
size_t iconv(iconv_t, char**, size_t*, char**, size_t*);

typedef struct
{
    HANDLE m;
    void *p;
} SIMPLE_UNMMAP;

/* map 'filename' and return a pointer to it. */
void *simple_mmap(int, int, SIMPLE_UNMMAP *);
void simple_unmmap(void*, int, SIMPLE_UNMMAP *);

#else
# define simple_mmap(a, b, c)   mmap(0, (b), PROT_READ, MAP_PRIVATE, (a), 0L)
# define simple_unmmap(a, b, c) munmap((a), (b))
#endif

#ifndef XML_NONVALIDATING
# define FILENAME_LEN		1024
# define BUF_LEN		2048

# ifndef NDEBUG
#  define PRINT_INFO(a, b, c) \
    if (0 <= (c) && (c) < XML_MAX_ERROR) { \
        int i, last = 0, nl = 1; \
        if (a) for (i=0; i<(b)-(a)->root->start; ++i) { \
            if ((a)->root->start[i] == '\n') { last = i+1; nl++; } \
        } \
        snprintf(__zeroxml_strerror, BUF_LEN, "%s:\n\t%s at line %i offset %i\n", __zeroxml_filename, __zeroxml_error_str[(c)], nl, i-last); \
        fprintf(stderr, "%s\tdetected in %s at line %i\n", __zeroxml_strerror, __func__, __LINE__); \
    } else { \
        fprintf(stderr, "%s: in %s at line %i: Unknown error number %i\n", \
                        __zeroxml_filename, __func__, __LINE__, c); \
    }

#  define SET_ERROR(a, d, b, c) do { \
     __zeroxml_set_error_debug((a), (d), (b), (c), __func__, __LINE__); PRINT_INFO((a), (char*)(b), (c)); \
   } while(0)

# else /* NDEBUG */
#  define SET_ERROR(a, d, b, c) __zeroxml_set_error(a, d, b, c);
#  define PRINT_INFO(a, b, c)

# endif /* NDEBUG */

#else /* XML_NONVALIDATING */
# define PRINT_INFO(a, b, c)
# define SET_ERROR(a, b, c)

#endif /*XML_NONVALIDATING */

#define MEMCMP(a,b,c)		memcmp((a),(b),(c))
#define MEMCHR(a,b,c)		memchr((a),(b),(c))
#define CASECMP(rid,a,b)	((CASE(rid,a)) == (CASE(rid,b)))
#define LSTRNCMP(a,b,c,d)	string_compare((a),(b),(c),(d))

int string_compare(iconv_t, const char*, const char*, int*);
int __zeroxml_iconv(const struct _root_id*, const char*, size_t, char*, size_t);

#if defined(HAVE_LOCALE_H) && !defined(WIN32)
# define CASE(rid,a) (rid)->lcase ? (rid)->lcase((a),(rid)->locale) : (a)
#else
# define CASE(rid,a) (rid)->lcase ? (rid)->lcase(a) : (a)
#endif
#define STRNCMP(rid,a,b,c) (rid)->strncmp((a),(b),(c))

enum _xml_flags
{
    __XML_INDEX_STARTS_AT_ONE  = 0x01,
    __XML_RETURN_NONE_VALUE    = 0x02,
    __XML_CASE_SENSITIVE       = 0x04,
    __XML_COMMENT_AS_NODE      = 0x08,
    __XML_VALIDATING           = 0x10,
    __XML_CACHED_NODES         = 0x20,
    __XML_LOCALIZATION         = 0x40,

    __XML_DEFAULT_MODE         = (-1) /* all true */
};

#define INDEX_STARTS_AT_ONE(a)	((a)->root->flags & __XML_INDEX_STARTS_AT_ONE)
#define RETURN_NONE_VALUE(a)	((a)->root->flags & __XML_RETURN_NONE_VALUE)
#define CASE_SENSITIVE(a)	((a)->root->flags & __XML_CASE_SENSITIVE)
#define COMMENT_AS_NODE(a)	((a)->root->flags & __XML_COMMENT_AS_NODE)
#define VALIDATING(a)		((a)->root->flags & __XML_VALIDATING)
#define CACHED_NODES(a)		((a)->root->flags & __XML_CACHED_NODES)
#define LOCALIZATION(a)		((a)->root->flags & __XML_LOCALIZATION)

#define __XML_BOOL_NONE        RETURN_NONE_VALUE(xid) ? XML_BOOL_NONE : 0
#define __XML_FPNONE           RETURN_NONE_VALUE(xid) ? XML_FPNONE : 0.0
#define __XML_NONE             RETURN_NONE_VALUE(xid) ? XML_NONE : 0


#define MAX_ENCODING	32
#define MMAP_FREE	-2
#define MMAP_ERROR	-1
#define STRIPPED	 0
#define RAW		 1

#include <xml_cache.h>

#ifndef XML_NONVALIDATING
struct _zeroxml_error
{
    const char *start;
    const char *pos;
    int column;
    int line;
    int err_no;
    int line_no;
    const char *func;
};
#endif

/*
 * It is required for both the rood node and the normal xml nodes to both
 * have 'char *name' defined as the first entry. The code tests whether
 * name == 0 to detect the root node.
 */
struct _root_id
{
    struct _root_id *root;
    const char *name;
    const char *start;
    off_t len;
    const cacheId *node;

    /* _root_id specifics */
    int fd;
    enum _xml_flags flags;
    char *mmap;
    char encoding[MAX_ENCODING+1];

#if defined(HAVE_ICONV_H) || defined(WIN32)
    iconv_t cd;
#endif

    struct _zeroxml_error *info;

#ifdef WIN32
    SIMPLE_UNMMAP un;
#endif

#if defined(HAVE_LOCALE_H) && !defined(WIN32)
    locale_t locale;
    int (*lcase)(int, locale_t);
#else
    int (*lcase)(int);
#endif
    int (*strncmp)(const char*, const char*, size_t);
};

struct _xml_id
{
    struct _root_id *root;
    const char *name;
    const char *start;
    off_t len;
    const cacheId *node;

    /* _xml_id specifics */
    off_t name_len;
};

#define PRINT(s, b, c) { \
  int l1 = (b), l2 = (c); \
  if (s) { \
    int q, len = l2; \
    if (l1 < l2) len = l1; \
    if (len < 50000) { \
        printf("(%i) '", len); \
        for (q=0; q<len; q++) printf("%c", (s)[q]); \
        printf("'\n"); \
    } else printf("Length (%i) seems too large at line %i\n",len, __LINE__); \
  } else printf("NULL pointer at line %i\n", __LINE__); \
}

#endif /* __XML_API_H */
