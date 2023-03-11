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

#include <simgear_zeroxml_config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_LOCALE_H
# include <locale.h>
#endif
#include <wchar.h>
#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif
#include <errno.h>

#include "xml.h"
#include "api.h"

/*
 * A Unicode string comparison function that handles strings with different
 * character encodings.
 *
 * In this code, the string_compare function first converts the input strings
 * s1 and s2 from the specified encoding to wide character strings ws1 and ws2
 * using the iconv library. The iconv_open function opens a conversion
 * descriptor, and the iconv function performs the conversion. Finally, wcscoll
 * is used to compare the wide character strings.
 */
#define BUFSIZE		1024
int
string_compare(iconv_t cd, const char *s1, const char *s2, int *s2len)
{
#if defined(HAVE_ICONV_H) || defined(WIN32)
    size_t s1len = strlen(s1);
    int rv = -1;

    if (s1len > 0 && *s2len > 0)
    {
        char buffer[BUFSIZE+1];
        char *outbuf = buffer;
        char *inbuf = (char*)s2;
        size_t outbytesleft = BUFSIZE;
        size_t inbytesleft = *s2len; // strlen(s1);
        size_t nconv;

        iconv(cd, NULL, NULL, NULL, NULL);
        nconv = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        if (nconv != (size_t)-1)
        {
            iconv(cd, NULL, NULL, &outbuf, &outbytesleft);
            if ((rv = memcmp(s1, buffer, s1len)) == 0) {
                *s2len = inbuf-s2;
            }
        }
    }
    return rv;
#else
    return STRNCMP(s1, s2, *slen);
#endif
}

/*
 * Convert a string from XML defined character encoding to local encoding.
 *
 * @param cd character encoding conversion descriptor
 * @param out
 * @param olen
 * @param in
 * @param ilen
 * @return
 */
int
__zeroxml_iconv(const struct _root_id *rid,
                const char *inbuf, size_t inbytesleft,
                char *outbuf, size_t outbytesleft)
{
    iconv_t cd = rid->cd;
    char cvt = XML_FALSE;
    int rv = XML_NO_ERROR;

#if (defined(HAVE_ICONV_H) || defined(WIN32))
    if (LOCALIZATION(rid))
    {
        outbuf[0] = 0;
        if (cd != (iconv_t)-1)
        {
            char *ptr = (char*)inbuf;
            size_t nconv;
            iconv(cd, NULL, NULL, NULL, NULL);
            nconv = iconv(cd, &ptr, &inbytesleft, &outbuf, &outbytesleft);
            if (nconv != (size_t)-1)
            {
                iconv(cd, NULL, NULL, &outbuf, &outbytesleft);
                outbuf[0] = 0;
                cvt = XML_TRUE;
            }
            else
            {
                outbuf[outbytesleft] = 0;
                switch (errno)
                {
                case EILSEQ:
                    rv = XML_INVALID_MULTIBYTE_SEQUENCE;
                    break;
                case EINVAL:
                    rv = XML_INVALID_MULTIBYTE_SEQUENCE;
                    break;
                case E2BIG:
                    rv = XML_TRUNCATE_RESULT;
                    break;
                default:
                    break;
                }
            }
        }
    } /* LOCALIZED(rid) */
#endif

    if (cvt == XML_FALSE)
    {
        if (outbytesleft > inbytesleft) outbytesleft = inbytesleft;
        memcpy(outbuf, inbuf, outbytesleft);
        outbuf[inbytesleft] = 0;
    }
    return rv;
}

#ifdef WIN32
/*
 * A basic implementation of the iconv function for Windows in C:
 *
 * This implementation uses the Windows API functions MultiByteToWideChar and
 * WideCharToMultiByte to convert between different character sets. The
 * in_charset and out_charset parameters are compared against a list of known
 * character sets, and the appropriate code page is selected. If the input or
 * output character set is UTF-8, the corresponding code page is set to CP_UTF8.
 * The conversion is performed using MultiByteToWideChar to convert the input
 * buffer to a wide character buffer, and then WideCharToMultiByte to convert
 * the wide character buffer to the output buffer.
 *
 * https://www.iana.org/assignments/character-sets/character-sets.xhtml
 */
#define CP_GBK		  936
#define CP_UTF16LE	 1200 /* default Windows Unicode codepage identifier */
#define CP_1252		 1252
#define CP_UTF32LE	12000
#define CP_LATIN1       28591
#define CP_ASCII        20127
#define CP_GB2312	20936
#define CP_UTF8		65001

static UINT
charset_to_identifier(const char *charset)
{
    UINT identifier = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    if (strcasecmp(charset, "UTF-8") == 0) identifier = CP_UTF8;
    else if (strcasecmp(charset, "UTF-16") == 0) identifier = CP_UTF16LE;
    else if (strcasecmp(charset, "ISO-8859-1") == 0) identifier = CP_LATIN1;
    else if (strcasecmp(charset, "ASCII") == 0) identifier = CP_ASCII;
    else if (strcasecmp(charset, "US-ASCII") == 0) identifier = CP_ASCII;
    else if (strcasecmp(charset, "UTF-32") == 0) identifier = CP_UTF32LE;
    else  if (strcasecmp(charset, "GBK") == 0) identifier = CP_GBK;
    else  if (strcasecmp(charset, "GB-2312") == 0) identifier = CP_GB2312;

    return identifier;
}

size_t
iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
                  char **outbuf, size_t *outbytesleft)
{
    if (inbuf && *inbuf && inbytesleft &&
        outbuf && *outbuf && outbytesleft)
    {
        UINT code_page = charset_to_identifier(cd);
        wchar_t *wbuf;
	int res;

        res = MultiByteToWideChar(code_page, 0, *inbuf, *inbytesleft, NULL, 0);
        if (res <= 0)
        {
            // call GetLastError
            // ERROR_INSUFFICIENT_BUFFER: errno = E2BIG
            // ERROR_NO_UNICODE_TRANSLATION: errno = EILSEQ
            return -1;
        }

        wbuf = (wchar_t*)malloc(res*sizeof(wchar_t));
        res =MultiByteToWideChar(code_page, 0, *inbuf, *inbytesleft, wbuf, res);
        if (res <= 0)
        {
            free(wbuf);
            return -1;
        }

        *inbuf += res;
        res = WideCharToMultiByte(CP_1252, 0, wbuf, res,
                                  *outbuf, *outbytesleft, NULL, NULL);
        free(wbuf);
        if (res <= 0)
        {
            // call GetLastError
            // ERROR_INSUFFICIENT_BUFFER: errno = E2BIG
            // ERROR_NO_UNICODE_TRANSLATION: errno = EILSEQ
            return -1;
        }

        *inbytesleft = res;
        *outbytesleft -= res;
        *outbuf += res;
    }
    return 0;
}
#endif /* def WIN32 */

