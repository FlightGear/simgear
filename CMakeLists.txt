cmake_minimum_required (VERSION 3.10)

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

if(POLICY CMP0093)
  cmake_policy(SET CMP0093 NEW)
endif()

if(POLICY CMP0145)
  cmake_policy(SET CMP0145 NEW)
endif()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
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
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_CXX_STANDARD 17)
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


# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

include(Packaging)

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
option(ENABLE_CYCLONE   "Set to ON to build SimGear with Cyclone Data Distribution Service support" ON)
option(ENABLE_RTI       "Set to ON to build SimGear with RTI support" OFF)
option(ENABLE_GDAL      "Set to ON to build SimGear with GDAL support" OFF)
option(ENABLE_TESTS     "Set to OFF to disable building SimGear's test applications" ON)
option(ENABLE_SOUND     "Set to OFF to disable building SimGear's sound support" ON)
option(USE_SHADERVG     "Set to ON to use SHaderVG instead of ShivaVG" OFF)
option(USE_AEONWAVE     "Set to ON to use AeonWave instead of OpenAL" ON)
option(USE_OPENALSOFT   "Set to ON to use OpenAL from OpenAL-soft" OFF)

option(ENABLE_PKGUTIL   "Set to ON to build the sg_pkgutil application (default)" ON)
option(ENABLE_SIMD      "Enable SSE/SSE2 support for compilers" ON)
option(ENABLE_ASAN      "Set to ON to build SimGear with LLVM AddressSanitizer (ASan) support.  Incompatile with ENABLE_TSAN." OFF)
option(ENABLE_TSAN      "Set to ON to build SimGear with LLVM ThreadSanitizer (TSan) support. Incompatile with ENABLE_ASAN." OFF)
option(ENABLE_VIDEO_RECORD "Set to ON to build SimGear with video recording" ON)

if (USE_SHADERVG)
  set(OPENVG ShaderVG)
else()
  set(OPENVG ShivaVG)
endif()
message(STATUS "OpenVG: ${OPENVG}")

include (DetectArch)
include (ExportDebugSymbols)

# until the fstream fix is applied and generally available in OSG,
# keep the compatability link option as the default
option(OSG_FSTREAM_EXPORT_FIXED "Set to ON if the osgDB fstream export patch is applied" OFF)

if (MSVC)
  # override CMake default RelWithDebInfo flags. This is important to ensure
  # good performance
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Zi /O2 /Ob2 /D NDEBUG")
endif ()

# Setup MSVC 3rd party directories
include( ConfigureMsvc3rdParty )

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
endif()

find_package(Threads REQUIRED)

find_package(Boost REQUIRED)
set (BOOST_CXX_FLAGS "-DBOOST_BIMAP_DISABLE_SERIALIZATION -DBOOST_NO_STDLIB_CONFIG -DBOOST_NO_AUTO_PTR -DBOOST_NO_CXX98_BINDERS")
include(BoostTestTargets)

set(SG_SOUND_USES_OPENALSOFT OFF) # default to off

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
            if (USE_OPENALSOFT)
                message(STATUS "Sound requested to use OpenAL-soft: ensure Config.cmake files are in CMAKE_PREFIX_PATH")
               find_package(OpenAL REQUIRED CONFIG)
               if (TARGET OpenAL::OpenAL)
                set(SG_SOUND_USES_OPENALSOFT ON)
               endif()
            else()
              # regular OpenAL is found via the CMake distro-suppling FindOpenAL
              find_package(OpenAL REQUIRED)
            endif()

            message(STATUS "OpenAL: ${OPENAL_LIBRARY}")
        endif()

        if(AAX_FOUND)
            message(STATUS "Sound support: AeonWave")
        elseif (TARGET OpenAL::OpenAL)
           message(STATUS "Sound support: OpenAL-soft")
        else()
            message(STATUS "Sound support: OpenAL (from system)")
        endif()
    endif(ENABLE_SOUND)

    include(CreateOSGImportedTargets)
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
endif(SYSTEM_EXPAT)

if(ENABLE_RTI)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      SET(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
      pkg_check_modules(RTI IMPORTED_TARGET hla-rti13)
    endif(PKG_CONFIG_FOUND)
    if(RTI_FOUND)
      message(STATUS "RTI: ENABLED")
    else()
      message(STATUS "RTI: DISABLED")
    endif(RTI_FOUND)
else()
    message(STATUS "RTI: DISABLED")
endif(ENABLE_RTI)

if(ENABLE_CYCLONE)
    find_package(CycloneDDS QUIET)
    if (CycloneDDS_FOUND)
        message(STATUS "Data Distribution Service support: CycloneDDS")
        set(SG_HAVE_DDS 1)
    else(CycloneDDS_FOUND)
        message(STATUS "Data Distribution Service support: DISABLED")
    endif(CycloneDDS_FOUND)
endif(ENABLE_CYCLONE)

if(ENABLE_GDAL)
    find_package(GDAL 2.0.0 REQUIRED)
endif(ENABLE_GDAL)


if (ENABLE_VIDEO_RECORD)
  find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE)
  if (FFmpeg_FOUND)
  # set this so it's picked up in simgear_config.h
    set(SG_FFMPEG 1)
  else()
    message(STATUS "Video recording was requested, but required FFmpeg/avcodec libaries not found")
  endif()
endif()

