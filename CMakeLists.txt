cmake_minimum_required (VERSION 3.0)

if(COMMAND cmake_policy)
   if(POLICY CMP0054)
       cmake_policy(SET CMP0054 NEW)
   endif()
   if(POLICY CMP0042)
       cmake_policy(SET CMP0042 NEW)
   endif()
   if(POLICY CMP0067)
       cmake_policy(SET CMP0067 NEW)
   endif()
    
  # OpenGL VND policy : use the old definition for now, until we can audit this
  if(POLICY CMP0072)
        cmake_policy(SET CMP0072 OLD)
  endif()

   if(POLICY CMP0093)
       cmake_policy(SET CMP0093 NEW)
   endif()
endif()


message(STATUS "CMAKE Build type: ${CMAKE_BUILD_TYPE}")
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckLibraryExists)
include (CheckCXXSourceCompiles)
include (CheckCXXCompilerFlag)
include (GenerateExportHeader)

# only relevant for building shared libs but let's set it regardless
set(CMAKE_OSX_RPATH 1)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

# let's use & require C++11 - note these are only functional with CMake 3.1
# we do manual fallbacks for CMake 3.0 in the compilers section
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ simgear-version versionFile)
string(STRIP ${versionFile} SIMGEAR_VERSION)

project(SimGear VERSION ${SIMGEAR_VERSION} LANGUAGES C CXX)

# add a dependency on the version file
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version)

set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

# use simgear version also as the SO version (if building SOs)
SET(SIMGEAR_SOVERSION ${SIMGEAR_VERSION})

# Warning when build is not an out-of-source build.
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" InSourceBuild)
if(InSourceBuild)
    message(WARNING  "Avoid building inside the source tree!")
    message(WARNING  "Create a separate build directory instead (i.e. 'sgbuild') and call CMake from there: ")
    message(WARNING  "  mkdir ../sgbuild && cd ../sgbuild && cmake ${CMAKE_SOURCE_DIR}")
endif(InSourceBuild)

#packaging
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulation support libraries for FlightGear and related projects")
SET(CPACK_PACKAGE_VENDOR "The FlightGear project")
SET(CPACK_GENERATOR "TBZ2")
SET(CPACK_INSTALL_CMAKE_PROJECTS  ${CMAKE_CURRENT_BINARY_DIR};SimGear;ALL;/)


# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CPACK_PACKAGE_VERSION ${SIMGEAR_VERSION} )
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})

