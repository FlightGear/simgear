
if(NOT WIN32)
check_include_file(iconv.h HAVE_ICONV_H)
endif()
check_include_file(locale.h HAVE_LOCALE_H)

configure_file (
    "${PROJECT_SOURCE_DIR}/3rdparty/zeroxml/zeroxml_config_cmake.in"
    "${PROJECT_BINARY_DIR}/3rdparty/zeroxml/simgear_zeroxml_config.h"
)

set(zeroxml_sources 
	api.h
	types.h
	xml_cache.h
	xml.h
	localize.c
	xml_cache.c
	xml.c
	)

add_library(ZeroXML STATIC ${zeroxml_sources})

target_include_directories(ZeroXML PRIVATE ${PROJECT_BINARY_DIR}/3rdparty/zeroxml)

# look like Expat found by find()
add_library(ZeroXML::ZeroXML ALIAS ZeroXML)

# in the non-shared case, we need to export these as well
if (NOT SIMGEAR_SHARED)
    install(TARGETS ZeroXML
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
