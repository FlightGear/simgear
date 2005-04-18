includedir = @includedir@/nasal

lib_LIBRARIES = libsgnasal.a

include_HEADERS = nasal.h

libsgnasal_a_SOURCES = \
	code.c code.h \
	codegen.c \
        data.h \
	gc.c \
	hash.c \
	lex.c \
	lib.c \
	mathlib.c \
	misc.c \
	nasal.h \
	parse.c parse.h \
	string.c \
	vector.c \
	thread-posix.c \
	thread-win32.c

INCLUDES = -I$(top_srcdir)