message(STATUS "version is ${CPACK_PACKAGE_VERSION_MAJOR} dot ${CPACK_PACKAGE_VERSION_MINOR} dot ${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "simgear-${SIMGEAR_VERSION}" CACHE INTERNAL "tarball basename")
set(CPACK_SOURCE_IGNORE_FILES
  "^${PROJECT_SOURCE_DIR}/.git;\\\\.gitignore;Makefile.am;~$;${CPACK_SOURCE_IGNORE_FILES}")

include (CPack)

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

# Change the default build type to something fast
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Determine name of library installation directory, i.e. "lib" vs "lib64", which
# differs between all Debian-based vs all other Linux distros.
# See cmake bug #11964, http://cmake.org/gitweb?p=cmake.git;a=commit;h=126c993d
include(GNUInstallDirs)
message(STATUS "Library installation directory: ${CMAKE_INSTALL_LIBDIR}")

#####################################################################################
# Configure library search paths
#####################################################################################

if (NOT MSVC)
option(SIMGEAR_SHARED   "Set to ON to build SimGear as a shared library/framework" OFF)
option(SYSTEM_EXPAT     "Set to ON to build SimGear using the system expat library" OFF)
option(SYSTEM_UDNS      "Set to ON to build SimGear using the system udns library" OFF)
else()
# Building SimGear DLLs is currently not supported for MSVC.
set(SIMGEAR_SHARED OFF)
# Using external 3rd party libraries is currently not supported for MSVC - it would require shared simgear (DLL).
set(SYSTEM_EXPAT OFF)
set(SYSTEM_UDNS OFF)
endif()

option(SIMGEAR_HEADLESS "Set to ON to build SimGear without GUI/graphics support" OFF)
option(ENABLE_RTI       "Set to ON to build SimGear with RTI support" OFF)
option(ENABLE_GDAL      "Set to ON to build SimGear with GDAL support" OFF)
option(ENABLE_TESTS     "Set to OFF to disable building SimGear's test applications" ON)
option(ENABLE_SOUND     "Set to OFF to disable building SimGear's sound support" ON)
option(USE_AEONWAVE     "Set to ON to use AeonWave instead of OpenAL" ON)
option(ENABLE_PKGUTIL   "Set to ON to build the sg_pkgutil application (default)" ON)
option(ENABLE_DNS       "Set to ON to use udns library and DNS service resolver" ON)
option(ENABLE_SIMD      "Enable SSE/SSE2 support for compilers" ON)
option(ENABLE_SIMD_CODE	"Enable SSE/SSE2 support code for compilers" OFF)
option(ENABLE_OPENMP    "Enable OpenMP compiler support" OFF)

if (NOT ENABLE_SIMD AND ENABLE_SIMD_CODE)
  set(ENABLE_SIMD_CODE OFF)
endif()

include (DetectArch)
include (ExportDebugSymbols)

# until the fstream fix is applied and generally available in OSG,
# keep the compatability link option as the default
option(OSG_FSTREAM_EXPORT_FIXED "Set to ON if the osgDB fstream export patch is applied" OFF)

if (CMAKE_COMPILER_IS_GNUCXX OR CLANG)
  if (CMAKE_VERSION VERSION_LESS 3.1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${CMAKE_CXX_STANDARD}")
  elseif (CMAKE_VERSION VERSION_LESS 3.8)
    # policy CMP0067 (try_compile does not honor CMAKE_CXX_STANDARD)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++${CMAKE_CXX_STANDARD}")
  endif()
endif()


if (MSVC)
  GET_FILENAME_COMPONENT(PARENT_DIR ${PROJECT_BINARY_DIR} DIRECTORY)
  if (CMAKE_CL_64)
    SET(TEST_3RDPARTY_DIR "${PARENT_DIR}/3rdparty.x64")
  else (CMAKE_CL_64)
    SET(TEST_3RDPARTY_DIR "${PARENT_DIR}/3rdparty")
  endif (CMAKE_CL_64)
  if (EXISTS ${TEST_3RDPARTY_DIR})
    set(MSVC_3RDPARTY_ROOT ${PARENT_DIR} CACHE PATH "Location where the third-party dependencies are extracted")
  else (EXISTS ${TEST_3RDPARTY_DIR})
    set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
  endif (EXISTS ${TEST_3RDPARTY_DIR})

  # override CMake default RelWithDebInfo flags. This is important to ensure
  # good performance
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")
else (MSVC)
  set(MSVC_3RDPARTY_ROOT NOT_FOUND CACHE PATH "Location where the third-party dependencies are extracted")
endif (MSVC)

if (MSVC AND MSVC_3RDPARTY_ROOT)
  message(STATUS "3rdparty files located in ${MSVC_3RDPARTY_ROOT}")

  string(SUBSTRING ${MSVC_VERSION} 0 2 MSVC_VERSION_MAJOR)
  string(SUBSTRING ${MSVC_VERSION} 2 2 MSVC_VERSION_MINOR)

  set( OSG_MSVC "msvc" )
  if (${MSVC_VERSION_MAJOR} EQUAL "19")
    if (${MSVC_VERSION_MINOR} GREATER_EQUAL "20")
      set( OSG_MSVC ${OSG_MSVC}142 )
    elseif (${MSVC_VERSION_MINOR} GREATER_EQUAL "10")
      set( OSG_MSVC ${OSG_MSVC}141 )
    else ()
      set( OSG_MSVC ${OSG_MSVC}140 )
    endif ()
  elseif (${MSVC_VERSION_MAJOR} EQUAL "18")
    set( OSG_MSVC ${OSG_MSVC}120 )
  else ()
    message(FATAL_ERROR "Visual Studio 2013 or higher is required")
  endif ()

  if (CMAKE_CL_64)
      set( OSG_MSVC ${OSG_MSVC}-64 )
      set( MSVC_3RDPARTY_DIR 3rdParty.x64 )
  else (CMAKE_CL_64)
      set( MSVC_3RDPARTY_DIR 3rdParty )
  endif (CMAKE_CL_64)

  set (CMAKE_LIBRARY_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/lib ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenRTI/lib )
  set (CMAKE_INCLUDE_PATH ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenScenegraph/include ${MSVC_3RDPARTY_ROOT}/install/${OSG_MSVC}/OpenRTI/include)

  if(NOT BOOST_INCLUDEDIR)
    # if this variable was not set by the user, set it to 3rdparty root's
    # parent dir, which is the normal location for people using our
    # windows-3rd-party repo
    get_filename_component(MSVC_ROOT_PARENT_DIR ${MSVC_3RDPARTY_ROOT} DIRECTORY)
    set(BOOST_INCLUDEDIR ${MSVC_ROOT_PARENT_DIR})
    message(STATUS "BOOST_INCLUDEDIR is ${BOOST_INCLUDEDIR}")
  endif()

  set (OPENAL_INCLUDE_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/include)
  set (OPENAL_LIBRARY_DIR ${MSVC_3RDPARTY_ROOT}/${MSVC_3RDPARTY_DIR}/lib)
endif (MSVC AND MSVC_3RDPARTY_ROOT)

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    find_package(Threads REQUIRED)
endif()

find_package(Boost REQUIRED)
set (BOOST_CXX_FLAGS "-DBOOST_BIMAP_DISABLE_SERIALIZATION -DBOOST_NO_STDLIB_CONFIG")
include(BoostTestTargets)

if(SIMGEAR_HEADLESS)
    message(STATUS "SimGear mode: HEADLESS")
    set(ENABLE_SOUND 0)
else()
    message(STATUS "SimGear mode: NORMAL")
    find_package(OpenGL REQUIRED)

    if (ENABLE_SOUND)
        if (USE_AEONWAVE)
            find_package(AAX)
        endif()

        if(NOT AAX_FOUND)
            set(USE_AEONWAVE FALSE)
            find_package(OpenAL REQUIRED)
        endif()

        if(AAX_FOUND)
            message(STATUS "Sound support: AeonWave")
        else()
            message(STATUS "Sound support: OpenAL")
        endif()
    endif(ENABLE_SOUND)

    find_package(OpenSceneGraph 3.2.0 REQUIRED osgText osgSim osgDB osgParticle osgGA osgViewer osgUtil)

    if (MSVC)
      set(CMAKE_REQUIRED_INCLUDES ${OPENSCENEGRAPH_INCLUDE_DIRS})
      # ensure OSG was compiled with OSG_USE_UTF8_FILENAME set
      check_cxx_source_compiles(
         "#include <osg/Config>
         #if !defined(OSG_USE_UTF8_FILENAME)
          #error OSG UTF8 support not enabled
         #endif
         int main() { return 0; }"
         SIMGEAR_OSG_USE_UTF8_FILENAME)
      if (NOT SIMGEAR_OSG_USE_UTF8_FILENAME)
        message(FATAL_ERROR "Please rebuild OSG with OSG_USE_UTF8_FILENAME set to ON")
      endif()
    endif()
endif(SIMGEAR_HEADLESS)

if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    # As of 2020-08-01, OpenBSD's system zlib is slightly old, but it's usable
    # with a workaround in simgear/io/iostreams/gzfstream.cxx.
    find_package(ZLIB 1.2.3 REQUIRED)
else()
    find_package(ZLIB 1.2.4 REQUIRED)
endif()

find_package(LibLZMA REQUIRED)
find_package(CURL REQUIRED)

if (SYSTEM_EXPAT)
    message(STATUS "Requested to use system Expat library, forcing SIMGEAR_SHARED to true")
    set(SIMGEAR_SHARED ON)
    find_package(EXPAT REQUIRED)

else()
    message(STATUS "Using built-in expat code")
    set(EXPAT_INCLUDE_DIRS
            ${PROJECT_SOURCE_DIR}/3rdparty/expat
            ${PROJECT_BINARY_DIR}/3rdparty/expat)
endif(SYSTEM_EXPAT)

check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(windows.h HAVE_WINDOWS_H)

if(HAVE_INTTYPES_H)
  # ShivaVG needs inttypes.h
  add_definitions(-DHAVE_INTTYPES_H)
endif()

if(ENABLE_RTI)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      SET(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
      pkg_check_modules(RTI hla-rti13)
    endif(PKG_CONFIG_FOUND)
    if(RTI_FOUND)
      message(STATUS "RTI: ENABLED")
    else()
      message(STATUS "RTI: DISABLED")
    endif(RTI_FOUND)
else()
    message(STATUS "RTI: DISABLED")
endif(ENABLE_RTI)

if(ENABLE_GDAL)
    find_package(GDAL 2.0.0 REQUIRED)
    if (GDAL_FOUND)
        include_directories(${GDAL_INCLUDE_DIR})
    endif(GDAL_FOUND)
endif(ENABLE_GDAL)

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(rint HAVE_RINT)
check_function_exists(mkdtemp HAVE_MKDTEMP)
check_function_exists(bcopy HAVE_BCOPY)
check_function_exists(mmap HAVE_MMAP)

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

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually 'd' on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

# isnan might not be real symbol, so can't check using function_exists
check_cxx_source_compiles(
    "#include <cmath>
    int main() { return std::isnan(0.0);} "
    HAVE_STD_ISNAN)

if (NOT ${HAVE_STD_ISNAN})
  message(FATAL_ERROR "Your compiler lacks C++11 std::isnan, please update it")
endif()

# Check if the <regex> implementation in the C++ standard library is usable.
# This is necessary because g++ 4.8 lies about its C++11 compliance: its
# <regex> is utterly unusable, cf. [1].
# The big preprocessor test essentially comes from [2], and gcc upstream devs
# appear to back it (see comments following [2], as well as [3]).
#
#   [1] https://stackoverflow.com/a/12665408/4756009
#   [2] https://stackoverflow.com/a/41186162/4756009
#   [3] https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78905
check_cxx_source_compiles(
    "#include <regex>

     int main() {
       #if __cplusplus >= 201103L &&                             \
           (!defined(__GLIBCXX__)                         ||     \
            (__cplusplus >= 201402L)                      ||     \
            defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) ||     \
            defined(_GLIBCXX_REGEX_STATE_LIMIT)           ||     \
            (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE > 4))
       #else
         nullptr = void; // intentionally trigger a compilation error
       #endif
     }"
    HAVE_WORKING_STD_REGEX)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(WARNING_FLAGS_CXX "-Wall -fPIC")
    set(WARNING_FLAGS_C   "-Wall -fPIC")

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4)
      message(WARNING "GCC 4.4 will be required soon, please upgrade")
    endif()

    if (X86 OR X86_64)
        set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
    endif()

    # certain GCC versions don't provide the atomic builds, and hence
    # require is to provide them in SGAtomic.cxx
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
    check_cxx_source_compiles(
       "int main() { unsigned mValue; return __sync_add_and_fetch(&mValue, 1); }"
        GCC_ATOMIC_BUILTINS_FOUND)

      # override CMake default RelWithDebInfo flags.
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CLANG)
    # Boost redeclares class members
    set(WARNING_FLAGS_CXX "-Wall -fPIC -Wno-overloaded-virtual -Wno-redeclared-class-member")
    set(WARNING_FLAGS_C   "-Wall -fPIC")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    # fix Boost compilation :(
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

    # override CMake default RelWithDebInfo flags.
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")

    set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
