
configure_file (
    "${PROJECT_SOURCE_DIR}/3rdparty/expat/expat_config_cmake.in"
    "${PROJECT_BINARY_DIR}/3rdparty/expat/simgear_expat_config.h"
)

set(expat_sources 
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
        sg_expat.h
        sg_expat_external.h 
	)

foreach(s ${expat_sources})
	set_property(GLOBAL
		APPEND PROPERTY LOCAL_EXPAT_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
endforeach()
