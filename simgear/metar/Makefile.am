includedir = @includedir@/metar

lib_LIBRARIES = libsgmetar.a

include_HEADERS = MetarReport.h MetarStation.h 

libsgmetar_a_SOURCES = \
	Antoi.cpp Charcmp.cpp Dcdmetar.cpp Dcdmtrmk.cpp Drvmetar.c \
	Fracpart.cpp Local.h Metar.h \
	MetarReport.cpp \
	MetarStation.cpp \
	Prtdmetr.cpp Stspack2.cpp Stspack3.cpp

INCLUDES += -I$(top_srcdir)
