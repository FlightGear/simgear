dnl Process this file with autoget.sh to produce a working configure
dnl script.
dnl
dnl $Id$

AC_INIT
AC_CONFIG_SRCDIR([simgear/bucket/newbucket.cxx])

dnl Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
AM_INIT_AUTOMAKE(SimGear, 0.3.7)

dnl Specify KAI C++ compiler and flags.
dnl Borrowed with slight modification from blitz distribution.
AC_ARG_WITH(cxx,
  [  --with-cxx=COMPILER[:name-flags] set options for COMPILER (KCC)],
  [case "$withval" in
    KCC*)    # KAI C++ http://www.kai.com/
      echo "Configuring for KAI C++"
      AC_SG_SET_COMPILER($withval,"KCC","--restrict --strict_warnings")
      CXX_OPTIMIZE_FLAGS=="+K3 -O3"
      CXX_DEBUG_FLAGS="-g +K0"
    ;;
  esac
])

echo CXX = $CXX
echo CC = $CC

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

dnl set the $host variable based on local machine/os
AC_CANONICAL_HOST

dnl Used on the Irix platform
case "${host}" in
*-*-irix*)
    if test "x$CXX" = "xCC" -o "x$CXX" = "xccache CC"; then
        AR="$CXX -ar"
        ARFLAGS="-o"
        CXXFLAGS="$CXXFLAGS -I$(top_srcdir)/simgear/compatibility/MIPSpro740"
        compatibility_DIR="compatibility"
        MIPSpro_DIRS="MIPSpro740"
        AC_MSG_CHECKING([for MIPSpro compiler version 7.4 or newer])
        AC_TRY_RUN([
           int main() {
              if ( _COMPILER_VERSION < 740 ) {
                 return -1;
              }
              return 0;
           }

        ], AC_MSG_RESULT(yes),
        [  AC_MSG_RESULT(no)
           CXXFLAGS="$CXXFLAGS -I$(top_srcdir)/simgear/compatibility/MIPSpro721"
           MIPSpro_DIRS="$(MIPSpro_DIRS) MIPSpro721"
           AC_MSG_WARN([Using our own subset of the STL headers])
        ], AC_MSG_RESULT(yes))
        AC_SUBST(MIPSpro_DIRS)
    fi
    ;;
*)
    AR="ar"
    ARFLAGS="cru"
    compatibility_DIR=
    ;;
esac
AC_SUBST(AR)
AC_SUBST(ARFLAGS)
AC_SUBST(compatibility_DIR)

if echo $includedir | egrep "simgear$" > /dev/null; then
    echo "includedir is" $includedir "libdir is" $libdir
else
    includedir="${includedir}/simgear"
    echo "includedir changed to" $includedir "libdir is" $libdir
fi
         
dnl set logging; default value of with_logging=yes
AC_ARG_WITH(logging, [  --with-logging          Include logging output (default)])
if test "x$with_logging" = "xno" ; then
    AC_DEFINE([FG_NDEBUG], 1, [Define for no logging output])
fi

