
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



set(HEADERS compiler.h constants.h sg_inlines.h version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

if(SIMGEAR_SHARED)
    message(STATUS "building shared library")
    get_property(allSources DIRECTORY PROPERTY ALL_SOURCES)
	get_property(publicHeaders DIRECTORY PROPERTY PUBLIC_HEADERS)

    add_library(SimGear SHARED ${allSources})
    set_property(TARGET SimGear PROPERTY FRAMEWORK 1)
	set_property(TARGET SimGear PROPERTY PUBLIC_HEADERS ${publicHeaders})
	
    target_link_libraries(SimGear ${ZLIB_LIBRARY} ${OPENSCENEGRAPH_LIBRARIES})
endif(SIMGEAR_SHARED)
    