endif()

if (ENABLE_OPENMP)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        message(STATUS "OpenMP: ENABLED")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    else()
        message(STATUS "OpenMP: NOT FOUND")
    endif()
else()
    message(STATUS "OpenMP: DISABLED")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# boost goes haywire wrt static asserts
    check_cxx_compiler_flag(-Wno-unused-local-typedefs HAS_NOWARN_UNUSED_TYPEDEFS)
    if(HAS_NOWARN_UNUSED_TYPEDEFS)
        set(WARNING_FLAGS_CXX " ${WARNING_FLAGS_CXX} -Wno-unused-local-typedefs")
    endif()
endif()

if(WIN32)

    if(MINGW)
        add_definitions(-D_WIN32_WINNT=0x501)
    endif()

    if(MSVC)
        set(MSVC_FLAGS "-DWIN32 -DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS /MP")
        if (X86)
          set(SIMD_COMPILER_FLAGS "/arch:SSE /arch:SSE2")
        endif()

        if (NOT OSG_FSTREAM_EXPORT_FIXED)
          message(STATUS "For better linking performance, use OSG with patched fstream header")
          # needed to avoid link errors on multiply-defined standard C++
          # symbols. Suspect this may be an OSG-DB export bug
            set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
        endif ()

        if (${MSVC_VERSION} GREATER 1899)
            # needed for debug builds with VS2015
            set( MSVC_FLAGS "${MSVC_FLAGS} /bigobj" )
        endif()
    endif(MSVC)

    # assumed on Windows
    set(HAVE_GETLOCALTIME 1)

    set( WINSOCK_LIBRARY "ws2_32.lib" )
    set( SHLWAPI_LIBRARY "Shlwapi.lib" )
    set( RT_LIBRARY "winmm" )
