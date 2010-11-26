
include (SimGearComponent)

set(HEADERS 
    naref.h
    nasal.h
    )

set(SOURCES 
    bitslib.c
    code.c
    codegen.c
    gc.c
    hash.c
    iolib.c
    lex.c
    lib.c
    mathlib.c
    misc.c
    parse.c
    string.c
    thread-posix.c
    thread-win32.c
    threadlib.c
    utf8lib.c
    vector.c
    code.h
    data.h
    iolib.h
    parse.h
    )

simgear_component(nasal nasal "${SOURCES}" "${HEADERS}")