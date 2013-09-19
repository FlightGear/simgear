

configure_file (
    "${PROJECT_SOURCE_DIR}/3rdparty/expat/expat_config_cmake.in"
    "${PROJECT_BINARY_DIR}/3rdparty/expat/expat_config.h"
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


# this code will be linked into SimGearCore shared object, so it 
# must be compiled with -fPIC too


if(SIMGEAR_SHARED AND UNIX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(local_expat STATIC ${expat_sources})

