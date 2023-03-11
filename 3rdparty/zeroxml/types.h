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
 * ----------------------------------------------------------------------------- * ALTERNATIVE B - Public Domain (www.unlicense.org)
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

#ifndef __OAL_TYPES_H
#define __OAL_TYPES_H 1

#if defined(__cplusplus)
extern "C" {
#endif

#include <simgear_zeroxml_config.h>

#if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2
#    define __func__ __FUNCTION__
#  elif defined(_MSC_VER)
#    if _MSC_VER < 1300
#      define __func__ "<unknown>"
#    else
#      define __func__ __FUNCTION__
#    endif
#  elif defined(__BORLANDC__)
#    define __func__ __FUNC__
#  else
#    define __func__ "<unknown>"
#  endif
#endif

#if _MSC_VER
# include <Windows.h>
# define strtoll _strtoi64
# define snprintf _snprintf
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
# define rintf(v) (int)(v+0.5f)
# define msecSleep(tms) SleepEx((DWORD)tms, FALSE)

struct timespec
{
  time_t tv_sec; /* seconds */
  long tv_nsec;  /* nanoseconds */
};

struct timezone
{
  int tz_minuteswest; /* of Greenwich */
  int tz_dsttime;     /* type of dst correction to apply */
};
int gettimeofday(struct timeval*, void*);

typedef long	off_t;
# if SIZEOF_SIZE_T == 4
typedef INT32	ssize_t;
#else
typedef INT64	ssize_t;
#endif

#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>		/* for gettimeofday */
# endif
int msecSleep(unsigned int);
#endif

#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif /* !__OAL_TYPES_H */

