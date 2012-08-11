include (SimGearComponent)

set(HEADERS
    terrasync.hxx
    )

set(SOURCES 
    terrasync.cxx
    )

if(LIBSVN_FOUND)
	add_definitions(${APR_CFLAGS})
	include_directories(${LIBSVN_INCLUDE_DIR})
endif()

simgear_component(tsync scene/tsync "${SOURCES}" "${HEADERS}")