endif(WIN32)

# append the SIMD flags if requested
if (ENABLE_SIMD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_COMPILER_FLAGS}")

  # set for multi-configuration generators
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")

  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

include(CheckCXXFeatures)

# use BEFORE to ensure local directories are used first,
# ahead of system-installed libs
include_directories(BEFORE ${PROJECT_BINARY_DIR}/simgear)

if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    include_directories("/usr/X11R6/include")
endif()

add_definitions(-DHAVE_CONFIG_H)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/simgear/simgear_config_cmake.h.in"
  "${PROJECT_BINARY_DIR}/simgear/simgear_config.h"
  )

if(ENABLE_TESTS)
    # enable CTest / make test target
    message(STATUS "Tests: ENABLED")

    include (Dart)
    enable_testing()
else()
    message(STATUS "Tests: DISABLED")
endif(ENABLE_TESTS)

# always set TEST_LIBS as it is also used by other tools/applications
set(TEST_LIBS_INTERNAL_CORE
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARY}
    ${WINSOCK_LIBRARY}
    ${SHLWAPI_LIBRARY}
    ${RT_LIBRARY}
    ${DL_LIBRARY}
    ${COCOA_LIBRARY}
    ${CURL_LIBRARIES}
    ${GDAL_LIBRARY})
set(TEST_LIBS SimGearCore ${TEST_LIBS_INTERNAL_CORE})