# Specify if we want to build with Norman's jpeg image server support.
# This requires libjpeg to be installed and available.
# Default to with_jpeg_server=no
JPEGLIB=''
AC_ARG_WITH(jpeg_factory, [  --with-jpeg-factory     Include Norman's jpeg image factory support code])
if test "x$with_jpeg_factory" = "xyes" ; then
    echo "Building with Norman's jpeg image factory support"
    AC_CHECK_LIB(jpeg, jpeg_start_compress)
    if test "x$ac_cv_lib_jpeg_jpeg_start_compress" != "xyes" ; then
        echo
        echo "In order to build the jpeg factory code you need libjpeg installed."
        echo "otherwise please configure with the --with-jpeg-sever=no option"
        echo
        echo "libjpeg is available at :"
        echo "  ftp://ftp.uu.net in the directory graphics/jpeg"
        exit 1
    fi
else
    echo "Building without Norman's jpeg image server support"
fi
AM_CONDITIONAL(ENABLE_JPEG_SERVER, test "x$with_jpeg_factory" = "xyes")

# specify the plib location
AC_ARG_WITH(plib, [  --with-plib=PREFIX      Specify the prefix path to plib])

if test "x$with_plib" != "x" ; then
    echo "plib prefix is $with_plib"
    EXTRA_DIRS="${EXTRA_DIRS} $with_plib"
fi

dnl Determine an extra directories to add to include/lib search paths
case "${host}" in
*-apple-darwin* | *-*-mingw32*)
    echo no EXTRA_DIRS for $host
    ;;

*)
    if test -d /usr/X11R6 ; then
        EXTRA_DIR1="/usr/X11R6"
    fi
    if test -d /opt/X11R6 ; then
        EXTRA_DIR2="/opt/X11R6"
    fi
    EXTRA_DIRS="${EXTRA_DIRS} $EXTRA_DIR1 $EXTRA_DIR2"
    ;;

esac
wi_EXTRA_DIRS(no, ${EXTRA_DIRS})


dnl Using AM_CONDITIONAL is a step out of the protected little 
dnl automake fold so it is potentially dangerous.  But, we are
dnl beginning to run into cases where the standard checks are not
dnl enough.  AM_CONDITIONALS are then referenced to conditionally
dnl build a Makefile.in from a Makefile.am which lets us define custom
dnl includes, compile alternative source files, etc.

dnl X11 might be installed on Mac OS X or cygwin/mingwin, we don't want
dnl to use it if it is.
case "${host}" in
*-apple-darwin* | *-*-cygwin* | *-*-mingw32*)
    echo no fancy X11 check
    ;;

*)
    AC_PATH_XTRA
    ;;

esac

dnl Checks for libraries.

dnl Thread related checks
AC_CHECK_HEADER(pthread.h)
AC_CHECK_LIB(pthread, pthread_exit)
if test "x$ac_cv_lib_pthread_pthread_exit" = "xyes" -a "x$ac_cv_header_pthread_h" = "xyes"; then
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
fi
if test "x$ac_cv_lib_pthread_pthread_exit" != "xyes" -a "x$ac_cv_header_pthread_h" = "xyes"; then
    dnl FreeBSD: System has pthread.h, but -lpthread library check
    dnl fails.  See if we need -pthread instead of -lpthread and look
    dnl for the functions in libc_r.
    save_CXXFLAGS="$CXXFLAGS"
    save_CFLAGS="$CFLAGS"
    CXXFLAGS="-pthread $CXXFLAGS"
    CFLAGS="-pthread $FLAGS"
    save_LIBS=$LIBS
    AC_CHECK_LIB(c_r, pthread_exit)
    if test "x$ac_cv_lib_c_r_pthread_exit" != "xyes"; then
        CXXFLAGS=$save_CXXFLAGS
        CFLAGS=$save_CFLAGS
    else
        dnl This is cheating a bit. pthread_exit comes with using -pthread, not
-lpthread
        ac_cv_lib_pthread_pthread_exit="yes"
    fi
    LIBS=$save_LIBS
fi

AM_CONDITIONAL(HAVE_THREADS, test "x$ac_cv_lib_pthread_pthread_exit" = "xyes" -a "x$ac_cv_header_pthread_h" = "xyes")

thread_LIBS="$LIBS"
LIBS=""

dnl search for network related libraries
AC_SEARCH_LIBS(inet_addr, xnet)
AC_SEARCH_LIBS(socket, socket)

network_LIBS="$LIBS"
LIBS=""

dnl check for some default libraries
AC_SEARCH_LIBS(cos, m)

base_LIBS="$LIBS"

dnl check for OpenGL related libraries
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.

    echo Win32 specific hacks...
    AC_DEFINE([WIN32], 1, [Define for Win32 platforms])
    AC_DEFINE([NOMINMAX], 1, [Define for Win32 platforms])

    LIBS="$LIBS -lglu32 -lopengl32"
    LIBS="$LIBS -luser32 -lgdi32 -lwinmm"

    dnl add -lwsock32 for mingwin
    case "${host}" in
    *-*-mingw32*)
        base_LIBS="$base_LIBS -lws2_32"
        ;;
    esac

    echo "Will link apps with $LIBS"
    ;;

