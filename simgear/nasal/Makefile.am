includedir = @includedir@/nasal

lib_LIBRARIES = libsgnasal.a

include_HEADERS = nasal.h naref.h

libsgnasal_a_SOURCES = bitslib.c code.c code.h codegen.c data.h gc.c	\
                       hash.c iolib.c iolib.h lex.c lib.c mathlib.c	\
                       misc.c naref.h nasal.h parse.c parse.h string.c	\
                       thread-posix.c thread-win32.c threadlib.c	\
                       utf8lib.c vector.c

INCLUDES = -I$(top_srcdir)
