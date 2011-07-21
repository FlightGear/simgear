cmake_minimum_required (VERSION 2.6)
include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckCXXSourceCompiles)
include (CPack)

project(SimGear)

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ version versionFile)
string(STRIP ${versionFile} SIMGEAR_VERSION)

#packaging
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

option(SIMGEAR_SHARED "Set to ON to build SimGear as a shared library/framework" OFF)
option(SIMGEAR_HEADLESS "Set to ON to build SimGear with GUI/graphics support" OFF)
option(JPEG_FACTORY "Enable JPEG-factory support" OFF)
option(ENABLE_LIBSVN "Set to ON to build SimGear with libsvnclient support" OFF)

set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")

if (MSVC AND MSVC_3RDPARTY_ROOT)
  message(STATUS "3rdparty files located in ${MSVC_3RDPARTY_ROOT}")
  set( OSG_MSVC "msvc" )
  if (${MSVC_VERSION} EQUAL 1600)
      set( OSG_MSVC ${OSG_MSVC}100 )
  else (${MSVC_VERSION} EQUAL 1600)
      set( OSG_MSVC ${OSG_MSVC}90 )
  endif (${MSVC_VERSION} EQUAL 1600)
  if (CMAKE_CL_64)
      set( OSG_MSVC ${OSG_MSVC}-64 )
      set( MSVC_3RDPARTY_DIR 3rdParty.x64 )
  else (CMAKE_CL_64)
      set( MSVC_3RDPARTY_DIR 3rdParty )
  endif (CMAKE_CL_64)

  set (CMAKE_LIBRARY_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/lib )
  set (CMAKE_INCLUDE_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/include)
  set (BOOST_ROOT ${MSVC_3RDPARTY_ROOT}/boost_1_44_0)
  set (OPENAL_INCLUDE_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
  set (ALUT_INCLUDE_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
  set (OPENAL_LIBRARY_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib) 
endif (MSVC AND MSVC_3RDPARTY_ROOT)

find_package(Boost REQUIRED)
set (BOOST_CXX_FLAGS "-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION -DBOOST_BIMAP_DISABLE_SERIALIZATION")

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if (${SIMGEAR_HEADLESS})
    message(STATUS "headlesss mode")
    set(NO_OPENSCENEGRAPH_INTERFACE 1)
else()
    find_package(OpenGL REQUIRED)
    find_package(OpenAL REQUIRED)
    find_package(ALUT REQUIRED)
    find_package(OpenSceneGraph 2.8.1 REQUIRED osgText osgSim osgDB osgParticle osgUtil)
endif()

if(JPEG_FACTORY)
    message(STATUS "JPEG-factory enabled")
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
endif()

if(ENABLE_LIBSVN)
	find_package(SvnClient)
	
	if(LIBSVN_FOUND)
		message(STATUS "libsvn found, enabling in SimGear")
		set(HAVE_SVN_CLIENT_H 1)
		set(HAVE_LIBSVN_CLIENT_1 1)
	endif(LIBSVN_FOUND)
endif(ENABLE_LIBSVN)

check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(windows.h HAVE_WINDOWS_H)

# See if we have any rti library variant installed
find_package(RTI)

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(ftime HAVE_FTIME)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(rint HAVE_RINT)


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
    check_function_exists(clock_gettime CLOCK_GETTIME_IN_LIBC)
    if(NOT CLOCK_GETTIME_IN_LIBC)
        check_library_exists(rt clock_gettime "" HAVE_RT)
        if(HAVE_RT)
            set(RT_LIBRARY rt)
        endif(HAVE_RT)
        endif(NOT CLOCK_GETTIME_IN_LIBC)
endif(HAVE_CLOCK_GETTIME)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# isnan might not be real symbol, so can't check using function_exists
check_cxx_source_compiles(
    "#include <cmath> 
    void f() { isnan(0.0);} "
    HAVE_ISNAN)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS -Wall)
endif(CMAKE_COMPILER_IS_GNUCXX)

if(WIN32)

    if(MSVC)
        # turn off various warnings
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305 4996)
        #     SET(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
        # endforeach(warning)
        
        set(MSVC_FLAGS "-DWIN32 -DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS")
    endif(MSVC)
    
    # assumed on Windows
    set(HAVE_GETLOCALTIME 1)
endif(WIN32)    

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} ${BOOST_CXX_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR}/simgear)

include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS} 
    ${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} 
    ${ALUT_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} )


add_definitions(-DHAVE_CONFIG_H)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/simgear/simgear_config_cmake.h.in"
  "${PROJECT_BINARY_DIR}/simgear/simgear_config.h"
  )
  
# enable CTest / make test target

include (Dart)
enable_testing()
  
install (FILES ${PROJECT_BINARY_DIR}/simgear/simgear_config.h  DESTINATION include/simgear/)
add_subdirectory(simgear)

#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")



