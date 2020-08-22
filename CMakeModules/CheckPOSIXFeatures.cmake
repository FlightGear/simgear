check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(rint HAVE_RINT)
check_function_exists(mkdtemp HAVE_MKDTEMP)
check_function_exists(bcopy HAVE_BCOPY)
check_function_exists(mmap HAVE_MMAP)

check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(windows.h HAVE_WINDOWS_H)

if (NOT MSVC)
  check_function_exists(timegm HAVE_TIMEGM)
  if (NOT HAVE_TIMEGM)
    message(FATAL_ERROR "Non-Windows platforms must support timegm()")
  endif()
endif()

if(HAVE_UNISTD_H)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
    check_cxx_source_compiles(
       "#include <unistd.h>
        #if !defined(_POSIX_TIMERS) || (0 >= _POSIX_TIMERS)
            #error clock_gettime is not supported
        #endif

        int main() { return 0; }
        "
        HAVE_CLOCK_GETTIME)
endif(HAVE_UNISTD_H)

set(RT_LIBRARY "")
if(HAVE_CLOCK_GETTIME)
    check_library_exists(rt clock_gettime "" HAVE_RT)
    if(HAVE_RT)
        set(RT_LIBRARY rt)
    endif(HAVE_RT)
endif(HAVE_CLOCK_GETTIME)

set(DL_LIBRARY "")
check_cxx_source_compiles(
    "#include <dlfcn.h>
    int main(void) {
        return 0;
    }
    "
    HAVE_DLFCN_H)

if(HAVE_DLFCN_H)
    check_library_exists(dl dlerror "" HAVE_DL)
    if(HAVE_DL)
        set(DL_LIBRARY "dl")
    endif()
endif()

