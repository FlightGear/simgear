dnl Process this file with aclocal ; automake -a ; autoconf to produce a 
dnl working configure script.
dnl
dnl $Id$

AC_INIT
AC_CONFIG_SRCDIR([simgear/bucket/newbucket.cxx])

# Require at least automake 2.52
AC_PREREQ(2.52)

dnl Initialize the automake stuff
AM_INIT_AUTOMAKE(SimGear, 0.3.0)

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


# Used on the Irix platform
AR="ar"
ARFLAGS="cru"
OS=`uname -s`
if test "$OS" = "IRIX" -o "$OS" = "IRIX64"; then
    if test "$CXX" = "CC"; then
        AR="CC -ar"
        ARFLAGS="-o"
    fi
fi
AC_SUBST(AR)
AC_SUBST(ARFLAGS)


if echo $includedir | egrep "simgear$" > /dev/null; then
    echo "includedir is" $includedir "libdir is" $libdir
else
    includedir="${includedir}/simgear"
    echo "includedir changed to" $includedir "libdir is" $libdir
fi
         
# set logging default value
# with_logging=yes
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


dnl Check for MS Windows environment
AC_CHECK_HEADER(windows.h)

AC_EGREP_CPP(yes,
[#ifdef __MINGW32__
 yes
 #endif
],is_mingw=yes, is_mingw=no)

echo "IS_MINGW = $is_mingw"
AM_CONDITIONAL(IS_MINGW, test "x$is_mingw" = "xyes")

AC_EGREP_CPP(yes,
[#ifdef __CYGWIN__
 yes
 #endif
],is_cygwin=yes, is_cygwin=no)

echo "IS_CYGWIN = $is_cygwin"
AM_CONDITIONAL(IS_CYGWIN, test "x$is_cygwin" = "xyes")

if test "x$HOSTTYPE" != "xmacintosh" -a "x$is_mingw" != "xyes"; then
    dnl extra library and include directories
    EXTRA_DIRS="/usr/local /usr/X11R6"

    if test -d /opt/X11R6 ; then
        EXTRA_DIRS="$EXTRA_DIRS /opt/X11R6"
    fi
fi

wi_EXTRA_DIRS(no, ${EXTRA_DIRS})

dnl Using AM_CONDITIONAL is a step out of the protected little 
dnl automake fold so it is potentially dangerous.  But, we are
dnl beginning to run into cases where the standard checks are not
dnl enough.  AM_CONDITIONALS are then referenced to conditionally
dnl build a Makefile.in from a Makefile.am which lets us define custom
dnl includes, compile alternative source files, etc.

dnl X11 might be installed on Mac OS X, don't want to use it if it is.
if test "x$HOSTTYPE" != "xmacintosh" ; then
    dnl Check for X11 (fancy)
    AC_PATH_XTRA
fi

dnl Checks for libraries.

null_LIBS="$LIBS"

AC_CHECK_LIB(m, cos)

base_LIBS="$LIBS"

dnl Thread related checks
AC_CHECK_LIB(pthread, pthread_exit)
AC_CHECK_HEADER(pthread.h)
if test "x$ac_cv_lib_pthread_pthread_exit" = "xyes" -a "x$ac_cv_header_pthread_h" = "xyes"; then
    CXXFLAGS="$CXXFLAGS -D_REENTRANT"
    CFLAGS="$CFLAGS -D_REENTRANT"
fi
AM_CONDITIONAL(HAVE_THREADS, test "x$ac_cv_lib_pthread_pthread_exit" = "xyes" -a "x$ac_cv_header_pthread_h" = "xyes")

AC_CHECK_LIB(socket, socket)

dnl X11 might be installed, but we don't want to use it for OSX -dw-
if test "x$HOSTTYPE" != "xmacintosh" ; then
    AC_CHECK_LIB(X11, XCreateWindow)
    AC_CHECK_LIB(Xext, XShmCreateImage)
    AC_CHECK_LIB(Xi, XGetExtensionVersion)
    AC_CHECK_LIB(ICE, IceOpenConnection)
    AC_CHECK_LIB(SM, SmcOpenConnection)
    AC_CHECK_LIB(Xt, XtMalloc)
    AC_CHECK_LIB(Xmu, XmuLookupStandardColormap)
fi

dnl check for OpenGL related libraries

if test "x$HOSTTYPE" = "xmacintosh" ; then
    dnl Macintosh OSX
    LIBS="$LIBS -framework OpenGL -framework GLUT"