*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework OpenGL -framework Carbon -lobjc"
    ;;

*)
    dnl X-Windows based machines

    AC_SEARCH_LIBS(XCreateWindow, X11)
    AC_SEARCH_LIBS(XShmCreateImage, Xext)
    AC_SEARCH_LIBS(XGetExtensionVersion, Xi)
    AC_SEARCH_LIBS(IceOpenConnection, ICE)
    AC_SEARCH_LIBS(SmcOpenConnection, SM)
    AC_SEARCH_LIBS(XtMalloc, Xt)
    AC_SEARCH_LIBS(XmuLookupStandardColormap, Xmu)
    
    AC_SEARCH_LIBS(glNewList, [ GL GLcore MesaGL ])
    if test "x$ac_cv_search_glNewList" = "x-lGLcore"; then
	dnl if GLcore found, then also check for GL
        AC_SEARCH_LIBS(glXCreateContext, GL)
    fi

    dnl if using mesa, check for xmesa.h
    if test "x$ac_cv_search_glNewList" = "x-lMesaGL"; then
	AC_CHECK_HEADER(GL/fxmesa.h)
	if test "x$ac_cv_header_GL_fxmesa_h" = "xyes"; then
	    AC_DEFINE([XMESA], 1, [Define for fxmesa])
            AC_DEFINE([FX], 1, [Define for fxmesa])
        fi
    fi

    AC_SEARCH_LIBS(gluLookAt, [ GLU MesaGLU ])
    ;;

esac

opengl_LIBS="$LIBS"
LIBS="$base_LIBS"

dnl check for OpenAL libraries
OPENAL_OK="no"
case "${host}" in
*-*-cygwin* | *-*-mingw32*)
    dnl CygWin under Windoze.

    AC_SEARCH_LIBS(alGenBuffers, openal32)
    AC_SEARCH_LIBS(alutInit, [ openal32 ALut ] )
    LIBS="$LIBS -lwinmm -ldsound -ldxguid -lole32"
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    ;;

*-apple-darwin*)
    dnl Mac OS X

    LIBS="$LIBS -framework IOKit -framework OpenAL"
    # not sure how to test if OpenAL exists on MacOS (does it come by default?)
    OPENAL_OK="yes"
    ;;

*)
    dnl default unix style machines

    AC_SEARCH_LIBS(alGenBuffers, openal)
    OPENAL_OK="$ac_cv_search_alGenBuffers"
    ;;

esac

if test "$OPENAL_OK" == "no"; then
    echo
    echo "You *must* have the openal library installed on your system to build"
    echo "SimGear!"
    echo
    echo "Please see README.OpenAL for more details."
    echo
    echo "configure aborted."
    exit
fi

openal_LIBS="$LIBS"
LIBS="$base_LIBS"

AC_SUBST(base_LIBS)
AC_SUBST(openal_LIBS)
AC_SUBST(opengl_LIBS)
AC_SUBST(thread_LIBS)
AC_SUBST(network_LIBS)

dnl Check for MS Windows environment
AC_CHECK_HEADER(windows.h)
AM_CONDITIONAL(EXTGL_NEEDED, test "x$ac_cv_header_windows_h" = "xyes")

# The following are C++ items that need to be tested for with the c++
# compiler

AC_LANG_PUSH(C++)

dnl Check for "plib" without which we cannot go on
AC_CHECK_HEADER(plib/ul.h)
if test "x$ac_cv_header_plib_ul_h" != "xyes"; then
    echo
    echo "You *must* have the plib library installed on your system to build"
    echo "SimGear!"
    echo
    echo "Please see README.plib for more details."
    echo
    echo "configure aborted."
    exit
