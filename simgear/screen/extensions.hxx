#ifndef __SG_EXTENSIONS_HXX
#define __SG_EXTENSIONS_HXX 1

#ifndef WIN32
#include <dlfcn.h>
#endif

inline void (*SGLookupFunction(const char *func))() {

#if defined( WIN32 )
        return (void (*)()) wglGetProcAddress(func);

#else

  // If the target system s UNIX and the ARB_get_proc_address
  // GLX extension is *not* guaranteed to be supported. An alternative
  // dlsym-based approach will be used instead.
  #if defined( linux ) || defined ( sgi )
        void *libHandle;
        void (*fptr)();
        libHandle = dlopen("libGL.so", RTLD_LAZY);
        fptr = (void (*)()) dlsym(libHandle, func);
        dlclose(libHandle);
        return fptr;
  #else
        return glXGetProcAddressARB(func);
  #endif
#endif
}

#endif
