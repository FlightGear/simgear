
#ifndef __SHCONFIG_H
#define __SHCONFIG_H

////////////////////////////////////////////////////////////
// Identify the operating system
////////////////////////////////////////////////////////////
#if defined(_WIN32) || defined(__WIN32__)

    // Windows
    #define VG_API_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

#elif defined(linux) || defined(__linux)

    // Linux
    #define VG_API_LINUX

#elif defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh)

    // MacOS
    #define VG_API_MACOSX

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)

    // FreeBSD
    #define VG_API_FREEBSD

#elif defined(__OpenBSD__)

    // FreeBSD
    #define VG_API_OPENBSD

#else

    // Unsupported system
    #error This operating system is not supported by SFML library

#endif

// We currently do not support using images (inside paths). If we were going to
// use it loading and unloading needs to happen within OpenSceneGraph to handle
// synchronization correctly in multithreading mode.
#define SH_NO_IMAGE

#endif // __SHCONFIG_H