if(NOT SIMGEAR_HEADLESS)
    set(TEST_LIBS SimGearScene ${OPENGL_LIBRARIES} ${TEST_LIBS})
endif()

install (FILES ${PROJECT_BINARY_DIR}/simgear/simgear_config.h  DESTINATION include/simgear/)

if(ENABLE_DNS)
    if(SYSTEM_UDNS)
        message(STATUS "Requested to use system udns library, forcing SIMGEAR_SHARED to true")
        set(SIMGEAR_SHARED ON)
        find_package(Udns REQUIRED)
    else()
        message(STATUS "DNS resolver: ENABLED")
        include_directories(3rdparty/udns)
    endif()
else()
    message(STATUS "DNS resolver: DISABLED")
endif()


add_subdirectory(3rdparty)
add_subdirectory(simgear)

#-----------------------------------------------------------------------------
### Export stuff, see https://cmake.org/cmake/help/v3.2/manual/cmake-packages.7.html#creating-packages
#-----------------------------------------------------------------------------

generate_export_header(SimGearCore)
if(NOT SIMGEAR_HEADLESS)
  generate_export_header(SimGearScene)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SimGear/SimGearConfigVersion.cmake"
  VERSION ${SIMGEAR_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_file(SimGearConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/SimGear/SimGearConfig.cmake"
  @ONLY
)

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/SimGear)
install(EXPORT SimGearTargets
  DESTINATION ${ConfigPackageLocation}
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SimGear/SimGearConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SimGear/SimGearConfigVersion.cmake"
  DESTINATION ${ConfigPackageLocation}
  COMPONENT Devel
)

#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