fi

AC_MSG_CHECKING([for plib 1.6.0 or newer])
AC_TRY_RUN([
#include <plib/ul.h>

#define MIN_PLIB_VERSION 160

int main() {
    int major, minor, micro;

    if ( PLIB_VERSION < MIN_PLIB_VERSION ) {
	 return -1;
    }

    return 0;
}

],
  AC_MSG_RESULT(yes),
  [AC_MSG_RESULT(wrong version);
   AC_MSG_ERROR([Install plib 1.6.0 or later first...])],
  AC_MSG_RESULT(yes)
)

LIBS="$saved_LIBS"

AC_LANG_POP

dnl Check for system installed zlib
AC_CHECK_HEADER(zlib.h)
if test "x$ac_cv_header_zlib_h" != "xyes"; then
    echo
    echo "zlib library not found."
    echo
    echo "If your OS does not provide an installable package for zlib"
    echo "you will have to compile and install it first yourself.  A copy"
    echo "of zlib-1.1.4.tar.gz is included with SimGear.  You will"
    echo "have to untar this source code, and follow its included instructions"
    echo "to compile and install on your system."
    echo
    echo "configure aborted."
    echo
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS( \
	fcntl.h getopt.h malloc.h memory.h stdint.h stdlib.h sys/param.h \
	sys/stat.h sys/time.h sys/timeb.h unistd.h values.h )

if test "x$ac_cv_header_stdint_h" = "xyes"; then
    AC_DEFINE([HAVE_STDINT_H], 1, [Define if stdint.h exists])
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
old_LIBS=$LIBS
LIBS="$base_LIBS $network_LIBS $opengl_LIBS"
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS( [ \
        ftime gettimeofday timegm memcpy bcopy mktime strstr rand \
	random drand48 setitimer getitimer signal GetLocalTime rint getrusage ] )
LIBS=$old_LIBS

AM_CONFIG_HEADER(simgear/simgear_config.h)

AC_CONFIG_FILES([ \
	Makefile \
	SimGear.spec \
	src-libs/Makefile \
	simgear/Makefile \
	simgear/version.h \
	simgear/compatibility/Makefile \
	simgear/compatibility/MIPSpro721/Makefile \
	simgear/compatibility/MIPSpro740/Makefile \
	simgear/bucket/Makefile \
	simgear/debug/Makefile \
	simgear/ephemeris/Makefile \
	simgear/io/Makefile \
	simgear/magvar/Makefile \
	simgear/math/Makefile \
	simgear/environment/Makefile \
	simgear/misc/Makefile \
	simgear/nasal/Makefile \
	simgear/props/Makefile \
	simgear/route/Makefile \
	simgear/scene/Makefile \
	simgear/scene/fgsg/Makefile \
	simgear/scene/material/Makefile \
	simgear/scene/model/Makefile \
	simgear/scene/sky/Makefile \
	simgear/scene/sky/clouds3d/Makefile \
	simgear/scene/tgdb/Makefile \
	simgear/screen/Makefile \
	simgear/serial/Makefile \
	simgear/sound/Makefile \
	simgear/structure/Makefile \
	simgear/threads/Makefile \
	simgear/timing/Makefile \
	simgear/xgl/Makefile \
	simgear/xml/Makefile \
])
AC_OUTPUT


echo ""
echo "Configure Summary"
echo "================="

echo "Prefix: $prefix"

if test "x$with_logging" != "x"; then
    echo "Debug messages: $with_logging"
else
    echo "Debug messages: yes"
fi

echo -n "Automake version: "
automake --version | head -1

if test "x$with_jpeg_factory" = "xyes"; then
   echo "With JPEG Factory support"
else
   echo "Without JPEG Factory support"
fi

if test "x$ac_cv_lib_pthread_pthread_exit" = "xyes" -a "x$ac_cv_header_pthread_h" = "xyes"; then
   echo "Threads: pthread lib found."
else
   echo "Threads: no threads (pthread lib not found.)"
fi