include(CheckPOSIXFeatures)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually 'd' on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

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

    if (X86 OR X86_64)
        set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
    endif()

    # certain GCC versions don't provide the atomic builds, and hence
    # require is to provide them in SGAtomic.cxx
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
    check_cxx_source_compiles(
       "int main() { unsigned mValue; return __sync_add_and_fetch(&mValue, 1); }"
        GCC_ATOMIC_BUILTINS_FOUND)

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CLANG)
    message(STATUS "CMAKE_OSX_ARCHITECTURES has value: ${CMAKE_OSX_ARCHITECTURES}")

    # Boost redeclares class members
    # OSG doesn't use override but we subclass from it, so we will
    # have inconsistent overrides until it's fixed
    set(WARNING_FLAGS_CXX "-Wall -fPIC -Wno-overloaded-virtual -Wno-redeclared-class-member -Wno-inconsistent-missing-override")
    set(WARNING_FLAGS_C   "-Wall -fPIC")

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "-O3 -g -DNDEBUG")

    # special case for macOS cross-compilation: use the values in CMAKE_OSX_ARCHITECTURES,
    # rather than the CMake properties which reflect the host system.
    if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
      set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse")
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
      # set(SIMD_COMPILER_FLAGS "-mfpmath=vfp4")
    elseif(X86 OR X86_64)
      set(SIMD_COMPILER_FLAGS "-msse2 -mfpmath=sse")
    endif()
    set(SIMD_COMPILER_FLAGS "${SIMD_COMPILER_FLAGS} -ftree-vectorize -ftree-slp-vectorize")
endif()

if (ENABLE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")

  # needed for check_cxx_source_compiles
  set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
  if (ENABLE_TSAN)
    message(FATAL_ERROR  "ENABLE_ASAN=ON incompatible with ENABLE_TSAN=ON")
  endif()
else ()
  if (ENABLE_TSAN)
    message(WARNING  "ThreadSanitizer (ENABLE_TSAN=ON) has a performance impact.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")

    # needed for check_cxx_source_compiles
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=thread")
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# boost goes haywire wrt static asserts
    check_cxx_compiler_flag(-Wno-unused-local-typedefs HAS_NOWARN_UNUSED_TYPEDEFS)
    if(HAS_NOWARN_UNUSED_TYPEDEFS)
        set(WARNING_FLAGS_CXX " ${WARNING_FLAGS_CXX} -Wno-unused-local-typedefs")
    endif()
    if (CLANG)
        check_cxx_compiler_flag(-Wno-inconsistent-missing-override HAS_NOWARN_MISSING_OVERRIDE)
        if(HAS_NOWARN_MISSING_OVERRIDE)
            set(WARNING_FLAGS_CXX " ${WARNING_FLAGS_CXX} -Wno-inconsistent-missing-override")
        endif()
    endif()
endif()

if(WIN32)

    if(MINGW)
        add_definitions(-D_WIN32_WINNT=0x501)
    endif()

    if(MSVC)
        set(MSVC_FLAGS "-DWIN32 -DNOMINMAX -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS /MP /bigobj")
        if (X86)
          set(SIMD_COMPILER_FLAGS "/arch:SSE /arch:SSE2")
        endif()

        if (NOT OSG_FSTREAM_EXPORT_FIXED)
          message(STATUS "For better linking performance, use OSG with patched fstream header")
          # needed to avoid link errors on multiply-defined standard C++
          # symbols. Suspect this may be an OSG-DB export bug
            set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
        endif ()
    endif(MSVC)

    # assumed on Windows
    set(HAVE_GETLOCALTIME 1)

    set( WINSOCK_LIBRARY "ws2_32.lib" )
    set( SHLWAPI_LIBRARY "Shlwapi.lib" )
    set( RT_LIBRARY "winmm" )
endif(WIN32)

message(STATUS "SIMD flags are ${SIMD_COMPILER_FLAGS}")

# append flags, but ensure we don't duplicate them for multi-config generators
if (CMAKE_CONFIGURATION_TYPES)
  if (ENABLE_SIMD)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SIMD_COMPILER_FLAGS}")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SIMD_COMPILER_FLAGS}")
  endif()
else()
  if (ENABLE_SIMD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIMD_COMPILER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_COMPILER_FLAGS}")
  endif()
endif()

# TODO: should this be special-cased for multi-config as well?
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

include(CheckCXXFeatures)

add_definitions(-DHAVE_CONFIG_H)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/simgear/simgear_config_cmake.h.in"
  "${PROJECT_BINARY_DIR}/simgear/simgear_config.h"
  )

enable_testing()
include(AddSimGearTest) # helper to setup a test

install (FILES ${PROJECT_BINARY_DIR}/simgear/simgear_config.h  DESTINATION include/simgear/)

if(SYSTEM_UDNS)
    message(STATUS "Requested to use system udns library, forcing SIMGEAR_SHARED to true")
    set(SIMGEAR_SHARED ON)
    find_package(Udns REQUIRED)
endif()

add_subdirectory(3rdparty)
add_subdirectory(simgear)

#-----------------------------------------------------------------------------
### Export stuff, see https://cmake.org/cmake/help/v3.2/manual/cmake-packages.7.html#creating-packages
#-----------------------------------------------------------------------------

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/SimGear)

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

configure_package_config_file(
  SimGearConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/SimGear/SimGearConfig.cmake"
  INSTALL_DESTINATION ${ConfigPackageLocation}
)

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

install (FILES CMakeModules/CreateOSGImportedTargets.cmake  DESTINATION ${ConfigPackageLocation})


#-----------------------------------------------------------------------------
### uninstall target
#-----------------------------------------------------------------------------
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
