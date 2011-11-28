include (SimGearComponent)

set(HEADERS
    terrasync.hxx
    )

set(SOURCES 
    terrasync.cxx
    )

if(LIBSVN_FOUND)
	add_definitions(${APR_CFLAGS})

	IF(APPLE)
		set_property(SOURCE terrasync.cxx PROPERTY COMPILE_FLAGS "-iwithsysroot ${LIBSVN_INCLUDE_DIR}")
	ELSE()
		include_directories(${LIBSVN_INCLUDE_DIR})
	ENDIF(APPLE)

endif()

simgear_scene_component(tsync scene/tsync "${SOURCES}" "${HEADERS}")
