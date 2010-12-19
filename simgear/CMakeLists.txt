
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

foreach( mylibfolder 
        bucket
        debug
        ephemeris
        io
        magvar
        math
        misc
        nasal
        props
        route
        serial
        structure
        threads
        timing
        xml
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if (NOT SIMGEAR_HEADLESS)
    add_subdirectory(environment)
    add_subdirectory(screen)
    add_subdirectory(scene)
    add_subdirectory(sound)
endif()



set(HEADERS compiler.h constants.h sg_inlines.h ${PROJECT_BINARY_DIR}/simgear/version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

if(SIMGEAR_SHARED)
    message(STATUS "building shared library")
    get_property(allSources GLOBAL PROPERTY ALL_SOURCES)
	get_property(publicHeaders GLOBAL PROPERTY PUBLIC_HEADERS)

    add_library(SimGear SHARED ${allSources})
    set_property(TARGET SimGear PROPERTY FRAMEWORK 1)
	message(STATUS "public header: ${publicHeaders}")
	set_property(TARGET SimGear PROPERTY PUBLIC_HEADER "${publicHeaders}")
	
    target_link_libraries(SimGear ${ZLIB_LIBRARY} 
			${OPENSCENEGRAPH_LIBRARIES} 
			${OPENAL_LIBRARY} ${ALUT_LIBRARY}
			${OPENGL_LIBRARY})
			
	install()
endif(SIMGEAR_SHARED)
    