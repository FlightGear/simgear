#ifndef _NASAL_H
#define _NASAL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "naref.h"

typedef struct Context* naContext;
    
// The function signature for an extension function:
typedef naRef (*naCFunction)(naContext ctx, naRef me, int argc, naRef* args);

// All Nasal code runs under the watch of a naContext:
naContext naNewContext();
void naFreeContext(naContext c);

// Use this when making a call to a new context "underneath" a
// preexisting context on the same stack.  It allows stack walking to
// see through the boundary, and eliminates the need to release the
// mod lock (i.e. must be called with the mod lock held!)
naContext naSubContext(naContext super);

// The naContext supports a user data pointer that can be used to
// store data specific to an naCall invocation without exposing it to
// Nasal as a ghost.  FIXME: this API is semi-dangerous, there is no
// provision for sharing it, nor for validating the source or type of
// the pointer returned.
void naSetUserData(naContext c, void* p);
void* naGetUserData(naContext c);

// "Save" this object in the context, preventing it (and objects
// referenced by it) from being garbage collected.
void naSave(naContext ctx, naRef obj);

// Similar, but the object is automatically released when the
// context next runs native bytecode.  Useful for saving off C-space
// temporaries to protect them before passing back into a naCall.
void naTempSave(naContext c, naRef r);

// Parse a buffer in memory into a code object.  The srcFile parameter
// is a Nasal string representing the "file" from which the code is
// read.  The "first line" is typically 1, but is settable for
// situations where the Nasal code is embedded in another context with
// its own numbering convetions.  If an error occurs, returns nil and
// sets the errLine pointer to point to the line at fault.  The string
// representation of the error can be retrieved with naGetError() on
// the context.
naRef naParseCode(naContext c, naRef srcFile, int firstLine,
                  char* buf, int len, int* errLine);

// Binds a bare code object (as returned from naParseCode) with a
// closure object (a hash) to act as the outer scope / namespace.
naRef naBindFunction(naContext ctx, naRef code, naRef closure);

// Similar, but it binds to the current context's closure (i.e. the
// namespace at the top of the current call stack).
naRef naBindToContext(naContext ctx, naRef code);

// Call a code or function object with the specified arguments "on"
// the specified object and using the specified hash for the local
// variables.  Passing a null args array skips the parameter variables
// (e.g. "arg") assignments; to get a zero-length arg instead, pass in
// argc==0 and a non-null args vector.  The obj or locals parameters
// may be nil.  Will attempt to acquire the mod lock, so call
// naModUnlock() first if the lock is already held.
naRef naCall(naContext ctx, naRef func, int argc, naRef* args,
             naRef obj, naRef locals);

// As naCall(), but continues execution at the operation after a
// previous die() call or runtime error.  Useful to do "yield"
// semantics, leaving the context in a condition where it can be
// restarted from C code.  Cannot be used currently to restart a
// failed operation.  Will attempt to acquire the mod lock, so call
// naModUnlock() first if the lock is already held.
naRef naContinue(naContext ctx);

// Throw an error from the current call stack.  This function makes a
// longjmp call to a handler in naCall() and DOES NOT RETURN.  It is
// intended for use in library code that cannot otherwise report an
// error via the return value, and MUST be used carefully.  If in
// doubt, return naNil() as your error condition.  Works like
// printf().
void naRuntimeError(naContext c, const char* fmt, ...);

// "Re-throws" a runtime error caught from the subcontext.  Acts as a
// naRuntimeError() called on the parent context.  Does not return.
void naRethrowError(naContext subc);

// Retrieve the specified member from the object, respecting the
// "parents" array as for "object.field".  Returns zero for missing
// fields.
int naMember_get(naRef obj, naRef field, naRef* out);
int naMember_cget(naRef obj, const char* field, naRef* out);

// Returns a hash containing functions from the Nasal standard library
// Useful for passing as a namespace to an initial function call
naRef naInit_std(naContext c);

