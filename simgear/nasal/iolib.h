#ifndef _IOLIB_H
#define _IOLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include "nasal.h"

// Note use of 32 bit ints, should fix at some point to use
// platform-dependent fpos_t/size_t or just punt and use int64_t
// everywhere...

// The naContext is passed in for error reporting via
// naRuntimeError().
struct naIOType {
    void (*close)(naContext c, void* f);
    int  (*read) (naContext c, void* f, char* buf, unsigned int len);
    int  (*write)(naContext c, void* f, char* buf, unsigned int len);
    void (*seek) (naContext c, void* f, unsigned int off, int whence);
    int  (*tell) (naContext c, void* f);
    void (*flush) (naContext c, void* f);
    void (*destroy)(void* f);
};

struct naIOGhost {
    struct naIOType* type;
    void* handle; // descriptor, FILE*, HANDLE, etc...
};

extern naGhostType naIOGhostType;
extern struct naIOType naStdIOType;

#define IOGHOST(r) ((struct naIOGhost*)naGhost_ptr(r))
#define IS_IO(r) (IS_GHOST(r) && naGhost_type(r) == &naIOGhostType)
#define IS_STDIO(r) (IS_IO(r) && (IOGHOST(r)->type == &naStdIOType))

// Defined in iolib.c, there is no "library" header to put this in
naRef naIOGhost(naContext c, FILE* f);
#ifdef __cplusplus
} // extern "C"
#endif
#endif // _IOLIB_H
