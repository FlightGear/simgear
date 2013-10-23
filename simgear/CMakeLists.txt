
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

foreach( mylibfolder 
        bucket
        bvh
        debug
        ephemeris
        io
        magvar
        math
        misc
        nasal
        nasal/cppbind
        props
        serial
        structure
        threads
        timing
        xml
        package
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if(NOT SIMGEAR_HEADLESS)
    add_subdirectory(canvas)
    add_subdirectory(environment)
    add_subdirectory(screen)
    add_subdirectory(scene)
    add_subdirectory(sound)
endif(NOT SIMGEAR_HEADLESS)


if(ENABLE_RTI)
    add_subdirectory(hla)
endif(ENABLE_RTI)


set(HEADERS compiler.h constants.h sg_inlines.h ${PROJECT_BINARY_DIR}/simgear/version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

get_property(coreSources GLOBAL PROPERTY CORE_SOURCES)
get_property(sceneSources GLOBAL PROPERTY SCENE_SOURCES)
get_property(publicHeaders GLOBAL PROPERTY PUBLIC_HEADERS)
get_property(localExpatSources GLOBAL PROPERTY LOCAL_EXPAT_SOURCES)

if(SIMGEAR_SHARED)
    message(STATUS "Library building mode: SHARED LIBRARIES")
    add_library(SimGearCore SHARED ${coreSources} ${localExpatSources})

    set_property(TARGET SimGearCore PROPERTY LINKER_LANGUAGE CXX)
    set_property(TARGET SimGearCore PROPERTY VERSION   ${SIMGEAR_VERSION})
    set_property(TARGET SimGearCore PROPERTY SOVERSION ${SIMGEAR_SOVERSION})
    install(TARGETS SimGearCore EXPORT SimGearCoreConfig LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(EXPORT SimGearCoreConfig DESTINATION share/SimGearCore)
    
    if(NOT SIMGEAR_HEADLESS)
        add_library(SimGearScene SHARED ${sceneSources})
        set_property(TARGET SimGearScene PROPERTY LINKER_LANGUAGE CXX)
        set_property(TARGET SimGearScene PROPERTY VERSION   ${SIMGEAR_VERSION})
        set_property(TARGET SimGearScene PROPERTY SOVERSION ${SIMGEAR_SOVERSION})

        # EXPORT SimGearSceneConfig
        install(TARGETS SimGearScene LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
    #    install(EXPORT SimGearSceneConfig DESTINATION share/SimGearScene)
    endif()
    
else()
    message(STATUS "Library building mode: STATIC LIBRARIES")

	get_property(FG_GROUPS_CORE_SOURCES_C GLOBAL PROPERTY FG_GROUPS_CORE_SOURCES_C)
	string(REPLACE "@" ";" groups ${FG_GROUPS_CORE_SOURCES_C} )
	foreach(g ${groups})
		string(REPLACE "#" ";" g2 ${g})
		list(GET g2 0 name)
		list(REMOVE_AT g2 0)
		source_group("${name}\\Sources" FILES ${g2})
	endforeach()

	get_property(FG_GROUPS_CORE_SOURCES_H GLOBAL PROPERTY FG_GROUPS_CORE_SOURCES_H)
	string(REPLACE "@" ";" groups ${FG_GROUPS_CORE_SOURCES_H} )
	foreach(g ${groups})
		string(REPLACE "#" ";" g2 ${g})
		list(GET g2 0 name)
		list(REMOVE_AT g2 0)
		source_group("${name}\\Headers" FILES ${g2})
	endforeach()

    add_library(SimGearCore STATIC ${coreSources} ${localExpatSources})
    install(TARGETS SimGearCore ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    
    if(NOT SIMGEAR_HEADLESS)
		get_property(FG_GROUPS_SCENE_SOURCES_C GLOBAL PROPERTY FG_GROUPS_SCENE_SOURCES_C)
		string(REPLACE "@" ";" groups ${FG_GROUPS_SCENE_SOURCES_C} )
		foreach(g ${groups})
			string(REPLACE "#" ";" g2 ${g})
			list(GET g2 0 name)
			list(REMOVE_AT g2 0)
			source_group("${name}\\Sources" FILES ${g2})
		endforeach()

		get_property(FG_GROUPS_SCENE_SOURCES_H GLOBAL PROPERTY FG_GROUPS_SCENE_SOURCES_H)
		string(REPLACE "@" ";" groups ${FG_GROUPS_SCENE_SOURCES_H} )
		foreach(g ${groups})
			string(REPLACE "#" ";" g2 ${g})
			list(GET g2 0 name)
			list(REMOVE_AT g2 0)
			source_group("${name}\\Headers" FILES ${g2})
		endforeach()

        add_library(SimGearScene STATIC ${sceneSources})
        install(TARGETS SimGearScene ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif(NOT SIMGEAR_HEADLESS)
endif(SIMGEAR_SHARED)

target_link_libraries(SimGearCore
    ${ZLIB_LIBRARY}
    ${RT_LIBRARY}
    ${DL_LIBRARY}
    ${EXPAT_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CORE_SERVICES_LIBRARY})

if(NOT SIMGEAR_HEADLESS)
    target_link_libraries(SimGearScene
        SimGearCore
        ${ZLIB_LIBRARY}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${OPENAL_LIBRARY}
        ${OPENGL_LIBRARY}
        ${JPEG_LIBRARY})
endif()

if(ENABLE_RTI)
    # Ugly first aid to make hla compile agian
    set_property(SOURCE hla/RTI13InteractionClass.cxx hla/RTI13ObjectClass.cxx
        hla/RTI13ObjectInstance.cxx hla/RTI13Federate.cxx
        hla/RTI13FederateFactory.cxx
        APPEND PROPERTY COMPILE_FLAGS "-I${RTI_INCLUDE_DIR}")
endif(ENABLE_RTI)