elif test "x$ac_cv_header_windows_h" != "xyes" ; then
    dnl Reasonable stuff for X-Windows based machines

    AC_CHECK_LIB(GLcore, glNewList)
    if test "x$ac_cv_lib_GLcore_glNewList" = "xno" ; then
	dnl if no GLcore, check for GL
	AC_CHECK_LIB(GL, glNewList)
	if test "x$ac_cv_lib_GL_glNewList" = "xno" ; then
	    dnl if no GL, check for MesaGL
	    AC_CHECK_LIB(MesaGL, glNewList)
	fi
    else
	dnl if GLcore found, then also check for GL
	AC_CHECK_LIB(GL, glXCreateContext)
    fi

    dnl if using mesa, check for xmesa.h
    if test "x$ac_cv_lib_MesaGL_glNewList" = "xyes" ; then
	AC_CHECK_HEADER(GL/fxmesa.h)
	if test "x$ac_cv_header_GL_fxmesa_h" = "xyes"; then
	    AC_DEFINE([XMESA], 1, [Define for fxmesa])
            AC_DEFINE([FX], 1, [Define for fxmesa])
        fi
    fi

    AC_CHECK_LIB(GLU, gluLookAt)
    if test "x$ac_cv_lib_GLU_gluLookAt" = "xno" ; then
	dnl if no GLU, check for MesaGLU
	AC_CHECK_LIB(MesaGLU, gluLookAt)
    fi

    dnl check for glut
    AC_CHECK_LIB(glut, glutGetModifiers)

    dnl test for glutGameModeString, but avoid adding glut a second time into
    dnl the list of libraries
    save_LIBS="$LIBS"
    AC_CHECK_LIB(glut, glutGameModeString)
    LIBS="$save_LIBS"
else
    dnl Win32 libs

    echo Win32 specific hacks...
    AC_DEFINE([WIN32], 1, [Define for Win32 platforms])

    dnl just define these to true and hope for the best
    ac_cv_lib_glut_glutGetModifiers="yes"
    ac_cv_lib_glut_glutGameModeString="yes"

    if test "x$with_sgi_opengl" = "xyes" ; then
        echo "Building with glut.dll, glu.dll, and opengl.dll"
	WIN32_GLUT=glut
	WIN32_GLU=glu
	WIN32_OPENGL=opengl
    else 
	echo "Building with glut32.dll, glu32.dll, and opengl32.dll"
	WIN32_GLUT=glut32
	WIN32_GLU=glu32
	WIN32_OPENGL=opengl32
    fi

    LIBS="$LIBS -l${WIN32_GLUT} -l${WIN32_GLU} -l${WIN32_OPENGL}"
    LIBS="$LIBS -luser32 -lgdi32"
    if test "x$is_mingw" = "xyes" ; then
        EXTRA_DIRS="${EXTRA_DIRS}"
    fi
    echo "Will link apps with $LIBS"
fi

if test "x$ac_cv_lib_glut_glutGetModifiers" = "xno"; then
    echo 
    echo "Unable to find the necessary OpenGL or GLUT libraries."
    echo "See config.log for automated test details and results ..."
    exit 1
fi

if test "x$ac_cv_lib_glut_glutGameModeString" = "xno"; then
    echo
    echo "Your version of glut doesn't support game mode."
    echo "You need to install the latest version of glut.  If your"
    echo "distribution doesn't provide a newer version, you can get the source"
    echo "code from:"
    echo
    echo "    http://reality.sgi.com/opengl/glut3/glut3.html"
    exit 1
fi

opengl_LIBS="$LIBS"
LIBS="$base_LIBS"

AC_SUBST(base_LIBS)
AC_SUBST(opengl_LIBS)

AM_CONDITIONAL(HAVE_XWINDOWS, test "x$ac_cv_lib_X11_XCreateWindow" = "xyes" )

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

dnl Check for system installed metakit
AC_CHECK_HEADER(mk4.h)
if test "x$ac_cv_header_mk4_h" != "xyes"; then
    echo 
    echo "Metakit not found, you will need to install this first."
    echo "Please read the README.metakit for more information."
    exit
fi

AC_LANG_POP

dnl Specify if we want logging (testing build) or not (release build)

dnl Check for system installed zlib
AC_CHECK_HEADER(zlib.h)
if test "x$ac_cv_header_zlib_h" != "xyes"; then
    echo 
    echo "zlib not found, you will need to install this first."
    echo "Please read the README.zlib for more information."
    exit
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS( \
	fcntl.h getopt.h malloc.h memory.h stdint.h stdlib.h sys/param.h \
	sys/stat.h sys/time.h sys/timeb.h unistd.h windows.h winbase.h values.h )

if test "x$ac_cv_header_stdint_h" = "xyes"; then
    AC_DEFINE([HAVE_STDINT_H], 1, [Define if stdint.h exists])
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS( ftime gettimeofday timegm memcpy bcopy mktime strstr rand \
	random drand48 setitimer getitimer signal GetLocalTime rint getrusage )

AM_CONFIG_HEADER(simgear/simgear_config.h)

AC_CONFIG_FILES([ \
	Makefile \
	SimGear.spec \
	src-libs/Makefile \
	simgear/Makefile \
	simgear/version.h \
	simgear/bucket/Makefile \
	simgear/debug/Makefile \
	simgear/ephemeris/Makefile \
	simgear/interpreter/Makefile \
	simgear/io/Makefile \
	simgear/magvar/Makefile \
	simgear/math/Makefile \
	simgear/metar/Makefile \
	simgear/misc/Makefile \
	simgear/route/Makefile \
	simgear/screen/Makefile \
	simgear/serial/Makefile \
	simgear/sky/Makefile \
	simgear/sky/clouds3d/Makefile \
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

echo -n "Automake version: ($AUTO_MAKE_VERSION) "
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
