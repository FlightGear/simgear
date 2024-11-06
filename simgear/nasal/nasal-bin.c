// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2003  Andy Ross  andy@plausible.org

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
//#include <unistd.h>
//#include <pthread.h>
#endif

#include "nasal.h"
#include "data.h"

void checkError(naContext ctx)
{
    int i;
    if(naGetError(ctx)) {
        fprintf(stderr, "Runtime error: %s\n  at %s, line %d\n",
                naGetError(ctx), naStr_data(naGetSourceFile(ctx, 0)),
                naGetLine(ctx, 0));

        for(i=1; i<naStackDepth(ctx); i++)
            fprintf(stderr, "  called from: %s, line %d\n",
                    naStr_data(naGetSourceFile(ctx, i)),
                    naGetLine(ctx, i));
        exit(1);
    }
}

// A Nasal extension function (prints its argument list to stdout)
static naRef print(naContext c, naRef me, int argc, naRef* args)
{
    int i;
    for(i=0; i<argc; i++) {
        naRef s = naStringValue(c, args[i]);
        if(naIsNil(s)) continue;
        fwrite(naStr_data(s), 1, naStr_len(s), stdout);
    }
    return naNil();
}

#define MAX_PATH_LEN 1024
#define NASTR(s) naStr_fromdata(naNewString(ctx), (s), strlen((s)))
int main(int argc, char** argv)
{
    naRef code, namespace, result, *args;
    char path[MAX_PATH_LEN];
    struct Context *ctx;
    char *buf, *script;
    struct stat fdat;
    int errLine, i;
    FILE* f;

    if(argc < 2) {
        fprintf(stderr, "nasal: must specify a script to run\n");
        exit(1);
    }
    script = argv[1];

    // Read the contents of the file into a buffer in memory.
    f = fopen(script, "rb");
    if(!f) {
        snprintf(path, MAX_PATH_LEN-1, SOURCE_DIR"/misc/%s", script);
        f = fopen(path, "rb");
        if(!f) {
            fprintf(stderr, "nasal: could not open input file: %s\n", path);
            exit(1);
        }
        script = path;
    }
    stat(script, &fdat);
    buf = malloc(fdat.st_size);
    if(fread(buf, 1, fdat.st_size, f) != fdat.st_size) {
        fprintf(stderr, "nasal: error in fread()\n");
        exit(1);
    }

    // Create an interpreter context
    ctx = naNewContext();

    // Parse the code in the buffer.  The line of a fatal parse error
    // is returned via the pointer.
    code = naParseCode(ctx, NASTR(script), 1, buf, fdat.st_size, &errLine);
    if(naIsNil(code)) {
        fprintf(stderr, "Parse error: %s at line %d\n",
                naGetError(ctx), errLine);
        exit(1);
    }
    free(buf);

    // Make a hash containing the standard library functions.  This
    // will be the namespace for a new script
    namespace = naInit_std(ctx);

    // Add application-specific functions (in this case, "print" and
    // the math library) to the namespace if desired.
    naAddSym(ctx, namespace, "print", naNewFunc(ctx, naNewCCode(ctx, print)));

    // Add extra libraries as needed.
    naAddSym(ctx, namespace, "utf8", naInit_utf8(ctx));
    naAddSym(ctx, namespace, "math", naInit_math(ctx));
    naAddSym(ctx, namespace, "bits", naInit_bits(ctx));
    naAddSym(ctx, namespace, "io", naInit_io(ctx));
#ifndef _WIN32
    naAddSym(ctx, namespace, "unix", naInit_unix(ctx));
#endif
    naAddSym(ctx, namespace, "thread", naInit_thread(ctx));
#ifdef HAVE_PCRE
    naAddSym(ctx, namespace, "regex", naInit_regex(ctx));
#endif
#ifdef HAVE_SQLITE
    naAddSym(ctx, namespace, "sqlite", naInit_sqlite(ctx));
#endif
#ifdef HAVE_READLINE
    naAddSym(ctx, namespace, "readline", naInit_readline(ctx));
#endif
#ifdef HAVE_GTK
    // Gtk goes in as "_gtk" -- there is a higher level wrapper module
    naAddSym(ctx, namespace, "_gtk", naInit_gtk(ctx));
    naAddSym(ctx, namespace, "cairo", naInit_cairo(ctx));
#endif

    // Bind the "code" object from naParseCode into a "function"
    // object.  This is optional, really -- we could also just pass
    // the namespace hash as the final argument to naCall().  But
    // having the global namespace in an outer scope means that we
    // won't be polluting it with the local variables of the script.
    code = naBindFunction(ctx, code, namespace);

    // Build the arg vector
    args = malloc(sizeof(naRef) * (argc-2));
    for(i=0; i<argc-2; i++)
        args[i] = NASTR(argv[i+2]);

    // Run it.
    result = naCall(ctx, code, argc-2, args, naNil(), naNil());
    free(args);

#if 0
    // Test for naContinue() feature.  Keep trying until it reports
    // success.
    while(naGetError(ctx)) {
        printf("Err: \"%s\", calling naContinue()...\n", naGetError(ctx));
        naContinue(ctx);
    }
#endif

    checkError(ctx);
    return 0;
}
#undef NASTR
