includedir = @includedir@/threads

lib_LIBRARIES = libsgthreads.a

include_HEADERS = \
	SGGuard.hxx \
	SGQueue.hxx \
	SGThread.hxx

libsgthreads_a_SOURCES = \
	SGThread.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir)
else
INCLUDES = -I$(top_srcdir)
endif
