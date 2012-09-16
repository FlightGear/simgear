
include (SimGearComponent)


set(HEADERS 
    easyxml.hxx
    )
    
set(SOURCES 
    easyxml.cxx
    )

if (NOT SYSTEM_EXPAT)
    list(APPEND SOURCES
        asciitab.h
        hashtable.h
        iasciitab.h
        latin1tab.h
        nametab.h
        utf8tab.h
        xmldef.h
        xmlparse.h
        xmlrole.h
        xmltok.h
        xmltok_impl.h
        hashtable.c
        xmlparse.c
        xmlrole.c
        xmltok.c
        internal.h
        ascii.h
        expat.h
        expat_external.h 
    )
endif()
    
simgear_component(xml xml "${SOURCES}" "${HEADERS}")