// Ditto, for other core libraries
naRef naInit_math(naContext c);
naRef naInit_bits(naContext c);
naRef naInit_io(naContext c);
naRef naInit_regex(naContext c);
naRef naInit_unix(naContext c);
naRef naInit_thread(naContext c);
naRef naInit_utf8(naContext c);
naRef naInit_sqlite(naContext c);
naRef naInit_readline(naContext c);
naRef naInit_gtk(naContext ctx);
naRef naInit_cairo(naContext ctx);

// Context stack inspection, frame zero is the "top"
int naStackDepth(naContext ctx);
int naGetLine(naContext ctx, int frame);
naRef naGetSourceFile(naContext ctx, int frame);
char* naGetError(naContext ctx);

// Type predicates
int naIsNil(naRef r);
int naIsNum(naRef r);
int naIsString(naRef r);
int naIsScalar(naRef r);
int naIsVector(naRef r);
int naIsHash(naRef r);
int naIsCode(naRef r);
int naIsFunc(naRef r);
int naIsCCode(naRef r);

// Allocators/generators:
naRef naNil();
naRef naNum(double num);
naRef naNewString(naContext c);
naRef naNewVector(naContext c);
naRef naNewHash(naContext c);
naRef naNewFunc(naContext c, naRef code);
naRef naNewCCode(naContext c, naCFunction fptr);

// Some useful conversion/comparison routines
int naEqual(naRef a, naRef b);
int naStrEqual(naRef a, naRef b);
int naTrue(naRef b);
naRef naNumValue(naRef n);
naRef naStringValue(naContext c, naRef n);

// String utilities:
int naStr_len(naRef s);
char* naStr_data(naRef s);
naRef naStr_fromdata(naRef dst, char* data, int len);
naRef naStr_concat(naRef dest, naRef s1, naRef s2);
naRef naStr_substr(naRef dest, naRef str, int start, int len);
naRef naInternSymbol(naRef sym);

// Vector utilities:
int naVec_size(naRef v);
naRef naVec_get(naRef v, int i);
void naVec_set(naRef vec, int i, naRef o);
int naVec_append(naRef vec, naRef o);
naRef naVec_removelast(naRef vec);
void naVec_setsize(naRef vec, int sz);

// Hash utilities:
int naHash_size(naRef h);
int naHash_get(naRef hash, naRef key, naRef* out);
naRef naHash_cget(naRef hash, char* key);
void naHash_set(naRef hash, naRef key, naRef val);
void naHash_cset(naRef hash, char* key, naRef val);
void naHash_delete(naRef hash, naRef key);
void naHash_keys(naRef dst, naRef hash);

// Ghost utilities:
typedef struct naGhostType {
    void (*destroy)(void* ghost);
    const char* name;
} naGhostType;
naRef        naNewGhost(naContext c, naGhostType* t, void* ghost);
naGhostType* naGhost_type(naRef ghost);
void*        naGhost_ptr(naRef ghost);
int          naIsGhost(naRef r);

// Acquires a "modification lock" on a context, allowing the C code to
// modify Nasal data without fear that such data may be "lost" by the
// garbage collector (nasal data the C stack is not examined in GC!).
// This disallows garbage collection until the current thread can be
// blocked.  The lock should be acquired whenever nasal objects are
// being modified.  It need not be acquired when only read access is
// needed, PRESUMING that the Nasal data being read is findable by the
// collector (via naSave, for example) and that another Nasal thread
// cannot or will not delete the reference to the data.  It MUST NOT
// be acquired by naCFunction's, as those are called with the lock
// already held; acquiring two locks for the same thread will cause a
// deadlock when the GC is invoked.  It should be UNLOCKED by
// naCFunction's when they are about to do any long term non-nasal
// processing and/or blocking I/O.  Note that naModLock() may need to
// block to allow garbage collection to occur, and that garbage
// collection by other threads may be blocked until naModUnlock() is
// called.  It must also be UNLOCKED by threads that hold a lock
// already before making a naCall() or naContinue() call -- these
// functions will attempt to acquire the lock again.
void naModLock();
void naModUnlock();

// Library utilities.  Generate namespaces and add symbols.
typedef struct { char* name; naCFunction func; } naCFuncItem;
naRef naGenLib(naContext c, naCFuncItem *funcs);
void naAddSym(naContext c, naRef ns, char *sym, naRef val);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _NASAL_H
