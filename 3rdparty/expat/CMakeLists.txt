
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

add_library(FGExpat STATIC ${expat_sources})

target_include_directories(FGExpat BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# for the config file above
target_include_directories(FGExpat PRIVATE ${PROJECT_BINARY_DIR}/3rdparty/expat)

target_compile_definitions(FGExpat PUBLIC HAVE_SIMGEAR_EXPAT_CONFIG_H XML_STATIC)

# look like Expat found by find()
add_library(EXPAT::EXPAT ALIAS FGExpat)

# in the non-shared case, we need to export these as well
if (NOT SIMGEAR_SHARED)
    install(TARGETS FGExpat
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
