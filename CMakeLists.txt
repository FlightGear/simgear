cmake_minimum_required (VERSION 2.6)
include (CheckFunctionExists)
include (CheckCXXSourceCompiles)
include (CPack)

project(SimGear)

set(SIMGEAR_VERSION "2.2.0")

#packaging
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

option(SIMGEAR_SHARED "Set to ON to build SimGear as a shared library/framework" OFF)
option(SIMGEAR_HEADLESS "Set to ON to build SimGear with GUI/graphics support" OFF)
option(JPEG_FACTORY "Enable JPEG-factory support" OFF)
option(MSVC_3RDPARTY_DIR "Location where the third-party dependencies are extracted" NOT_FOUND)

if (MSVC_3RDPARTY_DIR)
  message(STATUS "3rdparty files located in ${MSVC_3RDPARTY_DIR}")
  set (CMAKE_LIBRARY_PATH ${MSVC_3RDPARTY_DIR}/3rdParty/lib)
  set (CMAKE_INCLUDE_PATH ${MSVC_3RDPARTY_DIR}/3rdParty/include)
  set (BOOST_ROOT ${MSVC_3RDPARTY_DIR}/boost_1_44_0)
  set (OPENAL_INCLUDE_DIR ${MSVC_3RDPARTY_DIR}/3rdParty/include)
  set (OPENAL_LIBRARY_DIR ${MSVC_3RDPARTY_DIR}/3rdParty/lib) 
  set (OSGDIR ${MSVC_3RDPARTY_DIR}/install/msvc90/OpenScenegraph)
endif (MSVC_3RDPARTY_DIR)

find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if (${SIMGEAR_HEADLESS})
    message(STATUS "headlesss mode")
    set(NO_OPENSCENEGRAPH_INTERFACE 1)
else()
    find_package(OpenGL REQUIRED)
    find_package(OpenAL REQUIRED)
    find_package(ALUT REQUIRED)
    find_package(OpenSceneGraph 2.8.2 REQUIRED osgText osgSim osgDB osgParticle osgUtil)
endif()

if(JPEG_FACTORY)
    message(STATUS "JPEG-factory enabled")
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
endif()

find_path (HAVE_SYS_TIME_H sys/time.h )
find_path (HAVE_SYS_TIMEB_H sys/timeb.h )
find_path (HAVE_UNISTD_H unistd.h )
find_path (HAVE_WINDOWS_H windows.h)

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(ftime HAVE_FTIME)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(rint HAVE_RINT)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS}")

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
  
install (FILES ${PROJECT_BINARY_DIR}/simgear/simgear_config.h  DESTINATION include/simgear/)
add_subdirectory(simgear)

